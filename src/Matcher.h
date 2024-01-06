#ifndef Matcher_H
#define Matcher_H

#include "Graph.h"
#include "Vertex.h"
#include <list>
#include <unordered_map>
#include "Enums.h"
#include "Edge.h"
#include "Blossom.h"

class Matcher {
public:
    template <typename IT, typename VT>
    static void match(const Graph<IT, VT>& graph, 
                    std::vector<IT>& matching,
                    std::list<IT> &stack,
                    //std::unordered_map<int64_t,Vertex<int64_t>> &vertexMap,
                    std::vector<Vertex<IT>> & vertexVector);
    template <typename IT, typename VT>
    static Vertex<IT> * search(const Graph<IT, VT>& graph, 
                    const std::vector<IT>& matching, 
                    const size_t V_index,
                    std::list<IT> &stack,
                    //std::unordered_map<int64_t,Vertex<int64_t>> &vertexMap,
                    std::vector<Vertex<IT>> & vertexVector);
    template <typename IT, typename VT>
    static void pushEdgesOntoStack(const Graph<IT, VT>& graph, 
                                    std::vector<Vertex<IT>> & vertexVector, 
                                    IT V_index, 
                                    std::list<IT> &stack);

};
template <typename IT, typename VT>
void Matcher::match(const Graph<IT, VT>& graph, 
                    std::vector<IT>& matching,
                    std::list<IT> &stack,
                    //std::unordered_map<int64_t,Vertex<int64_t>> &vertexMap,
                    std::vector<Vertex<IT>> & vertexVector) {
    // Access the graph elements as needed
    for (std::size_t i = 0; i < graph.getN(); ++i) {
        if (matching[i] < 0) {
            // Your matching logic goes here...
            search(graph,matching,i,stack,vertexVector);
        }
    }
}
template <typename IT, typename VT>
Vertex<IT> * Matcher::search(const Graph<IT, VT>& graph, 
                    const std::vector<IT>& matching, 
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
    pushEdgesOntoStack<IT,VT>(graph,vertexVector,V_index,stack);
    while(!stack.empty()){
        stackEdge = stack.back();
        stack.pop_back();
        // Necessary because vertices dont know their own index.
        // It simplifies vector creation..
        FromBaseVertexID = Edge<IT,VT>::EdgeFrom(graph,stackEdge);
        FromBase = Blossom<IT>::Base(&vertexVector[FromBaseVertexID]);
        // Necessary because vertices dont know their own index.
        // It simplifies vector creation..
        ToBaseVertexID = Edge<IT,VT>::EdgeTo(graph,stackEdge);
        ToBase = Blossom<IT>::Base(&vertexVector[ToBaseVertexID]);
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
            nextVertexIndex = Edge<IT,VT>::Other(graph,matchedEdge,ToBaseVertexID);
            nextVertex = &vertexVector[nextVertexIndex];
            nextVertex->LabelField=Label::EvenLabel;
            nextVertex->AgeField=time++;
            pushEdgesOntoStack<IT,VT>(graph,vertexVector,nextVertexIndex,stack);

        } else if (ToBase->IsEven()) {
            // Shrink Blossoms

        }
    }
    return nullptr;
}

template <typename IT, typename VT>
void Matcher::pushEdgesOntoStack(const Graph<IT, VT>& graph, 
                                    std::vector<Vertex<IT>> & vertexVector, 
                                    IT V_index, 
                                    std::list<IT> &stack){
    IT nextVertexIndex;
    Vertex<IT>* nextVertex;

    // Push edges onto stack, breaking if that stackEdge is a solution.
    for (IT start = graph.indptr[V_index]; start < graph.indptr[V_index + 1]; ++start) {
        stack.push_back(graph.indices[start]);

        nextVertexIndex = Edge<IT, VT>::Other(graph, graph.indices[start], V_index);

        nextVertex = &vertexVector[nextVertexIndex];
        if (!nextVertex->IsReached() && !nextVertex->IsMatched())
            break;
    }
}
#endif
