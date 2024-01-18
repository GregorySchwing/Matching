#ifndef Matcher_H
#define Matcher_H

#include "Graph.h"
#include "Vertex.h"
#include <list>
#include <unordered_map>
#include "Enums.h"
#include "DSU.h"
//#include "DSU2.h"

#include "Blossom.h"
#include "Stack.h"
#include "Frontier.h"
#include "Statistics.h"
#include "concurrentqueue.h"

class Matcher {
public:
    template <typename IT, typename VT>
    static void match(Graph<IT, VT>& graph);
    template <typename IT, typename VT>
    static void match(Graph<IT, VT>& graph, Statistics<IT>& stats);
    template <typename IT, typename VT>
    static void match_wl(Graph<IT, VT>& graph, Statistics<IT>& stats);
    template <typename IT, typename VT>
    static void match_persistent_wl(Graph<IT, VT> &graph,
                                    moodycamel::ConcurrentQueue<IT> &worklist,
                                    bool &finished);
    template <typename IT, typename VT>
    static void match_persistent_wl2(Graph<IT, VT> &graph,
                                    moodycamel::ConcurrentQueue<IT> &worklist,
                                    bool &finished_iteration,
                                    bool &finished_algorithm,
                                    IT &i,
                                    std::mutex & mtx,
                                    std::condition_variable & cv,
                                    int tid,
                                    std::atomic<IT> & num_enqueued,
                                    std::atomic<IT> & num_dequeued,
                                    std::atomic<IT> & num_running,
                                    std::atomic<IT> & num_spinning,
                                    std::vector<bool> &spinning,
                                    std::vector<std::atomic<bool>> &atomicBoolVector,
                                    const int numThreads);

private:
    template <typename IT, typename VT>
    static Vertex<IT> * search(Graph<IT, VT>& graph, 
                    const size_t V_index,
                    Frontier<IT> & f);
    template <typename IT, typename VT>
    static void next_iteration(Graph<IT, VT>& graph, 
                    IT &V_index,
                    std::atomic<IT> & num_enqueued,
                    moodycamel::ConcurrentQueue<IT> &worklist,
                    bool & finished_algorithm);
    template <typename IT, typename VT>
    static void search_persistent(Graph<IT, VT>& graph, 
                    IT &V_index,
                    Frontier<IT> & f,
                    moodycamel::ConcurrentQueue<IT> &worklist,
                    int tid,
                    std::atomic<IT> & num_enqueued,
                    std::atomic<IT> & num_dequeued,
                    std::atomic<IT> & num_running,
                    std::atomic<IT> & num_spinning,
                    bool & finished_algorithm,
                    const int numThreads);
    template <typename IT, typename VT>
    static void augment(Graph<IT, VT>& graph, 
                    Vertex<IT> * TailOfAugmentingPath,
                    Frontier<IT> & f);
    template <typename IT, typename VT>
    static void pathThroughBlossom(Graph<IT, VT>& graph, 
                        // V
                        const Vertex<IT> * TailOfAugmentingPath,
                        const Vertex<IT> * TailOfAugmentingPathBase,
                        std::vector<Vertex<IT>> & vertexVector,
                        //std::list<IT> & path,
                        Stack<IT> & path);
};

template <typename IT, typename VT>
void Matcher::match(Graph<IT, VT>& graph) {
    auto allocate_start = high_resolution_clock::now();
    Frontier<IT> f(graph.getN(),graph.getM());
    auto allocate_end = high_resolution_clock::now();
    auto duration_alloc = duration_cast<milliseconds>(allocate_end - allocate_start);
    std::cout << "Frontier (9|V|+|E|) memory allocation time: "<< duration_alloc.count() << " milliseconds" << '\n';
    Vertex<IT> * TailOfAugmentingPath;
    // Access the graph elements as needed
    for (std::size_t i = 0; i < graph.getN(); ++i) {
        if (graph.matching[i] < 0) {
            //printf("SEARCHING FROM %ld!\n",i);
            // Your matching logic goes here...
            TailOfAugmentingPath=search(graph,i,f);
            // If not a nullptr, I found an AP.
            if (TailOfAugmentingPath){
                augment(graph,TailOfAugmentingPath,f);
                f.reinit();
                f.clear();
                //printf("FOUND AP!\n");
            } else {
                f.clear();
                //printf("DIDNT FOUND AP!\n");
            }
        }
    }
}


