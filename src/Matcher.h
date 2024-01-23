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
#include <limits>

class Matcher {
public:
    template <typename IT, typename VT>
    static void match(Graph<IT, VT>& graph);
    template <typename IT, typename VT>
    static void match(Graph<IT, VT>& graph, Statistics<IT>& stats);
    template <typename IT, typename VT>
    static void match_wl(Graph<IT, VT>& graph, 
                        int num_threads,
                        int deferral_threshold);
    template <typename IT, typename VT>
    static void match_persistent_wl(Graph<IT, VT> &graph,
                                    moodycamel::ConcurrentQueue<IT> &worklist,
                                    bool &finished);
    template <typename IT, typename VT>
    static void match_persistent_wl2(Graph<IT, VT> &graph,
                                    std::vector<moodycamel::ConcurrentQueue<IT, moodycamel::ConcurrentQueueDefaultTraits>> &worklists,
                                    moodycamel::ConcurrentQueue<IT> &worklist,
                                    std::vector<size_t> &read_messages,
                                    std::atomic<bool>& found_augmenting_path,
                                    std::atomic<IT> & currentRoot,
                                    std::vector<std::mutex> &worklistMutexes,
                                    std::vector<std::condition_variable> &worklistCVs,
                                    int tid,
                                    std::atomic<IT> & num_enqueued,
                                    std::atomic<IT> & num_dequeued,
                                    std::atomic<IT> & num_augmentations);

template <typename IT, typename VT>
static void match_persistent_wl3(Graph<IT, VT>& graph,
                                std::vector<moodycamel::ConcurrentQueue<Frontier<IT>, moodycamel::ConcurrentQueueDefaultTraits>> &worklists,
                                moodycamel::ConcurrentQueue<std::vector<IT>> &pathQueue,
                                std::atomic<IT> &masterTID,
                                std::vector<size_t> &read_messages,
                                std::atomic<bool>& found_augmenting_path,
                                std::atomic<IT> & currentRoot,
                                std::vector<std::mutex> &worklistMutexes,
                                std::vector<std::condition_variable> &worklistCVs,
                                int tid,
                                std::atomic<IT> & num_enqueued,
                                std::atomic<IT> & num_dequeued,
                                std::atomic<IT> & num_augmentations,
                                int deferral_threshold);

private:
    template <typename IT, typename VT>
    static void search(Graph<IT, VT>& graph, 
                    const size_t V_index,
                    Frontier<IT> & f,
                    std::vector<Vertex<IT>> & vertexVector);
    template <typename IT, typename VT>
    static bool start_search(Graph<IT, VT>& graph, 
                    const size_t V_index,
                    Frontier<IT> & f,
                    std::vector<Vertex<IT>> & vertexVector,
                    std::atomic<IT> & num_enqueued,
                    std::vector<moodycamel::ConcurrentQueue<Frontier<IT>, moodycamel::ConcurrentQueueDefaultTraits>> &worklists,
                    std::atomic<bool>& found_augmenting_path,
                    std::atomic<IT> &masterTID,
                    int deferral_threshold);
    template <typename IT, typename VT>
    static void continue_search(Graph<IT, VT>& graph, 
                    Frontier<IT> & f,
                    std::vector<Vertex<IT>> & vertexVector,
                    std::atomic<IT> & num_enqueued,
                    std::vector<moodycamel::ConcurrentQueue<Frontier<IT>, moodycamel::ConcurrentQueueDefaultTraits>> &worklists,
                    std::atomic<bool>& found_augmenting_path,
                    std::atomic<IT> &masterTID,
                    std::atomic<IT> &num_augmentations);
    template <typename IT, typename VT>
    static void next_iteration(Graph<IT, VT>& graph, 
                    std::atomic<IT> & currentRoot,
                    std::atomic<IT> & num_enqueued,  
                    int tid,                  
                    //moodycamel::ConcurrentQueue<IT> &worklist,
                    std::vector<moodycamel::ConcurrentQueue<IT, moodycamel::ConcurrentQueueDefaultTraits>> &worklists);
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
                    std::vector<Vertex<IT>> & vertexVector,
                    std::vector<IT> & path);
    template <typename IT, typename VT>
    static void pathThroughBlossom(Graph<IT, VT>& graph, 
                        // V
                        const Vertex<IT> * TailOfAugmentingPath,
                        const Vertex<IT> * TailOfAugmentingPathBase,
                        std::vector<Vertex<IT>> & vertexVector,
                        //std::list<IT> & path,
                        std::vector<IT> & path);
};

