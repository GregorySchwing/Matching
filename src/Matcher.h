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
#include <thread>
class Matcher {
public:
    template <typename IT, typename VT>
    static void match(Graph<IT, VT>& graph);
private:
    template <typename IT, typename VT>
    static Vertex<IT> * search(Graph<IT, VT>& graph, 
                    Stack<IT> &stack,
                    Stack<IT> &tree,
                     DisjointSetUnion<IT> &dsu,
                    std::vector<Vertex<IT>> & vertexVector,
                    IT time);

    template <typename IT, typename VT>
    Vertex<IT> * Matcher::searchForkable(Graph<IT, VT>& graph, 
                        Stack<IT> &stack,
                        Stack<IT> &tree,
                        DisjointSetUnion<IT> &dsu,
                        std::vector<Vertex<IT>> & vertexVector,
                        Stack<IT> &stack2,
                        Stack<IT> &tree2,
                        DisjointSetUnion<IT> &dsu2,
                        std::vector<Vertex<IT>> & vertexVector2,
                        IT time);

    template <typename IT, typename VT>
    static void augment(Graph<IT, VT>& graph, 
                    Vertex<IT> * TailOfAugmentingPath,
                     DisjointSetUnion<IT> &dsu,
                    std::vector<Vertex<IT>> & vertexVector);
    template <typename IT, typename VT>
    static void pathThroughBlossom(Graph<IT, VT>& graph, 
                        // V
                        const Vertex<IT> * TailOfAugmentingPath,
                        const Vertex<IT> * TailOfAugmentingPathBase,
                        std::vector<Vertex<IT>> & vertexVector,
                        std::list<IT> & path);
    template <typename IT, typename VT>
    static void initializeBuffer(size_t N, size_t M,std::vector<Vertex<int64_t>>& vertexVector,
                  DisjointSetUnion<IT>& dsu, Stack<IT>& tree, Stack<IT>& stack);
};
template <typename IT, typename VT>
void Matcher::match(Graph<IT, VT>& graph) {
    // A map is used for the frontier to limit copying N vertices.
    //std::unordered_map<int64_t, Vertex<int64_t>> vertexMap;
    // A vector is used for the frontier to allocate once all the memory ever needed.
    std::vector<Vertex<IT>> vertexVector;
    DisjointSetUnion<IT> dsu;
    Stack<IT> tree;
    Stack<IT> stack;

    std::vector<Vertex<IT>> vertexVector2;
    DisjointSetUnion<IT> dsu2;
    Stack<IT> tree2;
    Stack<IT> stack2;
    // Create a static thread and pass references to the static function
    
    std::thread initializeBufferThread([&]() {
        Matcher::initializeBuffer<IT, VT>(graph.getN(), graph.getM(),
            std::ref(vertexVector2), std::ref(dsu2), std::ref(tree2), std::ref(stack2));
    });

    // Do other work in the main thread
    initializeBufferThread.detach();  // Detach the thread
    
    bool four_times_longer_than_alloc = false;
    auto allocate_start = high_resolution_clock::now();
    vertexVector.resize(graph.getN());
    tree.resize(graph.getN());
    stack.resize(graph.getM());
    dsu.reset(graph.getN());
    auto allocate_end = high_resolution_clock::now();
    auto duration_alloc = duration_cast<milliseconds>(allocate_end - allocate_start);
    std::cout << "|V|-vector memory allocation time: "<< duration_alloc.count() << " milliseconds" << std::endl;

    Vertex<IT> * TailOfAugmentingPath;
    IT time;
    // Access the graph elements as needed
    for (std::size_t i = 0; i < graph.getN(); ++i) {
        if (graph.matching[i] < 0) {
            //printf("SEARCHING FROM %ld!\n",i);
            //auto inserted = vertexMap.try_emplace(V_index,Vertex<IT>(time++,Label::EvenLabel));
            tree.push_back(i);
            time = 0;
            vertexVector[i].AgeField=time++;
            // Push edges onto stack, breaking if that stackEdge is a solution.
            Graph<IT,VT>::pushEdgesOntoStack(graph,vertexVector,i,stack);
            // Your matching logic goes here...
            auto search_start = high_resolution_clock::now();
            TailOfAugmentingPath=search(graph,stack,tree,dsu,vertexVector,time);
            auto search_end = high_resolution_clock::now();
            auto duration_search = duration_cast<milliseconds>(search_end - search_start);
            if (!four_times_longer_than_alloc){
                four_times_longer_than_alloc=duration_search/duration_alloc >2.0;
                if (four_times_longer_than_alloc)
                    printf("4x> longer to search than alloc on iteration %ld/%ld\n",i,graph.getN());
            }
            // If not a nullptr, I found an AP.
            if (TailOfAugmentingPath){
                augment(graph,TailOfAugmentingPath,dsu,vertexVector);
                
                for (auto V : tree) {
                    vertexVector[V].TreeField=-1;
                    vertexVector[V].BridgeField=-1;
                    vertexVector[V].ShoreField=-1;
                    vertexVector[V].AgeField=-1;
                    dsu.link[V]=V;
                    dsu.directParent[V]=-1;
                    dsu.groupRoot[V]=V;
                    dsu.size[V]=1;
                }
                stack.clear();
                tree.clear();
                //printf("FOUND AP!\n");
            } else {
                stack.clear();
                tree.clear();
                //printf("DIDNT FOUND AP!\n");
            }
        }
    }
}

template <typename IT, typename VT>
Vertex<IT> * Matcher::search(Graph<IT, VT>& graph, 
                    Stack<IT> &stack,
                    Stack<IT> &tree,
                    DisjointSetUnion<IT> &dsu,
                    std::vector<Vertex<IT>> & vertexVector,
                    IT time) {
    Vertex<int64_t> *FromBase,*ToBase, *nextVertex;
    int64_t FromBaseVertexID,ToBaseVertexID;
    IT stackEdge, matchedEdge;
    IT nextVertexIndex;
    while(!stack.empty()){
        stackEdge = stack.back();
        stack.pop_back();
        if (stackEdge<0)
            continue;
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
Vertex<IT> * Matcher::searchForkable(Graph<IT, VT>& graph, 
                    Stack<IT> &stack,
                    Stack<IT> &tree,
                    DisjointSetUnion<IT> &dsu,
                    std::vector<Vertex<IT>> & vertexVector,
                    Stack<IT> &stack2,
                    Stack<IT> &tree2,
                    DisjointSetUnion<IT> &dsu2,
                    std::vector<Vertex<IT>> & vertexVector2,
                    IT time) {
    Vertex<int64_t> *FromBase,*ToBase, *nextVertex;
    int64_t FromBaseVertexID,ToBaseVertexID;
    IT stackEdge, matchedEdge;
    IT nextVertexIndex;
    while(!stack.empty()){
        stackEdge = stack.back();
        stack.pop_back();
        if (stackEdge<0)
            continue;
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
                     DisjointSetUnion<IT> &dsu,
                    std::vector<Vertex<IT>> & vertexVector) {
    std::list<IT> path;
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
                    std::list<IT> & path) {
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
void Matcher::initializeBuffer(size_t N, size_t M,std::vector<Vertex<int64_t>>& vertexVector,
                  DisjointSetUnion<IT>& dsu, Stack<IT>& tree, Stack<IT>& stack) {
    // Perform some operations using the provided references
    vertexVector.resize(N);
    tree.resize(N);
    stack.resize(M);
    dsu.reset(N);
}


#endif