template <typename IT, typename VT>
void Matcher::match(Graph<IT, VT>& graph, Statistics<IT>& stats) {
    auto allocate_start = high_resolution_clock::now();
    Frontier<IT> f(graph.getN(),graph.getM());
    auto allocate_end = high_resolution_clock::now();
    auto duration_alloc = duration_cast<milliseconds>(allocate_end - allocate_start);
    std::cout << "Frontier (9|V|+|E|) memory allocation time: "<< duration_alloc.count() << " milliseconds" << '\n';
    Vertex<IT> * TailOfAugmentingPath;
    // Access the graph elements as needed
    for (std::size_t i = 0; i < graph.getN(); ++i) {
        if (graph.matching[i] < 0) {
            //printf("SEARCHING FROM %ld!\n",i);
            // Your matching logic goes here...
            auto search_start = high_resolution_clock::now();
            TailOfAugmentingPath=search(graph,i,f);
            auto search_end = high_resolution_clock::now();
            // If not a nullptr, I found an AP.
            if (TailOfAugmentingPath){
                augment(graph,TailOfAugmentingPath,f);
                stats.write_entry(f.path.size() ? (2*f.path.size()-1):0,f.tree.size(),duration_cast<microseconds>(search_end - search_start));
                f.reinit();
                f.clear();
                //printf("FOUND AP!\n");
            } else {
                stats.write_entry(f.path.size() ? (2*f.path.size()-1):0,f.tree.size(),duration_cast<microseconds>(search_end - search_start));
                f.clear();
                //printf("DIDNT FOUND AP!\n");
            }
        }
    }
}

#include "ThreadFactory.h"
template <typename IT, typename VT>
void Matcher::match_wl(Graph<IT, VT>& graph, Statistics<IT>& stats) {
    size_t capacity = 1;
    moodycamel::ConcurrentQueue<IT> worklist{capacity};
    std::mutex mtx;
    std::condition_variable cv;
    std::atomic<IT> num_enqueued = 0;
    std::atomic<IT> num_dequeued = 0;
    std::atomic<IT> num_running = 0;
    std::atomic<IT> num_spinning = 0;
    std::vector<bool> spinning;
    IT currentRoot = 0;
    // 8 workers.
    bool finished_iteration = false;
    bool finished_algorithm = false;
    unsigned num_threads = 8;
    std::vector<std::atomic<bool>> atomicBoolVector(num_threads);
    std::vector<std::thread> workers(num_threads);
    std::vector<size_t> read_messages;
    // Assign all elements in the vector to false
    for (auto& atomicBool : atomicBoolVector) {
        atomicBool.store(false);
    }

    read_messages.resize(num_threads);
    spinning.resize(num_threads,false);
    // Access the graph elements as needed
    ThreadFactory::create_threads_concurrentqueue_wl<IT,VT>(workers, num_threads,read_messages,worklist,graph,
    currentRoot,finished_iteration,finished_algorithm,
    mtx,cv,num_enqueued,num_dequeued,num_running,num_spinning,spinning,atomicBoolVector);

    auto match_start = high_resolution_clock::now();
    for (; currentRoot < graph.getN(); ++currentRoot) {
        if (!graph.IsMatched(currentRoot)) {
            // This prevents race-conditions at end.
            num_enqueued++;
            worklist.enqueue(currentRoot);
        }
        // Rest of pushes are done by the persistent threads.
        break;
    }
    // Wait for the worker.
    {
        std::unique_lock lk(mtx);
        cv.wait(lk, [&] { return finished_algorithm; });
    }

    std::cout << "NUM ENQUEUED " << num_enqueued.load() << std::endl;
    std::cout << "NUM DEQUEUED " << num_dequeued.load() << std::endl;
    std::cout << "NUM SPINNING " << num_spinning.load() << std::endl;

    auto match_end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(match_end - match_start);
    for (auto& t : workers) {
        t.join();
    }
}


