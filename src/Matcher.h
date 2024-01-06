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
    static void match(const Graph<IT, VT>& graph, std::vector<IT>& matching);
    template <typename IT, typename VT>
    static bool search(const Graph<IT, VT>& graph, const std::vector<IT>& matching, const size_t V_index);
};
template <typename IT, typename VT>
void Matcher::match(const Graph<IT, VT>& graph, std::vector<IT>& matching) {
    // Access the graph elements as needed
    for (std::size_t i = 0; i < graph.getN(); ++i) {
        if (matching[i] < 0) {
            // Your matching logic goes here...
            search(graph,matching,i);
        }
    }
}
template <typename IT, typename VT>
bool Matcher::search(const Graph<IT, VT>& graph, const std::vector<IT>& matching, const size_t V_index) {
    // std::list is used for its splicing efficiency
    // Transfers elements from one list to another. 
    // No elements are copied or moved, only the internal 
    // pointers of the list nodes are re-pointed.
    // https://en.cppreference.com/w/cpp/container/list/splice
    std::list<IT> stack;
    // A map is used for the frontier to limit copying N vertices.
    std::unordered_map<IT, Vertex<IT>> vertexMap;
    IT edge;
    IT nextVertexIndex;
    Vertex<IT> nextVertex;
    Vertex<IT> *FromBase,*ToBase;
    IT age = 0;
    bool Found = false;
    vertexMap[V_index] = Vertex<IT>(age++,Label::EvenLabel);
    // Push edges onto stack, breaking if that edge is a solution.
    for (IT start = graph.indptr[V_index]; start < graph.indptr[V_index+1]; ++start){
        stack.push_back(graph.indices[start]);
        nextVertexIndex = Edge<IT,VT>::Other(graph,start,V_index);
        auto inserted = vertexMap.try_emplace(nextVertexIndex,Vertex<IT>{});
        vertexMap[nextVertexIndex].ParentField=&((inserted.first)->second);
        vertexMap[nextVertexIndex].print();
        if (!vertexMap[nextVertexIndex].IsMatched() && !vertexMap[nextVertexIndex].IsReached())
            break;
    }
    while(!stack.empty()){
        edge = stack.back();
        stack.pop_back();
        vertexMap[Edge<IT,VT>::EdgeTo(graph,edge)].print();
        vertexMap[Edge<IT,VT>::EdgeFrom(graph,edge)].print();
        FromBase = Blossom<IT>::Base(&(vertexMap[Edge<IT,VT>::EdgeFrom(graph,edge)]));
        //ToBase = Blossom<IT>::Base(&vertexMap[Edge<IT,VT>::EdgeTo(graph,edge)]);
        // Edge is between two vertices in the same blossom, continue.
        //if (FromBase == ToBase)
        //    continue;
        //if (true){}
    }
    return Found;
}
#endif
