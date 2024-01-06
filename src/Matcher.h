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
                    std::list<int64_t> &stack,
                    //std::unordered_map<int64_t,Vertex<int64_t>> &vertexMap,
                    std::vector<Vertex<IT>> & vertexVector);
    template <typename IT, typename VT>
    static Vertex<IT> * search(const Graph<IT, VT>& graph, 
                    const std::vector<IT>& matching, 
                    const size_t V_index,
                    std::list<int64_t> &stack,
                    //std::unordered_map<int64_t,Vertex<int64_t>> &vertexMap,
                    std::vector<Vertex<IT>> & vertexVector);
};
template <typename IT, typename VT>
void Matcher::match(const Graph<IT, VT>& graph, 
                    std::vector<IT>& matching,
                    std::list<int64_t> &stack,
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
                    std::list<int64_t> &stack,
                    //std::unordered_map<int64_t,Vertex<int64_t>> &vertexMap,
                    std::vector<Vertex<IT>> & vertexVector) {
    Vertex<int64_t> *FromBase,*ToBase, *CurrentVertex;
    IT edge;
    IT nextVertexIndex;
    IT time = 0;
    bool Found = false;
    //auto inserted = vertexMap.try_emplace(V_index,Vertex<IT>(time++,Label::EvenLabel));
    CurrentVertex = &vertexVector[V_index];
    CurrentVertex->AgeField=time++;
    CurrentVertex->LabelField=Label::EvenLabel;
    // Push edges onto stack, breaking if that edge is a solution.
    for (IT start = graph.indptr[V_index]; start < graph.indptr[V_index+1]; ++start){
        stack.push_back(graph.indices[start]);

        nextVertexIndex = Edge<IT,VT>::Other(graph,graph.indices[start],V_index);
       
        CurrentVertex = &vertexVector[nextVertexIndex];
        if (!CurrentVertex->IsReached() && !CurrentVertex->IsMatched())
            break;
    }
    while(!stack.empty()){
        edge = stack.back();
        stack.pop_back();
        FromBase = Blossom<IT>::Base(&vertexVector[Edge<IT,VT>::EdgeFrom(graph,edge)]);
        ToBase = Blossom<IT>::Base(&vertexVector[Edge<IT,VT>::EdgeTo(graph,edge)]);
        // Edge is between two vertices in the same blossom, continue.
        if (FromBase == ToBase)
            continue;
        if(!FromBase->IsEven())
            std::swap(FromBase,ToBase);
        // An unreached, unmatched vertex is found, AN AUGMENTING PATH!
        if (!ToBase->IsReached() && !ToBase->IsMatched()){
            ToBase->LabelField=Label::OddLabel;
            ToBase->TreeField=edge;
            ToBase->AgeField=time++;
            ToBase->MatchField=edge;
            // I'll let the augment path method recover the path.
            return ToBase;
        }
    }
    return nullptr;
}
#endif