template <typename IT, typename VT>
void Matcher::match_persistent_wl(Graph<IT, VT>& graph,
                                moodycamel::ConcurrentQueue<IT> &worklist,
                                bool &finished) {
    auto allocate_start = high_resolution_clock::now();
    Frontier<IT> f(graph.getN(),graph.getM());
    auto allocate_end = high_resolution_clock::now();
    auto duration_alloc = duration_cast<milliseconds>(allocate_end - allocate_start);
    std::cout << "Frontier (9|V|+|E|) memory allocation time: "<< duration_alloc.count() << " milliseconds" << '\n';
    Vertex<IT> * TailOfAugmentingPath;
    // Access the graph elements as needed
    for (std::size_t i = 0; i < graph.getN(); ++i) {
        if (graph.matching[i] < 0) {
            //printf("SEARCHING FROM %ld!\n",i);
            // Your matching logic goes here...
            auto search_start = high_resolution_clock::now();
            TailOfAugmentingPath=search(graph,i,f);
            auto search_end = high_resolution_clock::now();
            // If not a nullptr, I found an AP.
            if (TailOfAugmentingPath){
                augment(graph,TailOfAugmentingPath,f);
                //stats.write_entry(f.path.size() ? (2*f.path.size()-1):0,f.tree.size(),duration_cast<microseconds>(search_end - search_start));
                f.reinit();
                f.clear();
                //printf("FOUND AP!\n");
            } else {
                //stats.write_entry(f.path.size() ? (2*f.path.size()-1):0,f.tree.size(),duration_cast<microseconds>(search_end - search_start));
                f.clear();
                //printf("DIDNT FOUND AP!\n");
            }
        }
    }
}

template <typename IT, typename VT>
void Matcher::match_persistent_wl2(Graph<IT, VT>& graph,
                                moodycamel::ConcurrentQueue<IT> &worklist,
                                bool &finished_iteration,
                                bool &finished_algorithm,
                                IT &currentRoot,
                                std::mutex & mtx,
                                std::condition_variable & cv,
                                int tid,
                                std::atomic<IT> & num_enqueued,
                                std::atomic<IT> & num_dequeued,
                                std::atomic<IT> & num_running,
                                std::atomic<IT> & num_spinning,
                                std::vector<bool> &spinning,
                                std::vector<std::atomic<bool>> &atomicBoolVector,
                                const int numThreads) {
    auto allocate_start = high_resolution_clock::now();
    Frontier<IT> f(graph.getN(),graph.getM());
    auto allocate_end = high_resolution_clock::now();
    auto duration_alloc = duration_cast<milliseconds>(allocate_end - allocate_start);
    std::cout << "Frontier (9|V|+|E|) memory allocation time: "<< duration_alloc.count() << " milliseconds" << '\n';
    Vertex<IT> * TailOfAugmentingPath;
    // Access the graph elements as needed
    //for (std::size_t i = 0; i < graph.getN(); ++i) {
    num_running++;
    while(!finished_algorithm){
        if (worklist.try_dequeue(currentRoot)){
            // This is how you prevent race conditions.
            // by decrementing number of spinning before incrementing
            // num_dequeued.
            // At all-spin state, there should be parity between 
            // num en/dequeue and NT-1 threads spinning.
            if (atomicBoolVector[tid]) {
                atomicBoolVector[tid].store(false);
                num_spinning--;
            }
            num_dequeued++;
            // Turn on flag
            search_persistent(graph,currentRoot,f,worklist,tid,
            num_enqueued,num_dequeued,num_running,num_spinning,finished_algorithm,numThreads);
        } else {
            // Avoid lots of atomic ops when possible.
            if (!atomicBoolVector[tid]) {
                atomicBoolVector[tid].store(true);
                num_spinning++;
            }
            if (num_dequeued.load()==num_enqueued.load() &&
                num_spinning.load()==num_running.load() &&
                tid == 0){
                next_iteration(graph,currentRoot,num_enqueued,worklist,finished_algorithm);
            }
            continue;
        }
    }
    // The worker thread has done the work,
    // Notify the master thread to continue the work.
    cv.notify_one();
}

