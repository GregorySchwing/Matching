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
#include "SPMCQueue.h"
#include "SPMC_Config.h"

class Matcher {
public:
    template <typename IT, typename VT>
    static void match(Graph<IT, VT>& graph);
    template <typename IT, typename VT>
    static void match_parallel(Graph<IT, VT>& graph);
    template <typename IT, typename VT>
    static void match(Graph<IT, VT>& graph, Statistics<IT>& stats);

private:
    template <typename IT, typename VT>
    static Vertex<IT> * search(Graph<IT, VT>& graph, 
                    const size_t V_index,
                    Frontier<IT> & f);
    template <typename IT, typename VT, typename Q>
    static void search_slave(Graph<IT, VT>& graph, 
                        std::atomic<bool>& atomic_flag, // Make flag atomic
                        std::atomic<IT>& atomic_numActive, // Make numActive atomic
                        Q& q,
                        int tid, int cpu); // For record-keeping and CPU binding
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
void Matcher::match_parallel(Graph<IT, VT>& graph) {

    using Q = SPMCQueue<Msg, 1024>;
    Q q;
    std::atomic<bool> atomic_flag = true; // Make flag atomic
    std::atomic<IT> atomic_numActive = 0; // Make numActive atomic

    int reader_cnt = 2;
    int cpu_start = 0;
    std::vector<thread> reader_thrs;
    for (int i = 0; i < reader_cnt; i++) {
        //reader_thrs.emplace_back(read_thread, i, cpu_start < 0 ? -1 : cpu_start + i, std::ref(q));
        reader_thrs.emplace_back(std::ref(Matcher::search_slave<IT, VT, Q>), std::ref(graph), std::ref(atomic_flag), std::ref(atomic_numActive), std::ref(q), i, cpu_start + i);

    }


    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    if (cpu_start >= 0) {
        cpupin(cpu_start + reader_cnt);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    std::cout << "WR THIS :" << &q << std::endl;

    // Access the graph elements as needed
    for (std::size_t i = 0; i < graph.getN(); ++i) {
        if (graph.matching[i] < 0) {
            IT V_ndx = i;
            // Push edges onto stack, breaking if that stackEdge is a solution.
            for (IT start = graph.indptr[V_ndx]; start < graph.indptr[V_ndx + 1]; ++start) {
                // For blossom contraction, need to skip repushing the matched & tree edges
                IT E_ndx = graph.indices[start];
                
                q.write([V_ndx,E_ndx](Msg& msg) {
                    msg.V_ndx = V_ndx;
                    msg.E_ndx = E_ndx;
                    msg.tsc = rdtsc();
                });
                
                std::cout << "Writing V_ndx: "<<V_ndx<<"; E_ndx: " << E_ndx << std::endl;
                IT nextVertexIndex = Graph<IT, VT>::Other(graph, graph.indices[start], V_ndx);
                if (!graph.IsMatched(nextVertexIndex))
                    break;
            }
            while(true){}

        }
    }

    for (auto& thr : reader_thrs) {
        thr.join();
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


template <typename IT, typename VT, typename Q>
void Matcher::search_slave(Graph<IT, VT>& graph, 
                    std::atomic<bool>& atomic_flag, // Make flag atomic
                    std::atomic<IT>& atomic_numActive, // Make numActive atomic
                    Q& q,
                    int tid, int cpu) {
    if (cpu >= 0) {
        cpupin(cpu);
    }
    auto reader = q.getReader();
    Frontier<IT> f(graph.getN(),graph.getM());
    Vertex<int64_t> *FromBase,*ToBase, *nextVertex;
    int64_t FromBaseVertexID,ToBaseVertexID;
    IT stackEdge, matchedEdge;
    IT nextVertexIndex;
    Stack<IT> &stack = f.stack;
    Stack<IT> &tree = f.tree;
    DisjointSetUnion<IT> &dsu = f.dsu;
    std::vector<Vertex<IT>> & vertexVector = f.vertexVector;
    while(true){
        Msg* msg = reader.read();
        if (!msg){
            //std::cout << "Thread " << tid << " failed to read!\n" << std::endl;
            continue;
        } else {
            std::cout << "Thread " << tid << " read!\n" << std::endl;
        }
        atomic_numActive++;
        IT V_index;// = msg->V_ndx;
        IT E_index;// = msg->E_ndx;
        nextVertex = &vertexVector[V_index];
        tree.push_back(V_index);
        IT time = 0;
        nextVertex->AgeField=time++;
        // Push edges onto stack, breaking if that stackEdge is a solution.
        stack.push_back(E_index);
        while(atomic_flag.load() && !stack.empty()){
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
                //return ToBase;
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
    }
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