template <typename IT, typename VT>
void Matcher::match(Graph<IT, VT>& graph) {

    std::vector<Vertex<IT>> vertexVector;
    auto allocate_start = high_resolution_clock::now();
    vertexVector.reserve(graph.getN());
    std::iota(vertexVector.begin(), vertexVector.begin()+graph.getN(), 0);
    auto allocate_end = high_resolution_clock::now();
    auto duration_alloc = duration_cast<milliseconds>(allocate_end - allocate_start);
    std::cout << "Vertex Vector (9|V|) memory allocation time: "<< duration_alloc.count() << " milliseconds" << '\n';
    Vertex<IT> * TailOfAugmentingPath;
    Frontier<IT> f;
    std::vector<IT> path;
    // Access the graph elements as needed
    const size_t N = graph.getN();
    auto search_start = high_resolution_clock::now();
    for (std::size_t i = 0; i < N; ++i) {
        if (!graph.IsMatched(i)) {
            //printf("SEARCHING FROM %ld!\n",i);
            // Your matching logic goes here...
            search(graph,i,f,vertexVector);
            // If not -1, I found an AP.
            if (f.TailOfAugmentingPathVertexIndex!=-1){
                    TailOfAugmentingPath=&vertexVector[f.TailOfAugmentingPathVertexIndex];
                    augment(graph,TailOfAugmentingPath,vertexVector,path);
                    f.reinit(vertexVector);
                    path.clear();
                    f.clear();
                //printf("FOUND AP!\n");
            } else {
                f.clear();
                //printf("DIDNT FOUND AP!\n");
            }
        }
    }
    auto search_end = high_resolution_clock::now();
    auto duration_search = duration_cast<seconds>(search_end - search_start);
    std::cout << "Algorithm execution time: "<< duration_search.count() << " seconds" << '\n';
}


