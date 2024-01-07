#ifndef Matcher_H
#define Matcher_H

#include "Graph.h"
#include "Vertex.h"
#include <list>
#include <unordered_map>
#include "Enums.h"
////#include "Edge.h"
#include "Blossom.h"

class Matcher {
public:
    template <typename IT, typename VT>
    static void match(const Graph<IT, VT>& graph, 
                    std::list<IT> &stack,
                    std::vector<Vertex<IT>> & vertexVector);
private:
    template <typename IT, typename VT>
    static Vertex<IT> * search(const Graph<IT, VT>& graph, 
                    const size_t V_index,
                    std::list<IT> &stack,
                    std::vector<Vertex<IT>> & vertexVector);
    template <typename IT, typename VT>
    static void augment(const Graph<IT, VT>& graph, 
                    Vertex<IT> * TailOfAugmentingPath,
                    std::vector<Vertex<IT>> & vertexVector);
    template <typename IT, typename VT>
    static void pathThroughBlossom(const Graph<IT, VT>& graph, 
                        // V
                        const Vertex<IT> * TailOfAugmentingPath,
                        const Vertex<IT> * TailOfAugmentingPathBase,
                        std::vector<Vertex<IT>> & vertexVector,
                        std::list<IT> & path);
};
template <typename IT, typename VT>
void Matcher::match(const Graph<IT, VT>& graph, 
                    std::list<IT> &stack,
                    //std::unordered_map<int64_t,Vertex<int64_t>> &vertexMap,
                    std::vector<Vertex<IT>> & vertexVector) {
    Vertex<IT> * TailOfAugmentingPath;
    // Access the graph elements as needed
    for (std::size_t i = 0; i < graph.getN(); ++i) {
        if (!vertexVector[i].IsMatched()) {
            // Your matching logic goes here...
            TailOfAugmentingPath=search(graph,i,stack,vertexVector);
            // If not a nullptr, I found an AP.
            if (TailOfAugmentingPath){
                augment(graph,TailOfAugmentingPath,vertexVector);
                printf("FOUND AP!\n");
            } else {
                printf("DIDNT FOUND AP!\n");
            }
        }
    }
}
template <typename IT, typename VT>
Vertex<IT> * Matcher::search(const Graph<IT, VT>& graph, 
                    const size_t V_index,
                    std::list<IT> &stack,
                    //std::unordered_map<int64_t,Vertex<int64_t>> &vertexMap,
                    std::vector<Vertex<IT>> & vertexVector) {
    Vertex<int64_t> *FromBase,*ToBase, *nextVertex;
    int64_t FromBaseVertexID,ToBaseVertexID;
    IT stackEdge, matchedEdge;
    IT nextVertexIndex;
    IT time = 0;
    //auto inserted = vertexMap.try_emplace(V_index,Vertex<IT>(time++,Label::EvenLabel));
    nextVertex = &vertexVector[V_index];
    nextVertex->AgeField=time++;
    nextVertex->LabelField=Label::EvenLabel;
    // Push edges onto stack, breaking if that stackEdge is a solution.
    Graph<IT,VT>::pushEdgesOntoStack(graph,vertexVector,V_index,stack);
    while(!stack.empty()){
        stackEdge = stack.back();
        stack.pop_back();
        // Necessary because vertices dont know their own index.
        // It simplifies vector creation..
        FromBaseVertexID = Graph<IT,VT>::EdgeFrom(graph,stackEdge);
        FromBase = Blossom::Base(&vertexVector[FromBaseVertexID]);
        // Necessary because vertices dont know their own index.
        // It simplifies vector creation..
        ToBaseVertexID = Graph<IT,VT>::EdgeTo(graph,stackEdge);
        ToBase = Blossom::Base(&vertexVector[ToBaseVertexID]);
        // Edge is between two vertices in the same blossom, continue.
        if (FromBase == ToBase)
            continue;
        if(!FromBase->IsEven()){
            std::swap(FromBase,ToBase);
            std::swap(FromBaseVertexID,ToBaseVertexID);
        }
        // An unreached, unmatched vertex is found, AN AUGMENTING PATH!
        if (!ToBase->IsReached() && !ToBase->IsMatched()){
            ToBase->LabelField=Label::OddLabel;
            ToBase->TreeField=stackEdge;
            ToBase->AgeField=time++;
            ToBase->MatchField=stackEdge;
            // I'll let the augment path method recover the path.
            return ToBase;
        } else if (!ToBase->IsReached() && ToBase->IsMatched()){
            ToBase->LabelField=Label::OddLabel;
            ToBase->TreeField=stackEdge;
            ToBase->AgeField=time++;

            matchedEdge=ToBase->MatchField;
            nextVertexIndex = Graph<IT,VT>::Other(graph,matchedEdge,ToBaseVertexID);
            nextVertex = &vertexVector[nextVertexIndex];
            nextVertex->LabelField=Label::EvenLabel;
            nextVertex->AgeField=time++;
            Graph<IT,VT>::pushEdgesOntoStack(graph,vertexVector,nextVertexIndex,stack);

        } else if (ToBase->IsEven()) {
            // Shrink Blossoms
            //Blossom::Shrink(graph,stackEdge,vertexVector,stack);
        }
    }
    return nullptr;
}

template <typename IT, typename VT>
void Matcher::augment(const Graph<IT, VT>& graph, 
                    // V
                    Vertex<IT> * TailOfAugmentingPath,
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
        nextVertex = &vertexVector[Graph<IT,VT>::Other(graph,edge,TailOfAugmentingPath_VertexID)];

        //B = Base(Blossom(W));
        nextVertexBase = Blossom::Base(nextVertex); 
        
        // Path(W, B, P);
        pathThroughBlossom(graph,nextVertex,nextVertexBase,vertexVector,path);

        //V = Other(Match(B), B);
        ptrdiff_t nextVertexBase_VertexID = nextVertexBase - &vertexVector[0];
        if (nextVertexBase->MatchField>-1)
            TailOfAugmentingPath = &vertexVector[Graph<IT,VT>::Other(graph,nextVertexBase->MatchField,nextVertexBase_VertexID)];
        else 
            TailOfAugmentingPath = nullptr;
    } while (TailOfAugmentingPath != nullptr);
    // Print the list of integers
    for (auto E : path) {
        //Match(EdgeFrom(E)) = E;
        vertexVector[Graph<IT,VT>::EdgeFrom(graph,E)].MatchField=E;
        //Match(EdgeTo(E)) = E;
        vertexVector[Graph<IT,VT>::EdgeTo(graph,E)].MatchField=E;
    }
}


template <typename IT, typename VT>
void Matcher::pathThroughBlossom(const Graph<IT, VT>& graph, 
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
                                &vertexVector[Graph<IT,VT>::Other(graph,TailOfAugmentingPath->MatchField,TailOfAugmentingPath_VertexID)],
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
            nextVertex=&vertexVector[Graph<IT,VT>::Other(graph,TailOfAugmentingPath->MatchField,TailOfAugmentingPath_VertexID)];
            
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
