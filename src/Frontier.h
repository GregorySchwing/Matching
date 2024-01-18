#ifndef FRONTIER_H
#define FRONTIER_H
#include <vector>
#include "Vertex.h"
#include "Stack.h"
#include "DSU.h"
#include "DSU2.h"

template <typename IT>
class Frontier  {
public:
    Frontier(size_t N, size_t M);
    void reinit();
    void clear();

    // Other member functions...
    std::vector<Vertex<IT>> vertexVector;
    Stack<IT> stack;
    Stack<IT> tree;
    Stack<IT> path;
    DisjointSetUnion<IT> dsu;
};

// Constructor
template <typename IT>
Frontier<IT>::Frontier(size_t N, size_t M): vertexVector(N), tree(N), path(M), stack(M){
    // for backwards compatability...
    #ifndef NDEBUG
    dsu.reset(N);
    #endif
    std::iota(vertexVector.begin(), vertexVector.end(), 0);
}

// Constructor
template <typename IT>
void Frontier<IT>::reinit(){ 
    //DisjointSetUnionHelper<IT>::reset(10,vertexVector);     
    //DisjointSetUnionHelper<IT>::find(10,vertexVector);       
    for (auto V : tree) {
        vertexVector[V].TreeField=-1;
        vertexVector[V].BridgeField=-1;
        vertexVector[V].ShoreField=-1;
        vertexVector[V].AgeField=-1;
        vertexVector[V].LinkField=V;
        vertexVector[V].DirectParentField=-1;
        vertexVector[V].GroupRootField=V;
        vertexVector[V].SizeField=1;
        #ifndef NDEBUG
        dsu.link[V]=V;
        dsu.directParent[V]=-1;
        dsu.groupRoot[V]=V;
        dsu.size[V]=1;
        #endif
    }

}


// Constructor
template <typename IT>
void Frontier<IT>::clear(){       
    stack.clear();
    tree.clear();
    path.clear();
}

#endif // FRONTIER_H
