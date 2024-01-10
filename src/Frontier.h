#ifndef FRONTIER_H
#define FRONTIER_H
#include <vector>
#include "Vertex.h"
#include "Stack.h"
#include "DSU.h"

template <typename IT>
class Frontier  {
public:
    Frontier(size_t N, size_t M);

    // Other member functions...
    std::vector<Vertex<IT>> vertexVector;
    Stack<IT> stack;
    Stack<IT> tree;
    DisjointSetUnion<IT> dsu;
};

// Constructor
template <typename IT>
Frontier<IT>::Frontier(size_t N, size_t M): vertexVector(N), tree(N), stack(M){
    dsu.reset(N);
}

#endif // FRONTIER_H
