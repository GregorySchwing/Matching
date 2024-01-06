#ifndef Matcher_H
#define Matcher_H

#include "Graph.h"
#include "Vertex.h"
#include <list>
#include <unordered_map>

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
    IT age;

    vertexMap[V_index] = Vertex<IT>(V_index,age++);
    // Push edges onto stack
    for (IT start = graph.indptr[V_index]; start < graph.indptr[V_index+1]; ++start)
        stack.push_back(graph.indices[start]);
    while(!stack.empty()){
        edge = stack.back();
        stack.pop_back();
    }
    return true;
}
#endif
