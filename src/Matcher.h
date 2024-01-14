#ifndef Matcher_H
#define Matcher_H

#include "Graph.h"
#include "Vertex.h"
#include <list>
#include <unordered_map>
#include "Enums.h"
#include "DSU.h"
#include "Blossom.h"
#include "Stack.h"
#include "Frontier.h"
#include "Statistics.h"
// Parallel
#include "ThreadFactory.h"
#include <cassert>
#include "concurrentqueue.h"


class Matcher {
public:
    template <typename IT, typename VT>
    static void match(Graph<IT, VT>& graph);
    template <typename IT, typename VT>
    static void match_parallel_bench(Graph<IT, VT>& graph);
    template <typename IT, typename VT>
    static void match_parallel(Graph<IT, VT>& graph);
    template <typename IT, typename VT>
    static void match_parallel_one_atomic(Graph<IT, VT>& graph);
    template <typename IT, typename VT>
    static void match_parallel_baseline(Graph<IT, VT>& graph);
    template <typename IT, typename VT>
    static void match(Graph<IT, VT>& graph, Statistics<IT>& stats);

private:
    template <typename IT, typename VT>
    static void create_threads_concurrentqueue_bench(std::vector<std::thread> &threads, 
                                    unsigned num_threads,
                                    std::vector<size_t> & read_messages,
                                    Graph<IT, VT>& graph, 
                                    moodycamel::ConcurrentQueue<IT> &q,
                                    std::atomic<IT> &root,
                                    std::atomic<bool> &foundPath,
                                    std::atomic<bool> &finished);
    template <typename IT, typename VT>
    static void create_threads_concurrentqueue(std::vector<std::thread> &threads, 
                                    unsigned num_threads,
                                    std::vector<size_t> & read_messages,
                                    Graph<IT, VT>& graph, 
                                    moodycamel::ConcurrentQueue<IT> &q,
                                    std::atomic<IT> &root,
                                    std::atomic<bool> &foundPath,
                                    std::atomic<IT> &activeThreads,
                                    std::atomic<bool> &finished);
    template <typename IT, typename VT>
    static void create_threads_concurrentqueue_one_atomic(std::vector<std::thread> &threads, 
                                    unsigned num_threads,
                                    std::vector<size_t> & read_messages,
                                    Graph<IT, VT>& graph, 
                                    moodycamel::ConcurrentQueue<IT> &q,
                                    std::atomic<IT> &root,
                                    volatile bool &foundPath,
                                    std::atomic<IT> &activeThreads,
                                    volatile bool &finished);
    template <typename IT, typename VT>
    static void create_threads_concurrentqueue_baseline(std::vector<std::thread> &threads, 
                                    unsigned num_threads,
                                    std::vector<size_t> & read_messages,
                                    Graph<IT, VT>& graph, 
                                    moodycamel::ConcurrentQueue<IT> &q,
                                    volatile bool &foundPath,
                                    volatile bool &finished);
    template <typename IT, typename VT>
    static Vertex<IT> * search(Graph<IT, VT>& graph, 
                    const size_t V_index,
                    Frontier<IT> & f);

