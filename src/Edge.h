#ifndef EDGE_H
#define EDGE_H
#include "Enums.h"
#include <vector>

template <typename IT, typename VT>
class Edge {
public:
    // Static method to find the other endpoint of an edge
    static IT Other(const Graph<IT, VT>& graph, const IT edgeIndex, const IT vertexId) {
        IT source = graph.original_rows[edgeIndex];
        IT destination = graph.original_cols[edgeIndex];
        if (vertexId == source) {
            return destination;
        } else {
            return source;
        }
    }
    // Static method to find the other endpoint of an edge
    static IT EdgeFrom(const Graph<IT, VT>& graph, const IT edgeIndex) {
        IT source = graph.original_rows[edgeIndex];
        return source;
    }
    // Static method to find the other endpoint of an edge
    static IT EdgeTo(const Graph<IT, VT>& graph, const IT edgeIndex) {
        IT destination = graph.original_cols[edgeIndex];
        return destination;
    }
};
#endif //EDGE_H