template <typename IT, typename VT>
void Matcher::next_iteration(Graph<IT, VT>& graph, 
                    IT &currentRoot,
                    std::atomic<IT> & num_enqueued,
                    moodycamel::ConcurrentQueue<IT> &worklist,
                    bool & finished_algorithm){
    while(++currentRoot < graph.getN()){
        if (!graph.IsMatched(currentRoot)) {
            //printf("Enqueuing %d\n",i);
            num_enqueued++;
            worklist.enqueue(currentRoot);
            break;
        }
        // Rest of pushes are done by the persistent threads.
    }
    // Turn off flag
    finished_algorithm = (currentRoot==graph.getN());
}

template <typename IT, typename VT>
void Matcher::search_persistent(Graph<IT, VT>& graph, 
                    IT &V_index,
                    Frontier<IT> & f,
                    moodycamel::ConcurrentQueue<IT> &worklist,
                    int tid,
                    std::atomic<IT> & num_enqueued,
                    std::atomic<IT> & num_dequeued,
                    std::atomic<IT> & num_running,
                    std::atomic<IT> & num_spinning,
                    bool & finished_algorithm,
                    const int numThreads) {
    Vertex<int64_t> *FromBase,*ToBase, *nextVertex;
    int64_t FromBaseVertexID,ToBaseVertexID;
    IT stackEdge, matchedEdge;
    IT nextVertexIndex;
    IT time = 0;
    Stack<IT> &stack = f.stack;
    Stack<IT> &tree = f.tree;
    DisjointSetUnion<IT> &dsu = f.dsu;
    std::vector<Vertex<IT>> & vertexVector = f.vertexVector;
    //auto inserted = vertexMap.try_emplace(V_index,Vertex<IT>(time++,Label::EvenLabel));
    nextVertex = &vertexVector[V_index];
    tree.push_back(V_index);
    nextVertex->AgeField=time++;
    // Push edges onto stack, breaking if that stackEdge is a solution.
    Graph<IT,VT>::pushEdgesOntoStack(graph,vertexVector,V_index,stack);
    while(!stack.empty()){
        stackEdge = stack.back();
        stack.pop_back();
        // Necessary because vertices dont know their own index.
        // It simplifies vector creation..
        FromBaseVertexID = dsu[Graph<IT,VT>::EdgeFrom(graph,stackEdge)];
        FromBase = &vertexVector[FromBaseVertexID];

        // Necessary because vertices dont know their own index.
        // It simplifies vector creation..
        ToBaseVertexID = dsu[Graph<IT,VT>::EdgeTo(graph,stackEdge)];
        ToBase = &vertexVector[ToBaseVertexID];

        // Edge is between two vertices in the same blossom, continue.
        if (FromBase == ToBase)
            continue;
        if(!FromBase->IsEven()){
            std::swap(FromBase,ToBase);
            std::swap(FromBaseVertexID,ToBaseVertexID);
        }
        // An unreached, unmatched vertex is found, AN AUGMENTING PATH!
        if (!ToBase->IsReached() && !graph.IsMatched(ToBaseVertexID)){
            ToBase->TreeField=stackEdge;
            ToBase->AgeField=time++;
            tree.push_back(ToBaseVertexID);
            //graph.SetMatchField(ToBaseVertexID,stackEdge);
            // I'll let the augment path method recover the path.
            augment(graph,ToBase,f);
            f.reinit();
            f.clear();
            next_iteration(graph,V_index,num_enqueued,worklist,finished_algorithm);
            return;
        } else if (!ToBase->IsReached() && graph.IsMatched(ToBaseVertexID)){
            ToBase->TreeField=stackEdge;
            ToBase->AgeField=time++;
            tree.push_back(ToBaseVertexID);

            matchedEdge=graph.GetMatchField(ToBaseVertexID);
            nextVertexIndex = Graph<IT,VT>::Other(graph,matchedEdge,ToBaseVertexID);
            nextVertex = &vertexVector[nextVertexIndex];
            nextVertex->AgeField=time++;
            tree.push_back(nextVertexIndex);

            Graph<IT,VT>::pushEdgesOntoStack(graph,vertexVector,nextVertexIndex,stack,matchedEdge);

        } else if (ToBase->IsEven()) {
            // Shrink Blossoms
            // Not sure if this is wrong or the augment method is wrong
            Blossom::Shrink(graph,stackEdge,dsu,vertexVector,stack);
        }
    }
    f.clear();
    return;
}

