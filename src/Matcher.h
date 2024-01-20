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
    static void match_wl(Graph<IT, VT>& graph, 
                        int num_threads);
    template <typename IT, typename VT>
    static void match_persistent_wl(Graph<IT, VT> &graph,
                                    moodycamel::ConcurrentQueue<IT> &worklist,
                                    bool &finished);
    template <typename IT, typename VT>
    static void match_persistent_wl2(Graph<IT, VT> &graph,
                                    moodycamel::ConcurrentQueue<IT> &worklist,
                                    std::vector<size_t> &read_messages,
                                    std::atomic<bool>& found_augmenting_path,
                                    std::atomic<IT> & currentRoot,
                                    std::mutex & mtx,
                                    std::condition_variable & cv,
                                    int tid,
                                    std::atomic<IT> & num_enqueued,
                                    std::atomic<IT> & num_dequeued,
                                    std::atomic<IT> & num_spinning);

private:
    template <typename IT, typename VT>
    static Vertex<IT> * search(Graph<IT, VT>& graph, 
                    const size_t V_index,
                    Frontier<IT> & f);
    template <typename IT, typename VT>
    static void next_iteration(Graph<IT, VT>& graph, 
                    std::atomic<IT> & currentRoot,
                    std::atomic<IT> & num_enqueued,
                    moodycamel::ConcurrentQueue<IT> &worklist);
    template <typename IT, typename VT>
    static Vertex<IT> * search_persistent(Graph<IT, VT>& graph, 
                    IT & V_index,
                    Frontier<IT> & f,
                    moodycamel::ConcurrentQueue<IT> &worklist,
                    int tid,
                    std::atomic<bool> & found_augmenting_path);
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
    const size_t N = graph.getN();
    //const size_t N = 5;
    for (std::size_t i = 0; i < N; ++i) {
        if (!graph.IsMatched(i)) {
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
        if (!graph.IsMatched(i)) {
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
void Matcher::match_wl(Graph<IT, VT>& graph, 
                        int num_threads) {
    size_t capacity = 1;
    moodycamel::ConcurrentQueue<IT> worklist{capacity};
    std::mutex mtx;
    std::condition_variable cv;
    std::atomic<IT> num_enqueued(0);
    std::atomic<IT> num_dequeued(0);
    std::atomic<IT> num_spinning(0);
    std::atomic<IT> currentRoot(-2);
    std::atomic<bool> found_augmenting_path(false);
    
    //std::vector<std::atomic<bool>> atomicBoolVector(num_threads);
    std::vector<std::thread> workers(num_threads);
    std::vector<size_t> read_messages;
    read_messages.resize(num_threads);
    //spinning.resize(num_threads,false);
    // Access the graph elements as needed
    ThreadFactory::create_threads_concurrentqueue_wl<IT,VT>(workers, num_threads,read_messages,worklist,graph,
    currentRoot,found_augmenting_path,
    mtx,cv,num_enqueued,num_dequeued,num_spinning);

    for (auto& t : workers) {
        t.join();
    }

    std::cout << "NUM ENQUEUED " << num_enqueued.load() << std::endl;
    std::cout << "NUM DEQUEUED " << num_dequeued.load() << std::endl;
    std::cout << "NUM SPINNING " << num_spinning.load() << std::endl;

    size_t tot_read_messages = 0;
    int ni = 0;
    for (size_t n : read_messages){
        printf("reader: \t\t %d read_messages %zu \n", ni++,n);
        tot_read_messages += n;
    }
    printf("Total read_messages %zu \n", tot_read_messages);
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
        if (!graph.IsMatched(i)) {
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
                                std::vector<size_t> &read_messages,
                                std::atomic<bool>& found_augmenting_path,
                                std::atomic<IT> & currentRoot,
                                std::mutex & mtx,
                                std::condition_variable & cv,
                                int tid,
                                std::atomic<IT> & num_enqueued,
                                std::atomic<IT> & num_dequeued,
                                std::atomic<IT> & num_spinning) {
    auto allocate_start = high_resolution_clock::now();
    Frontier<IT> f(graph.getN(),graph.getM());
    auto allocate_end = high_resolution_clock::now();
    auto duration_alloc = duration_cast<milliseconds>(allocate_end - allocate_start);
    std::cout << "Frontier (9|V|+|E|) memory allocation time: "<< duration_alloc.count() << " milliseconds" << '\n';
    Vertex<IT>* TailOfAugmentingPath;
    const size_t N = graph.getN();
    //const size_t N = 5;
    IT V_index;

    IT expected = -2;
    IT desired = -1;
    // First to encounter this code will see currentRoot == -2,
    // it will be atomically exchanged with -1, and return true.
    // All others will modify expected, inconsequentially,
    // and enter the while loop.
    if (currentRoot.compare_exchange_strong(expected, desired)) {
        next_iteration(graph,currentRoot,num_enqueued,worklist);
    }
    // finished_algorithm when currentRoot == N
    while(currentRoot.load(std::memory_order_relaxed)!=N){
        if (worklist.try_dequeue(V_index)){
            read_messages[tid]++;       

            // Turn on flag
            TailOfAugmentingPath = search_persistent(graph,V_index,f,worklist,tid,
            found_augmenting_path);

            // At all-spin state, there should be parity between 
            // num en/dequeue
            num_dequeued++;

            // All parallel searchers have at least gotten here.
            // It is possible that one
            // Always false
            bool expected = false;
            // Found AP
            bool desired = TailOfAugmentingPath != nullptr;
            // First searcher to find an AP
            // If any AP was found only one walker will enter this loop
            if (desired && found_augmenting_path.compare_exchange_strong(expected, desired)) {
                // Need to wait here to avoid augmenting while the graph is being traversed.
                while(num_dequeued.load()!=num_enqueued.load()){}
                augment(graph,TailOfAugmentingPath,f);
            }

            //num_spinning++;
            if (1+num_spinning.fetch_add(1) == num_dequeued.load() &&
                num_dequeued.load() == num_enqueued.load()) {
                found_augmenting_path.store(false);
                next_iteration(graph,currentRoot,num_enqueued,worklist);
            }

            f.reinit();
            f.clear();

        } else {
            continue;
        }
    }
    // The worker thread has done the work,
    // Notify the master thread to continue the work.
    //cv.notify_one();
}

template <typename IT, typename VT>
void Matcher::next_iteration(Graph<IT, VT>& graph, 
                    std::atomic<IT> & currentRoot,
                    std::atomic<IT> & num_enqueued,
                    moodycamel::ConcurrentQueue<IT> &worklist){
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
    //finished_algorithm.store(currentRoot==graph.getN(),std::memory_order_relaxed);
}

template <typename IT, typename VT>
Vertex<IT> * Matcher::search_persistent(Graph<IT, VT>& graph, 
                    IT & V_index,
                    Frontier<IT> & f,
                    moodycamel::ConcurrentQueue<IT> &worklist,
                    int tid,
                    std::atomic<bool> & found_augmenting_path) {
    Vertex<IT> *FromBase,*ToBase, *nextVertex;
    IT FromBaseVertexID,ToBaseVertexID;
    IT stackEdge, matchedEdge;
    IT nextVertexIndex;
    IT time = 0;
    Stack<IT> &stack = f.stack;
    Stack<Vertex<IT>> &tree = f.tree;
    DisjointSetUnion<IT> &dsu = f.dsu;
    std::vector<Vertex<IT>> & vertexVector = f.vertexVector;

    nextVertex = &vertexVector[V_index];
    nextVertex->AgeField=time++;
    tree.push_back(*nextVertex);

    // Push edges onto stack, breaking if that stackEdge is a solution.
    Graph<IT,VT>::pushEdgesOntoStack(graph,vertexVector,V_index,stack);
    // Gracefully exit other searchers if an augmenting path is found.
    while(!stack.empty() && !found_augmenting_path.load(std::memory_order_relaxed)){
        stackEdge = stack.back();
        stack.pop_back();

        #ifndef NDEBUG
        FromBaseVertexID = dsu[Graph<IT,VT>::EdgeFrom(graph,stackEdge)];
        auto FromBaseVertexIDTest = DisjointSetUnionHelper<IT>::getBase(Graph<IT,VT>::EdgeFrom(graph,stackEdge),vertexVector);  
        assert(FromBaseVertexID==FromBaseVertexIDTest);
        #else
        FromBaseVertexID = DisjointSetUnionHelper<IT>::getBase(Graph<IT,VT>::EdgeFrom(graph,stackEdge),vertexVector);  
        #endif
        FromBase = &vertexVector[FromBaseVertexID];

        // Necessary because vertices dont know their own index.
        // It simplifies vector creation..
        #ifndef NDEBUG
        ToBaseVertexID = dsu[Graph<IT,VT>::EdgeTo(graph,stackEdge)];
        auto ToBaseVertexIDTest = DisjointSetUnionHelper<IT>::getBase(Graph<IT,VT>::EdgeTo(graph,stackEdge),vertexVector);  
        assert(ToBaseVertexID==ToBaseVertexIDTest);
        #else
        ToBaseVertexID = DisjointSetUnionHelper<IT>::getBase(Graph<IT,VT>::EdgeTo(graph,stackEdge),vertexVector);  
        #endif
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
            tree.push_back(*ToBase);
            // I'll let the augment path method recover the path.
            return ToBase;
        } else if (!ToBase->IsReached() && graph.IsMatched(ToBaseVertexID)){
            ToBase->TreeField=stackEdge;
            ToBase->AgeField=time++;
            tree.push_back(*ToBase);

            matchedEdge=graph.GetMatchField(ToBaseVertexID);
            nextVertexIndex = Graph<IT,VT>::Other(graph,matchedEdge,ToBaseVertexID);
            nextVertex = &vertexVector[nextVertexIndex];
            nextVertex->AgeField=time++;
            tree.push_back(*nextVertex);

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
Vertex<IT> * Matcher::search(Graph<IT, VT>& graph, 
                    const size_t V_index,
                    Frontier<IT> & f) {
    Vertex<IT> *FromBase,*ToBase, *nextVertex;
    IT FromBaseVertexID,ToBaseVertexID;
    IT stackEdge, matchedEdge;
    IT nextVertexIndex;
    IT time = 0;
    Stack<IT> &stack = f.stack;
    Stack<Vertex<IT>> &tree = f.tree;
    DisjointSetUnion<IT> &dsu = f.dsu;
    std::vector<Vertex<IT>> & vertexVector = f.vertexVector;
    //auto inserted = vertexMap.try_emplace(V_index,Vertex<IT>(time++,Label::EvenLabel));
    nextVertex = &vertexVector[V_index];
    nextVertex->AgeField=time++;
    tree.push_back(*nextVertex);

    // Push edges onto stack, breaking if that stackEdge is a solution.
    Graph<IT,VT>::pushEdgesOntoStack(graph,vertexVector,V_index,stack);
    while(!stack.empty()){
        stackEdge = stack.back();
        stack.pop_back();
        // Necessary because vertices dont know their own index.
        // It simplifies vector creation..
        #ifndef NDEBUG
        FromBaseVertexID = dsu[Graph<IT,VT>::EdgeFrom(graph,stackEdge)];
        auto FromBaseVertexIDTest = DisjointSetUnionHelper<IT>::getBase(Graph<IT,VT>::EdgeFrom(graph,stackEdge),vertexVector);  
        assert(FromBaseVertexID==FromBaseVertexIDTest);
        #else
        FromBaseVertexID = DisjointSetUnionHelper<IT>::getBase(Graph<IT,VT>::EdgeFrom(graph,stackEdge),vertexVector);  
        #endif
        FromBase = &vertexVector[FromBaseVertexID];

        // Necessary because vertices dont know their own index.
        // It simplifies vector creation..
        #ifndef NDEBUG
        ToBaseVertexID = dsu[Graph<IT,VT>::EdgeTo(graph,stackEdge)];
        auto ToBaseVertexIDTest = DisjointSetUnionHelper<IT>::getBase(Graph<IT,VT>::EdgeTo(graph,stackEdge),vertexVector);  
        assert(ToBaseVertexID==ToBaseVertexIDTest);
        #else
        ToBaseVertexID = DisjointSetUnionHelper<IT>::getBase(Graph<IT,VT>::EdgeTo(graph,stackEdge),vertexVector);  
        #endif

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
            tree.push_back(*ToBase);
            //graph.SetMatchField(ToBaseVertexID,stackEdge);
            // I'll let the augment path method recover the path.
            return ToBase;
        } else if (!ToBase->IsReached() && graph.IsMatched(ToBaseVertexID)){
            ToBase->TreeField=stackEdge;
            ToBase->AgeField=time++;
            tree.push_back(*ToBase);

            matchedEdge=graph.GetMatchField(ToBaseVertexID);
            nextVertexIndex = Graph<IT,VT>::Other(graph,matchedEdge,ToBaseVertexID);
            nextVertex = &vertexVector[nextVertexIndex];
            nextVertex->AgeField=time++;
            tree.push_back(*nextVertex);

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
        // GJS
        #ifndef NDEBUG
        auto nextVertexBaseID = dsu[nextVertexID];
        auto nextVertexBaseIDTest = DisjointSetUnionHelper<IT>::getBase(nextVertexID,vertexVector);  
        assert(nextVertexBaseIDTest==nextVertexBaseID);
        #else
        auto nextVertexBaseID = DisjointSetUnionHelper<IT>::getBase(nextVertexID,vertexVector);  
        #endif

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
