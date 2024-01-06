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
    IT time = 0;
    bool Found = false;
    auto inserted = vertexMap.try_emplace(V_index,Vertex<IT>(time++,Label::EvenLabel));

    // Push edges onto stack, breaking if that edge is a solution.
    for (IT start = graph.indptr[V_index]; start < graph.indptr[V_index+1]; ++start){
        stack.push_back(graph.indices[start]);
        nextVertexIndex = Edge<IT,VT>::Other(graph,graph.indices[start],V_index);
        auto inserted = vertexMap.try_emplace(nextVertexIndex,Vertex<IT>{});
        if (!inserted.first->second.IsReached() && !inserted.first->second.IsMatched())
            break;
    }
    while(!stack.empty()){
        edge = stack.back();
        stack.pop_back();
        auto inserted = vertexMap.try_emplace(Edge<IT,VT>::EdgeTo(graph,edge),Vertex<IT>{});
        // Successfully inserted new object, need to update ParentField to *this pointer
        if (inserted.second)
            inserted.first->second.ParentField = &(inserted.first->second);
        auto inserted2 = vertexMap.try_emplace(Edge<IT,VT>::EdgeFrom(graph,edge),Vertex<IT>{});
        // Successfully inserted new object, need to update ParentField to *this pointer
        if (inserted2.second)
            inserted2.first->second.ParentField = &(inserted2.first->second);

        FromBase = Blossom<IT>::Base(&inserted.first->second);
        ToBase = Blossom<IT>::Base(&inserted2.first->second);
        // Edge is between two vertices in the same blossom, continue.
        if (FromBase == ToBase)
            continue;
        if(!FromBase->IsEven())
            std::swap(FromBase,ToBase);
        // An unreached, unmatched vertex is found, AN AUGMENTING PATH!
        if (0 && !ToBase->IsReached() && !ToBase->IsMatched()){
            ToBase->LabelField=Label::OddLabel;
            ToBase->TreeField=edge;
            ToBase->AgeField=time++;
            // TODO RECOVER(ToBase)
            ToBase->MatchField=edge;
            Found = true;
            break;
        }
    }
    return Found;
}
#endif