    template <typename IT, typename VT>
    static void search_persistent_bench(Graph<IT, VT>& graph,
                                    moodycamel::ConcurrentQueue<IT> &q,
                                    std::atomic<IT> &root,
                                    std::atomic<bool> &finished,
                                    std::atomic<bool> &foundPath,
                                    std::vector<size_t> &read_messages,
                                    int tid);
    template <typename IT, typename VT>
    static void search_persistent(Graph<IT, VT>& graph,
                                    moodycamel::ConcurrentQueue<IT> &q,
                                    std::atomic<IT> &root,
                                    std::atomic<bool> &foundPath,
                                    std::atomic<IT> &activeThreads,
                                    std::atomic<bool> &finished,
                                    std::vector<size_t> &read_messages,
                                    int tid);
    template <typename IT, typename VT>
    static void search_persistent_one_atomic(Graph<IT, VT>& graph,
                                    moodycamel::ConcurrentQueue<IT> &q,
                                    std::atomic<IT> &root,
                                    volatile bool &foundPath,
                                    std::atomic<IT> &activeThreads,
                                    volatile bool &finished,
                                    std::vector<size_t> &read_messages,
                                    int tid);
    template <typename IT, typename VT>
    static void search_persistent_baseline(Graph<IT, VT>& graph,
                                    moodycamel::ConcurrentQueue<IT> &q,
                                    std::vector<size_t> &read_messages,
                                    volatile bool &foundPath,
                                    volatile bool &finished,
                                    int tid);
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
void Matcher::match_parallel_bench(Graph<IT, VT>& graph) {
  moodycamel::ConcurrentQueue<IT> q;
  std::atomic<IT> root;
  std::atomic<bool> finished = false;
  std::atomic<bool> foundPath = false;

  constexpr unsigned num_threads = 15;
  std::vector<std::thread> threads(num_threads);
  std::vector<size_t> read_messages;
  read_messages.resize(num_threads);
  create_threads_concurrentqueue_bench(threads, num_threads,read_messages,graph,q,root,foundPath,finished);

  auto duration = std::chrono::milliseconds(2000);

  IT written_messages = 0;

  std::thread sender_thread{[&]() {
    int u = 0;
    //while (u++<100000) {
    while (!finished.load()) {
      q.enqueue(written_messages++);
    }
  }};
  cpu_set_t my_set;
  CPU_ZERO(&my_set);
  CPU_SET(0, &my_set);
  if (sched_setaffinity(0, sizeof(cpu_set_t), &my_set)) {
    std::cout << "sched_setaffinity error: " << strerror(errno) << std::endl;
  }

  std::this_thread::sleep_for(duration);
  while(q.size_approx()){}

  finished.store(true);

  sender_thread.join();

  print_results(BenchResult{num_threads, written_messages, read_messages, duration});

  for (auto& t : threads) {
    t.join();
  }
  return;
}


template <typename IT, typename VT>
void Matcher::match_parallel(Graph<IT, VT>& graph) {
    moodycamel::ConcurrentQueue<IT> q;
    std::atomic<IT> root = 0;
    std::atomic<bool> finished = false;
    std::atomic<bool> foundPath = false;
    std::atomic<IT> activeThreads = 0;

    constexpr unsigned num_threads = 15;
    std::vector<std::thread> threads(num_threads);
    std::vector<size_t> read_messages;
    read_messages.resize(num_threads);
    create_threads_concurrentqueue(threads, num_threads,read_messages,graph,q,root,foundPath,activeThreads,finished);

    IT written_messages = 0;
    cpu_set_t my_set;
    CPU_ZERO(&my_set);
    CPU_SET(0, &my_set);
    if (sched_setaffinity(0, sizeof(cpu_set_t), &my_set)) {
        std::cout << "sched_setaffinity error: " << strerror(errno) << std::endl;
    }
    auto match_start = high_resolution_clock::now();
    // Access the graph elements as needed
    for (std::size_t i = 0; i < graph.getN(); ++i) {
        auto edgeCount = graph.indptr[i+1]-graph.indptr[i];
        if (graph.matching[i] < 0 && edgeCount) {
            root.store(i);
            foundPath.store(false);
            written_messages+=edgeCount;
            q.enqueue_bulk(graph.indices.cbegin()+graph.indptr[i],edgeCount);
            while(!foundPath.load() || activeThreads.load()){}
            // Once a path has been found, ensure q is empty for next iteration.
            IT pop;
            while(q.try_dequeue(pop)){}
            //std::cout << "Pushed " << i << "'s " << edgeCount << " edges" << std::endl;
        }
    }
    auto match_end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(match_end - match_start);

    while(q.size_approx()){}

    finished.store(true);

    print_results(BenchResult{num_threads, written_messages, read_messages, duration});

    for (auto& t : threads) {
        t.join();
    }
    return;
}


template <typename IT, typename VT>
void Matcher::match_parallel_one_atomic(Graph<IT, VT>& graph) {
    moodycamel::ConcurrentQueue<IT> q;
    std::atomic<IT> root = 0;
    volatile bool finished = false;
    volatile bool foundPath = false;
    std::atomic<IT> activeThreads = 0;

    constexpr unsigned num_threads = 15;
    std::vector<std::thread> threads(num_threads);
    std::vector<size_t> read_messages;
    read_messages.resize(num_threads);
    //create_threads_concurrentqueue_one_atomic(threads, num_threads,read_messages,graph,q,root,foundPath,activeThreads,finished);

    IT written_messages = 0;
    cpu_set_t my_set;
    CPU_ZERO(&my_set);
    CPU_SET(0, &my_set);
    if (sched_setaffinity(0, sizeof(cpu_set_t), &my_set)) {
        std::cout << "sched_setaffinity error: " << strerror(errno) << std::endl;
    }
    auto match_start = high_resolution_clock::now();
    // Access the graph elements as needed
    for (std::size_t i = 0; i < graph.getN(); ++i) {
        auto edgeCount = graph.indptr[i+1]-graph.indptr[i];
        if (graph.matching[i] < 0 && edgeCount) {
            root.store(i);
            foundPath=false;
            written_messages+=edgeCount;
            q.enqueue_bulk(graph.indices.cbegin()+graph.indptr[i],edgeCount);
            //while(!foundPath || activeThreads.load()){}
            // Once a path has been found, ensure q is empty for next iteration.
            IT pop;
            while(q.try_dequeue(pop)){}
            //std::cout << "Pushed " << i << "'s " << edgeCount << " edges" << std::endl;
        }
    }
    auto match_end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(match_end - match_start);

    while(q.size_approx()){}

    finished=true;

    /*
    print_results(BenchResult{num_threads, written_messages, read_messages, duration});

    for (auto& t : threads) {
        t.join();
    }
    */
    return;
}


template <typename IT, typename VT>
void Matcher::match_parallel_baseline(Graph<IT, VT>& graph) {

    moodycamel::ConcurrentQueue<IT> q;
    volatile bool finished = false;
    volatile bool foundPath = false;
    constexpr unsigned num_threads = 1;
    std::vector<std::thread> threads(num_threads);
    std::vector<size_t> read_messages;
    read_messages.resize(num_threads);
    auto match_start = high_resolution_clock::now();
    // Access the graph elements as needed
    create_threads_concurrentqueue_baseline(threads, num_threads,read_messages,graph,q,foundPath,finished);

    IT written_messages = 0;
    cpu_set_t my_set;
    CPU_ZERO(&my_set);
    CPU_SET(0, &my_set);
    if (sched_setaffinity(0, sizeof(cpu_set_t), &my_set)) {
        std::cout << "sched_setaffinity error: " << strerror(errno) << std::endl;
    }


    while(!finished){}
    auto match_end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(match_end - match_start);

    finished=true;

    
    print_results(BenchResult{num_threads, written_messages, read_messages, duration});

    for (auto& t : threads) {
        t.join();
    }
    
    return;
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

template <typename IT, typename VT>
void Matcher::search_persistent_bench(Graph<IT, VT>& graph,
                                    moodycamel::ConcurrentQueue<IT> &q,
                                    std::atomic<IT> &root,
                                    std::atomic<bool> &foundPath,
                                    std::atomic<bool> &finished,
                                    std::vector<size_t> &read_messages,
                                    int tid){
    while (!finished.load()) {
        IT edgeIndex;
        if(!q.try_dequeue(edgeIndex))
            continue;
        read_messages[tid-1]++;
    }
}


template <typename IT, typename VT>
void Matcher::search_persistent(Graph<IT, VT>& graph,
                                    moodycamel::ConcurrentQueue<IT> &q,
                                    std::atomic<IT> &root,
                                    std::atomic<bool> &foundPath,
                                    std::atomic<IT> &activeThreads,
                                    std::atomic<bool> &finished,
                                    std::vector<size_t> &read_messages,
                                    int tid){
    Vertex<IT> *FromBase,*ToBase, *nextVertex;
    IT FromBaseVertexID,ToBaseVertexID;
    IT stackEdge, matchedEdge;
    IT nextVertexIndex;

    Frontier<IT> f(graph.getN(),graph.getM());
    Stack<IT> &stack = f.stack;
    Stack<IT> &tree = f.tree;
    DisjointSetUnion<IT> &dsu = f.dsu;
    std::vector<Vertex<IT>> & vertexVector = f.vertexVector;

    while (!finished.load()) {
        IT E_index;
        if(!q.try_dequeue(E_index))
            continue;
        // Not exactly threadsafe, but pretty close.
        activeThreads++;
        IT V_index = root.load();
        
        IT time = 0;
        //auto inserted = vertexMap.try_emplace(V_index,Vertex<IT>(time++,Label::EvenLabel));
        nextVertex = &vertexVector[V_index];
        tree.push_back(V_index);
        nextVertex->AgeField=time++;
        stack.push_back(E_index);
        read_messages[tid-1]++;
        while(!foundPath.load()&&!stack.empty()){
            stack.pop_back();
        }
        foundPath.store(true);
        f.reinit();
        f.clear();
        activeThreads--;
    }
}


template <typename IT, typename VT>
void Matcher::search_persistent_one_atomic(Graph<IT, VT>& graph,
                                    moodycamel::ConcurrentQueue<IT> &q,
                                    std::atomic<IT> &root,
                                    volatile bool &foundPath,
                                    std::atomic<IT> &activeThreads,
                                    volatile bool &finished,
                                    std::vector<size_t> &read_messages,
                                    int tid){
    Vertex<IT> *FromBase,*ToBase, *nextVertex;
    IT FromBaseVertexID,ToBaseVertexID;
    IT stackEdge, matchedEdge;
    IT nextVertexIndex;

    Frontier<IT> f(graph.getN(),graph.getM());
    Stack<IT> &stack = f.stack;
    Stack<IT> &tree = f.tree;
    DisjointSetUnion<IT> &dsu = f.dsu;
    std::vector<Vertex<IT>> & vertexVector = f.vertexVector;

    while (!finished) {
        IT E_index;
        if(!q.try_dequeue(E_index))
            continue;
        // Not exactly threadsafe, but pretty close.
        activeThreads++;
        IT V_index = root.load();
        
        IT time = 0;
        //auto inserted = vertexMap.try_emplace(V_index,Vertex<IT>(time++,Label::EvenLabel));
        nextVertex = &vertexVector[V_index];
        tree.push_back(V_index);
        nextVertex->AgeField=time++;
        stack.push_back(E_index);
        read_messages[tid-1]++;
        while(!foundPath&&!stack.empty()){
            stack.pop_back();
        }
        foundPath=true;
        f.reinit();
        f.clear();
        activeThreads--;
    }
}


template <typename IT, typename VT>
void Matcher::search_persistent_baseline(Graph<IT, VT>& graph,
                                    moodycamel::ConcurrentQueue<IT> &q,
                                    std::vector<size_t> &read_messages,
                                    volatile bool &foundPath,
                                    volatile bool &finished,
                                    int tid){
    Vertex<IT> *FromBase,*ToBase, *nextVertex;
    IT FromBaseVertexID,ToBaseVertexID;
    IT stackEdge, matchedEdge;
    IT nextVertexIndex;

    Frontier<IT> f(graph.getN(),graph.getM());
    Stack<IT> &stack = f.stack;
    Stack<IT> &tree = f.tree;
    DisjointSetUnion<IT> &dsu = f.dsu;
    std::vector<Vertex<IT>> & vertexVector = f.vertexVector;
    for (std::size_t V_index = 0; V_index < graph.getN(); ++V_index) {
        if (graph.matching[V_index] < 0) {
            //read_messages[tid-1]++;
            IT time = 0;
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
                    // Safely kills other/this walker and allows for next iteration to begin
                    augment(graph,ToBase,f);
                    f.reinit();
                    break;
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
            // Safely kills walkers and allows for next iteration to begin
            f.clear();
        }
    }
    finished = true;
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


template <typename IT, typename VT>
void Matcher::create_threads_concurrentqueue_bench(std::vector<std::thread> &threads, 
                                    unsigned num_threads,
                                    std::vector<size_t> & read_messages,
                                    Graph<IT, VT>& graph, 
                                    moodycamel::ConcurrentQueue<IT> &q,
                                    std::atomic<IT> &root,
                                    std::atomic<bool> &foundPath,
                                    std::atomic<bool> &finished){

  for (unsigned i = 1; i < num_threads+1; ++i) {
    threads[i-1] = std::thread(&Matcher::search_persistent_bench<IT,VT>,
                                std::ref(graph), 
                                std::ref(q),
                                std::ref(root),
                                std::ref(foundPath),
                                std::ref(finished),
                                std::ref(read_messages),
                                i);

    // Create a cpu_set_t object representing a set of CPUs. Clear it and mark
    // only CPU i as set.
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(i, &cpuset);
    int rc = pthread_setaffinity_np(threads[i-1].native_handle(),
                                    sizeof(cpu_set_t), &cpuset);
    if (rc != 0) {
      std::cerr << "Error calling pthread_setaffinity_np: " << rc << "\n";
    }
  }
}


template <typename IT, typename VT>
void Matcher::create_threads_concurrentqueue(std::vector<std::thread> &threads, 
                                    unsigned num_threads,
                                    std::vector<size_t> & read_messages,
                                    Graph<IT, VT>& graph, 
                                    moodycamel::ConcurrentQueue<IT> &q,
                                    std::atomic<IT> &root,
                                    std::atomic<bool> &foundPath,
                                    std::atomic<IT> &activeThreads,
                                    std::atomic<bool> &finished){

  for (unsigned i = 1; i < num_threads+1; ++i) {
    threads[i-1] = std::thread(&Matcher::search_persistent<IT,VT>,
                                std::ref(graph), 
                                std::ref(q),
                                std::ref(root),
                                std::ref(foundPath),
                                std::ref(activeThreads),
                                std::ref(finished),
                                std::ref(read_messages),
                                i);

    // Create a cpu_set_t object representing a set of CPUs. Clear it and mark
    // only CPU i as set.
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(i, &cpuset);
    int rc = pthread_setaffinity_np(threads[i-1].native_handle(),
                                    sizeof(cpu_set_t), &cpuset);
    if (rc != 0) {
      std::cerr << "Error calling pthread_setaffinity_np: " << rc << "\n";
    }
  }
}


template <typename IT, typename VT>
void Matcher::create_threads_concurrentqueue_one_atomic(std::vector<std::thread> &threads, 
                                    unsigned num_threads,
                                    std::vector<size_t> & read_messages,
                                    Graph<IT, VT>& graph, 
                                    moodycamel::ConcurrentQueue<IT> &q,
                                    std::atomic<IT> &root,
                                    volatile bool &foundPath,
                                    std::atomic<IT> &activeThreads,
                                    volatile bool &finished){

  for (unsigned i = 1; i < num_threads+1; ++i) {
    threads[i-1] = std::thread(&Matcher::search_persistent_one_atomic<IT,VT>,
                                std::ref(graph), 
                                std::ref(q),
                                std::ref(root),
                                std::ref(foundPath),
                                std::ref(activeThreads),
                                std::ref(finished),
                                std::ref(read_messages),
                                i);

    // Create a cpu_set_t object representing a set of CPUs. Clear it and mark
    // only CPU i as set.
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(i, &cpuset);
    int rc = pthread_setaffinity_np(threads[i-1].native_handle(),
                                    sizeof(cpu_set_t), &cpuset);
    if (rc != 0) {
      std::cerr << "Error calling pthread_setaffinity_np: " << rc << "\n";
    }
  }
}



template <typename IT, typename VT>
void Matcher::create_threads_concurrentqueue_baseline(std::vector<std::thread> &threads, 
                                    unsigned num_threads,
                                    std::vector<size_t> & read_messages,
                                    Graph<IT, VT>& graph, 
                                    moodycamel::ConcurrentQueue<IT> &q,
                                    volatile bool &foundPath,
                                    volatile bool &finished){

  for (unsigned i = 1; i < num_threads+1; ++i) {
    threads[i-1] = std::thread(&Matcher::search_persistent_baseline<IT,VT>,
                                std::ref(graph), 
                                std::ref(q),
                                std::ref(read_messages),
                                std::ref(foundPath),
                                std::ref(finished),
                                i);

    // Create a cpu_set_t object representing a set of CPUs. Clear it and mark
    // only CPU i as set.
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(i, &cpuset);
    int rc = pthread_setaffinity_np(threads[i-1].native_handle(),
                                    sizeof(cpu_set_t), &cpuset);
    if (rc != 0) {
      std::cerr << "Error calling pthread_setaffinity_np: " << rc << "\n";
    }
  }
}



#endif