template <typename IT, typename VT>
Vertex<IT> * Matcher::search(Graph<IT, VT>& graph, 
                    const size_t V_index,
                    Frontier<IT> & f) {
    Vertex<int64_t> *FromBase,*ToBase, *nextVertex;
    int64_t FromBaseVertexID,ToBaseVertexID;
    IT stackEdge, matchedEdge;
    IT nextVertexIndex;
    IT time = 0;
    Stack<IT> &stack = f.stack;
    Stack<IT> &tree = f.tree;
    DisjointSetUnion<IT> &dsu = f.dsu;
    std::vector<Vertex<IT>> & vertexVector = f.vertexVector;
    //auto inserted = vertexMap.try_emplace(V_index,Vertex<IT>(time++,Label::EvenLabel));
    nextVertex = &vertexVector[V_index];
    tree.push_back(V_index);
    nextVertex->AgeField=time++;
    // Push edges onto stack, breaking if that stackEdge is a solution.
    Graph<IT,VT>::pushEdgesOntoStack(graph,vertexVector,V_index,stack);
    while(!stack.empty()){
        stackEdge = stack.back();
        stack.pop_back();
        // Necessary because vertices dont know their own index.
        // It simplifies vector creation..
        FromBaseVertexID = dsu[Graph<IT,VT>::EdgeFrom(graph,stackEdge)];
        FromBase = &vertexVector[FromBaseVertexID];

        // Necessary because vertices dont know their own index.
        // It simplifies vector creation..
        ToBaseVertexID = dsu[Graph<IT,VT>::EdgeTo(graph,stackEdge)];
        ToBase = &vertexVector[ToBaseVertexID];

        // Edge is between two vertices in the same blossom, continue.
        if (FromBase == ToBase)
            continue;
        if(!FromBase->IsEven()){
            std::swap(FromBase,ToBase);
            std::swap(FromBaseVertexID,ToBaseVertexID);
        }
        // An unreached, unmatched vertex is found, AN AUGMENTING PATH!
        if (!ToBase->IsReached() && !graph.IsMatched(ToBaseVertexID)){
            ToBase->TreeField=stackEdge;
            ToBase->AgeField=time++;
            tree.push_back(ToBaseVertexID);
            //graph.SetMatchField(ToBaseVertexID,stackEdge);
            // I'll let the augment path method recover the path.
            return ToBase;
        } else if (!ToBase->IsReached() && graph.IsMatched(ToBaseVertexID)){
            ToBase->TreeField=stackEdge;
            ToBase->AgeField=time++;
            tree.push_back(ToBaseVertexID);

            matchedEdge=graph.GetMatchField(ToBaseVertexID);
            nextVertexIndex = Graph<IT,VT>::Other(graph,matchedEdge,ToBaseVertexID);
            nextVertex = &vertexVector[nextVertexIndex];
            nextVertex->AgeField=time++;
            tree.push_back(nextVertexIndex);

            Graph<IT,VT>::pushEdgesOntoStack(graph,vertexVector,nextVertexIndex,stack,matchedEdge);

        } else if (ToBase->IsEven()) {
            // Shrink Blossoms
            // Not sure if this is wrong or the augment method is wrong
            Blossom::Shrink(graph,stackEdge,dsu,vertexVector,stack);
        }
    }
    return nullptr;
}