template <typename IT, typename VT>
void Matcher::match(Graph<IT, VT>& graph, Statistics<IT>& stats) {

        std::vector<Vertex<IT>> vertexVector;
        auto allocate_start = high_resolution_clock::now();
        vertexVector.reserve(graph.getN());
        std::iota(vertexVector.begin(), vertexVector.begin()+graph.getN(), 0);
        auto allocate_end = high_resolution_clock::now();
        auto duration_alloc = duration_cast<milliseconds>(allocate_end - allocate_start);
        std::cout << "Vertex Vector (9|V|) memory allocation time: "<< duration_alloc.count() << " milliseconds" << '\n';
        Vertex<IT> * TailOfAugmentingPath;
        Frontier<IT> f;
        std::vector<IT> path;
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
                augment(graph,TailOfAugmentingPath,vertexVector,path);
                stats.write_entry(path.size() ? (2*path.size()-1):0,f.tree.size(),duration_cast<microseconds>(search_end - search_start));
                f.reinit(vertexVector);
                path.clear();
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
                        int num_threads,
                        int deferral_threshold) {
    auto mt_thread_coordination_start = high_resolution_clock::now();
    size_t capacity = 1;
    moodycamel::ConcurrentQueue<std::vector<IT>> pathQueue{capacity};
    std::vector<moodycamel::ConcurrentQueue<Frontier<IT>, moodycamel::ConcurrentQueueDefaultTraits>> worklists;
    worklists.reserve(num_threads);
    for (int i = 0; i < num_threads; ++i) {
        // Initialize each queue with the desired parameters
        worklists.emplace_back(moodycamel::ConcurrentQueue<Frontier<IT>>{capacity});
    }
    
    std::vector<std::mutex> worklistMutexes(num_threads);
    std::vector<std::condition_variable> worklistCVs(num_threads);

    std::atomic<IT> num_enqueued(0);
    std::atomic<IT> num_dequeued(0);
    std::atomic<IT> num_augmentations(0);
    std::atomic<IT> currentRoot(-1);
    std::atomic<IT> masterTID(-1);
    std::atomic<bool> found_augmenting_path(false);
    
    //std::vector<std::atomic<bool>> atomicBoolVector(num_threads);
    std::vector<std::thread> workers(num_threads);
    std::vector<size_t> read_messages;
    read_messages.resize(num_threads);
    auto mt_thread_coordination_end = high_resolution_clock::now();
    auto durationmt = duration_cast<microseconds>(mt_thread_coordination_end - mt_thread_coordination_start);
    std::cout << "Worklist and atomic variable allocation time: "<< durationmt.count() << " microseconds" << '\n';



    //spinning.resize(num_threads,false);
    // Access the graph elements as needed
    ThreadFactory::create_threads_concurrentqueue_wl<IT,VT>(workers, num_threads,read_messages,
    worklists,pathQueue,masterTID,graph,
    currentRoot,found_augmenting_path,
    worklistMutexes,worklistCVs,num_enqueued,num_dequeued,num_augmentations,deferral_threshold);

    auto join_thread_start = high_resolution_clock::now();
    for (auto& t : workers) {
        t.join();
    }
    auto join_thread_end = high_resolution_clock::now();
    auto duration = duration_cast<seconds>(join_thread_end - join_thread_start);
    std::cout << "Thread joining time: "<< duration.count() << " seconds" << '\n';

    std::cout << "NUM ENQUEUED " << num_enqueued.load() << '\n';
    std::cout << "NUM DEQUEUED " << num_dequeued.load() << '\n';
    std::cout << "NUM AUGMENTATIONS " << num_augmentations.load() << '\n';

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

        std::vector<Vertex<IT>> vertexVector;
        auto allocate_start = high_resolution_clock::now();
        vertexVector.reserve(graph.getN());
        std::iota(vertexVector.begin(), vertexVector.begin()+graph.getN(), 0);
        auto allocate_end = high_resolution_clock::now();
        auto duration_alloc = duration_cast<milliseconds>(allocate_end - allocate_start);
        std::cout << "Vertex Vector (9|V|) memory allocation time: "<< duration_alloc.count() << " milliseconds" << '\n';
        Vertex<IT> * TailOfAugmentingPath;
        Frontier<IT> f;
        std::vector<IT> path;
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
                augment(graph,TailOfAugmentingPath,vertexVector,path);
                f.reinit(vertexVector);
                path.clear();
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
void Matcher::match_persistent_wl3(Graph<IT, VT>& graph,
                                std::vector<moodycamel::ConcurrentQueue<Frontier<IT>, moodycamel::ConcurrentQueueDefaultTraits>> &worklists,
                                moodycamel::ConcurrentQueue<std::vector<IT>> &pathQueue,
                                std::atomic<IT> &masterTID,
                                std::vector<size_t> &read_messages,
                                std::atomic<bool>& found_augmenting_path,
                                std::atomic<IT> & currentRoot,
                                std::vector<std::mutex> &worklistMutexes,
                                std::vector<std::condition_variable> &worklistCVs,
                                int tid,
                                std::atomic<IT> & num_enqueued,
                                std::atomic<IT> & num_dequeued,
                                std::atomic<IT> & num_augmentations,
                                int deferral_threshold) {
    const size_t N = graph.getN();
    const size_t nworkers = worklists.size();
    IT expected = -1;
    IT desired = 0;
    std::vector<Vertex<IT>> vertexVector;
    // first thread to reach here claims master status.
    auto thread_match_start = high_resolution_clock::now();
    if (currentRoot.compare_exchange_strong(expected, desired)) {
        masterTID.store(tid);
        auto allocate_start = high_resolution_clock::now();
        vertexVector.reserve(graph.getN());
        std::iota(vertexVector.begin(), vertexVector.begin()+graph.getN(), 0);
        auto allocate_end = high_resolution_clock::now();
        auto duration_alloc = duration_cast<milliseconds>(allocate_end - allocate_start);
        std::cout << "TID(" << tid << ") Vertex Vector (9|V|) memory allocation time: "<< duration_alloc.count() << " milliseconds" << '\n';
        Vertex<IT> * TailOfAugmentingPath;
        Frontier<IT> f;
        std::vector<IT> path;
        //const size_t N = 5;
        int numDeferred = 0;
        auto search_start = high_resolution_clock::now();
        for (; currentRoot < N; ++currentRoot) {
            if (!graph.IsMatched(currentRoot)) {
                // If I didn't finish, give the state onto another worker.
                if(!start_search(graph,currentRoot,f,vertexVector,num_enqueued,worklists,found_augmenting_path,masterTID,deferral_threshold))
                {
                    numDeferred++;
                    int workerID = tid;
                    int minWork = std::numeric_limits<int>::max();
                    int minWorkID = tid;

                    do {
                        workerID++;
                        if(worklists[workerID%nworkers].size_approx()<minWork){
                            minWorkID=workerID%nworkers;
                            minWork=worklists[workerID%nworkers].size_approx();
                            if (minWork == 0) break;
                        }
                    } while (workerID%nworkers != tid);
                    worklists[minWorkID].enqueue(f);
                    f.reinit(vertexVector);
                    f.clear();
                }
                if (f.TailOfAugmentingPathVertexIndex!=-1){
                    TailOfAugmentingPath=&vertexVector[f.TailOfAugmentingPathVertexIndex];
                    augment(graph,TailOfAugmentingPath,vertexVector,path);
                    num_augmentations++;
                    f.reinit(vertexVector);
                    path.clear();
                    f.clear();
                } else {
                    f.clear();
                }
            }
        }
        auto search_end = high_resolution_clock::now();
        auto duration_search = duration_cast<seconds>(search_end - search_start);
        std::cout << "Thread "<< tid << " algorithm execution time: "<< duration_search.count() << " seconds" << '\n';
        std::cout << "Thread "<< tid << " number deferred: "<< numDeferred << '\n';

        for (auto & cv:worklistCVs)
            cv.notify_one();
    }
    while(worklists[tid].size_approx() || currentRoot.load(std::memory_order_relaxed)!=N){
        std::unique_lock<std::mutex> lock(worklistMutexes[tid]);
        // If the worklist is empty (size_approx == 0), wait for a signal
        // If the algorithm is finished (CR==N), return
        worklistCVs[tid].wait(lock, [&] { return worklists[tid].size_approx() || currentRoot.load(std::memory_order_relaxed)==N; });
        Frontier<IT> f;
        while(worklists[tid].try_dequeue(f)){
            read_messages[tid]++;
            // Lazy allocation of vv when thread starts working.
            if(vertexVector.capacity()==0){
                auto allocate_start = high_resolution_clock::now();
                vertexVector.reserve(graph.getN());
                std::iota(vertexVector.begin(), vertexVector.begin()+graph.getN(), 0);
                auto allocate_end = high_resolution_clock::now();
                auto duration_alloc = duration_cast<milliseconds>(allocate_end - allocate_start);
                std::cout << "TID(" << tid << ") Vertex Vector (9|V|) memory allocation time: "<< duration_alloc.count() << " milliseconds" << '\n';

            }
            continue_search(graph,f,vertexVector,num_enqueued,worklists,found_augmenting_path,masterTID,num_augmentations);
            if (f.TailOfAugmentingPathVertexIndex==-1){
                IT b4, af;
                bool still_valid;
                do{
                    b4 = num_augmentations.load();
                    still_valid = f.verifyTree(graph.matching,vertexVector);
                    af = num_augmentations.load();
                }while(b4!=af && still_valid);
                if(still_valid){
                    // You can kill all these vertices :)
                } else {
                    // Need to restart search.

                }
            }
            f.reinit(vertexVector);
            f.clear();
            // If this thread found an AP, send it to master.
            num_dequeued++;
        }
    }
}


template <typename IT, typename VT>
void Matcher::match_persistent_wl2(Graph<IT, VT>& graph,
                                std::vector<moodycamel::ConcurrentQueue<IT, moodycamel::ConcurrentQueueDefaultTraits>> &worklists,
                                moodycamel::ConcurrentQueue<IT> &worklist,
                                std::vector<size_t> &read_messages,
                                std::atomic<bool>& found_augmenting_path,
                                std::atomic<IT> & currentRoot,
                                std::vector<std::mutex> &worklistMutexes,
                                std::vector<std::condition_variable> &worklistCVs,
                                int tid,
                                std::atomic<IT> & num_enqueued,
                                std::atomic<IT> & num_dequeued,
                                std::atomic<IT> & num_augmentations) {


        std::vector<Vertex<IT>> vertexVector;
        auto allocate_start = high_resolution_clock::now();
        vertexVector.reserve(graph.getN());
        std::iota(vertexVector.begin(), vertexVector.begin()+graph.getN(), 0);
        auto allocate_end = high_resolution_clock::now();
        auto duration_alloc = duration_cast<milliseconds>(allocate_end - allocate_start);
        std::cout << "Vertex Vector (9|V|) memory allocation time: "<< duration_alloc.count() << " milliseconds" << '\n';
        Vertex<IT> * TailOfAugmentingPath;
        Frontier<IT> f;
        std::vector<IT> path;

    const size_t N = graph.getN();
    //const size_t N = 5;
    IT V_index;

    IT expected = -2;
    IT desired = -1;
    // First to encounter this code will see currentRoot == -2,
    // it will be atomically exchanged with -1, and return true.
    // All others will modify expected, inconsequentially,
    // and enter the while loop.
    // The worker thread has done the work,
    // Notify the master thread to continue the work.
    auto thread_match_start = high_resolution_clock::now();
    if (currentRoot.compare_exchange_strong(expected, desired)) {
        next_iteration(graph,currentRoot,num_enqueued,tid,worklists);
    }
    // finished_algorithm when currentRoot == N
    while(currentRoot.load(std::memory_order_relaxed)!=N){
        if (worklists[tid].try_dequeue(V_index)){
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

            //num_augmentations++;
            if (1+num_augmentations.fetch_add(1) == num_dequeued.load() &&
                num_dequeued.load() == num_enqueued.load()) {
                found_augmenting_path.store(false);
                next_iteration(graph,currentRoot,num_enqueued,tid,worklists);
            }

            f.reinit();
            f.clear();

        } else {
            // If the worklist is empty, wait for a signal
            std::unique_lock<std::mutex> lock(worklistMutexes[tid]);
            worklistCVs[tid].wait(lock, [&] { return worklists[tid].size_approx() || currentRoot.load(std::memory_order_relaxed)==N; });
        }
    }
    auto thread_match_end = high_resolution_clock::now();
    auto duration = duration_cast<seconds>(thread_match_end - thread_match_start);
    std::cout << "Thread "<< tid << " algorithm execution time: "<< duration.count() << " seconds" << '\n';

    for (auto & cv:worklistCVs)
        cv.notify_one();

}

template <typename IT, typename VT>
void Matcher::next_iteration(Graph<IT, VT>& graph, 
                    std::atomic<IT> & currentRoot,
                    std::atomic<IT> & num_enqueued,
                    //moodycamel::ConcurrentQueue<IT> &worklist,
                    int tid,
                    std::vector<moodycamel::ConcurrentQueue<IT, moodycamel::ConcurrentQueueDefaultTraits>> &worklists
                    ){
    while(++currentRoot < graph.getN()){
        if (!graph.IsMatched(currentRoot)) {
            //printf("Enqueuing %d\n",i);
            num_enqueued++;
            worklists[tid].enqueue(currentRoot);
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
    std::vector<IT> &stack = f.stack;
    std::vector<Vertex<IT>> &tree = f.tree;
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
void Matcher::search(Graph<IT, VT>& graph, 
                    const size_t V_index,
                    Frontier<IT> & f,
                    std::vector<Vertex<IT>> & vertexVector) {
    Vertex<IT> *FromBase,*ToBase, *nextVertex;
    IT FromBaseVertexID,ToBaseVertexID;
    IT stackEdge, matchedEdge;
    IT nextVertexIndex;
    IT &time = f.time;
    std::vector<IT> &stack = f.stack;
    std::vector<Vertex<IT>> &tree = f.tree;
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
        FromBaseVertexID = DisjointSetUnionHelper<IT>::getBase(Graph<IT,VT>::EdgeFrom(graph,stackEdge),vertexVector);  
        FromBase = &vertexVector[FromBaseVertexID];

        // Necessary because vertices dont know their own index.
        // It simplifies vector creation..
        ToBaseVertexID = DisjointSetUnionHelper<IT>::getBase(Graph<IT,VT>::EdgeTo(graph,stackEdge),vertexVector);  

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
            f.TailOfAugmentingPathVertexIndex=ToBase->LabelField;
            return;
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
            Blossom::Shrink(graph,stackEdge,vertexVector,stack);
        }
    }
    return;
}

// Returns true if a decision was made.
template <typename IT, typename VT>
bool Matcher::start_search(Graph<IT, VT>& graph, 
                    const size_t V_index,
                    Frontier<IT> & f,
                    std::vector<Vertex<IT>> & vertexVector,
                    std::atomic<IT> & num_enqueued,
                    std::vector<moodycamel::ConcurrentQueue<Frontier<IT>, moodycamel::ConcurrentQueueDefaultTraits>> &worklists,
                    std::atomic<bool>& found_augmenting_path,
                    std::atomic<IT> &masterTID,
                    int deferral_threshold) {
    Vertex<IT> *FromBase,*ToBase, *nextVertex;
    IT FromBaseVertexID,ToBaseVertexID;
    IT stackEdge, matchedEdge;
    IT nextVertexIndex;
    IT &time = f.time;
    std::vector<IT> &stack = f.stack;
    std::vector<Vertex<IT>> &tree = f.tree;
    nextVertex = &vertexVector[V_index];
    nextVertex->AgeField=time++;
    tree.push_back(*nextVertex);

    // Push edges onto stack, breaking if that stackEdge is a solution.
    Graph<IT,VT>::pushEdgesOntoStack(graph,vertexVector,V_index,stack);
    //while(!stack.empty() && !found_augmenting_path.load(std::memory_order_relaxed)){
    while(!stack.empty()){
        stackEdge = stack.back();
        stack.pop_back();
        // Necessary because vertices dont know their own index.
        // It simplifies vector creation..
        FromBaseVertexID = DisjointSetUnionHelper<IT>::getBase(Graph<IT,VT>::EdgeFrom(graph,stackEdge),vertexVector);  
        FromBase = &vertexVector[FromBaseVertexID];

        // Necessary because vertices dont know their own index.
        // It simplifies vector creation..
        ToBaseVertexID = DisjointSetUnionHelper<IT>::getBase(Graph<IT,VT>::EdgeTo(graph,stackEdge),vertexVector);  
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
            f.TailOfAugmentingPathVertexIndex=ToBase->LabelField;
            return true;
        } else if (!ToBase->IsReached() && graph.IsMatched(ToBaseVertexID)){
            ToBase->TreeField=stackEdge;
            ToBase->AgeField=time++;
            tree.push_back(*ToBase);

            matchedEdge=graph.GetMatchField(ToBaseVertexID);
            nextVertexIndex = Graph<IT,VT>::Other(graph,matchedEdge,ToBaseVertexID);
            nextVertex = &vertexVector[nextVertexIndex];
            nextVertex->AgeField=time++;
            // For safe concurrent blossom contraction.
            nextVertex->MatchField = matchedEdge;
            // For safe concurrent blossom contraction.

            tree.push_back(*nextVertex);

            Graph<IT,VT>::pushEdgesOntoStack(graph,vertexVector,nextVertexIndex,stack,matchedEdge);

        } else if (ToBase->IsEven()) {
            // Shrink Blossoms
            Blossom::Shrink(graph,stackEdge,vertexVector,stack);
        }
        
        if (stack.size()>deferral_threshold){
            return false;
        }
    }
    return true;
}

template <typename IT, typename VT>
void Matcher::continue_search(Graph<IT, VT>& graph, 
                    Frontier<IT> & f,
                    std::vector<Vertex<IT>> & vertexVector,
                    std::atomic<IT> & num_enqueued,
                    std::vector<moodycamel::ConcurrentQueue<Frontier<IT>, moodycamel::ConcurrentQueueDefaultTraits>> &worklists,
                    std::atomic<bool>& found_augmenting_path,
                    std::atomic<IT> &masterTID,
                    std::atomic<IT> &num_augmentations) {
    Vertex<IT> *FromBase,*ToBase, *nextVertex;
    IT FromBaseVertexID,ToBaseVertexID;
    IT stackEdge, matchedEdge;
    IT nextVertexIndex;
    IT &time = f.time;
    std::vector<IT> &stack = f.stack;
    std::vector<Vertex<IT>> &tree = f.tree;
    while(!stack.empty() && !found_augmenting_path.load(std::memory_order_release)){
        stackEdge = stack.back();
        stack.pop_back();
        // Necessary because vertices dont know their own index.
        // It simplifies vector creation..

        FromBaseVertexID = DisjointSetUnionHelper<IT>::getBase(Graph<IT,VT>::EdgeFrom(graph,stackEdge),vertexVector);  
        FromBase = &vertexVector[FromBaseVertexID];

        // Necessary because vertices dont know their own index.
        // It simplifies vector creation..
        ToBaseVertexID = DisjointSetUnionHelper<IT>::getBase(Graph<IT,VT>::EdgeTo(graph,stackEdge),vertexVector);  
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
            f.TailOfAugmentingPathVertexIndex=ToBase->LabelField;
            return;
        } else if (!ToBase->IsReached() && graph.IsMatched(ToBaseVertexID)){
            ToBase->TreeField=stackEdge;
            ToBase->AgeField=time++;
            tree.push_back(*ToBase);

            matchedEdge=graph.GetMatchField(ToBaseVertexID);
            nextVertexIndex = Graph<IT,VT>::Other(graph,matchedEdge,ToBaseVertexID);
            nextVertex = &vertexVector[nextVertexIndex];
            nextVertex->AgeField=time++;
            // For safe concurrent blossom contraction.
            nextVertex->MatchField = matchedEdge;
            // For safe concurrent blossom contraction.

            tree.push_back(*nextVertex);

            Graph<IT,VT>::pushEdgesOntoStack(graph,vertexVector,nextVertexIndex,stack,matchedEdge);

        } else if (ToBase->IsEven()) {
            // Shrink Blossoms
            Blossom::Shrink(graph,stackEdge,vertexVector,stack);
        }
    }
    return;
}

template <typename IT, typename VT>
void Matcher::augment(Graph<IT, VT>& graph, 
                    Vertex<IT> * TailOfAugmentingPath,
                    std::vector<Vertex<IT>> & vertexVector,
                    std::vector<IT> & path) {

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
        auto nextVertexBaseID = DisjointSetUnionHelper<IT>::getBase(nextVertexID,vertexVector);  

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
                    std::vector<IT> & path) {
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