template <typename IT, typename VT>
void Matcher::augment(Graph<IT, VT>& graph, 
                    Vertex<IT> * TailOfAugmentingPath,
                    Frontier<IT> & f) {

    DisjointSetUnion<IT> &dsu = f.dsu;
    std::vector<Vertex<IT>> & vertexVector = f.vertexVector;
    //std::list<IT> path;
    Stack<IT> & path = f.path;
    IT edge;
    // W
    Vertex<IT>*nextVertex;
    Vertex<IT>*nextVertexBase;
    do
    {
        //ListPut(Tree(V), P);
        edge = TailOfAugmentingPath->TreeField;
        path.push_back(edge);

        //W = Other(Tree(V), V);
        ptrdiff_t TailOfAugmentingPath_VertexID = TailOfAugmentingPath - &vertexVector[0];
        auto nextVertexID = Graph<IT,VT>::Other(graph,edge,TailOfAugmentingPath_VertexID);
        nextVertex = &vertexVector[nextVertexID];

        //B = Base(Blossom(W));
        auto nextVertexBaseID = dsu[nextVertexID];  
        nextVertexBase = &vertexVector[nextVertexBaseID];
        
        // Path(W, B, P);
        pathThroughBlossom(graph,nextVertex,nextVertexBase,vertexVector,path);

        //V = Other(Match(B), B);
        ptrdiff_t nextVertexBase_VertexID = nextVertexBase - &vertexVector[0];
        if (graph.IsMatched(nextVertexBase_VertexID))
            TailOfAugmentingPath = &vertexVector[Graph<IT,VT>::Other(graph,graph.GetMatchField(nextVertexBase_VertexID),nextVertexBase_VertexID)];
        else 
            TailOfAugmentingPath = nullptr;
    } while (TailOfAugmentingPath != nullptr);
    // Print the list of integers
    for (auto E : path) {
        //Match(EdgeFrom(E)) = E;
        graph.SetMatchField(Graph<IT,VT>::EdgeFrom(graph,E),E);
        //Match(EdgeTo(E)) = E;
        graph.SetMatchField(Graph<IT,VT>::EdgeTo(graph,E),E);
    }
}


template <typename IT, typename VT>
void Matcher::pathThroughBlossom(Graph<IT, VT>& graph, 
                    // V
                    const Vertex<IT> * TailOfAugmentingPath,
                    const Vertex<IT> * TailOfAugmentingPathBase,
                    std::vector<Vertex<IT>> & vertexVector,
                    //std::list<IT> & path,
                    Stack<IT> & path) {
    // W
    Vertex<IT>*nextVertex;
    // if (V != B)
    if (TailOfAugmentingPath != TailOfAugmentingPathBase)
    {
        if (TailOfAugmentingPath->IsOdd())
        {
            // Path(Shore(V), Other(Match(V), V), P);
            ptrdiff_t TailOfAugmentingPath_VertexID = TailOfAugmentingPath - &vertexVector[0];
            pathThroughBlossom(graph,
                                &vertexVector[TailOfAugmentingPath->ShoreField],
                                &vertexVector[Graph<IT,VT>::Other(graph,graph.GetMatchField(TailOfAugmentingPath_VertexID),TailOfAugmentingPath_VertexID)],
                                vertexVector,
                                path);
            //ListPut(Bridge(V), P);
            path.push_back(TailOfAugmentingPath->BridgeField);
            
            //Path(Other(Bridge(V), Shore(V)), B, P);
            pathThroughBlossom(graph,
                                &vertexVector[Graph<IT,VT>::Other(graph,TailOfAugmentingPath->BridgeField,TailOfAugmentingPath->ShoreField)],
                                TailOfAugmentingPathBase,
                                vertexVector,
                                path);
        }
        else if (TailOfAugmentingPath->IsEven())
        {
            //W = Other(Match(V), V);
            ptrdiff_t TailOfAugmentingPath_VertexID = TailOfAugmentingPath - &vertexVector[0];
            nextVertex=&vertexVector[Graph<IT,VT>::Other(graph,graph.GetMatchField(TailOfAugmentingPath_VertexID),TailOfAugmentingPath_VertexID)];
            
            //ListPut(Tree(W), P);
            path.push_back(nextVertex->TreeField);

            //Path(Other(Tree(W), W), B, P);
            ptrdiff_t nextVertex_VertexID = nextVertex - &vertexVector[0];
            pathThroughBlossom(graph,
                                &vertexVector[Graph<IT,VT>::Other(graph,nextVertex->TreeField,nextVertex_VertexID)],
                                TailOfAugmentingPathBase,
                                vertexVector,
                                path);
        }
        else{
            ptrdiff_t TailOfAugmentingPath_VertexID = TailOfAugmentingPath - &vertexVector[0];
            ptrdiff_t TailOfAugmentingPathBase_VertexID = TailOfAugmentingPathBase - &vertexVector[0];
            std::cerr << "(Path) Internal error. TailOfAugmentingPath_VertexID: " << TailOfAugmentingPath_VertexID<< " TailOfAugmentingPathBase_VertexID: " << TailOfAugmentingPathBase_VertexID << std::endl;
            exit(1);
        }
    }
}


#endif
