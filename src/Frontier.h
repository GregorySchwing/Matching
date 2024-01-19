#ifndef FRONTIER_H
#define FRONTIER_H
#include <vector>
#include "Vertex.h"
#include "Stack.h"
#include "DSU.h"
#include "DSU2.h"
#include "State.h"

template <typename IT>
class Frontier  {
public:
    Frontier(size_t N, size_t M);
    void reinit();
    void updateTree();
    void clear();

    // Other member functions...
    std::vector<Vertex<IT>> vertexVector;
    State<IT> state;
    //std::vector<IT> stack;
    //std::vector<Vertex<IT>> tree;

    //Stack<Vertex<IT>> tree;
    Stack<IT> path;
    DisjointSetUnion<IT> dsu;
};

// Constructor
template <typename IT>
Frontier<IT>::Frontier(size_t N, size_t M): vertexVector(N), path(M){
    // for backwards compatability...
    #ifndef NDEBUG
    dsu.reset(N);
    #endif
    std::iota(vertexVector.begin(), vertexVector.end(), 0);
}

// Constructor
template <typename IT>
void Frontier<IT>::reinit(){      
    for (auto &V : state.tree) {
        vertexVector[V.LabelField].TreeField=-1;
        vertexVector[V.LabelField].BridgeField=-1;
        vertexVector[V.LabelField].ShoreField=-1;
        vertexVector[V.LabelField].AgeField=-1;
        vertexVector[V.LabelField].LinkField=V.LabelField;
        vertexVector[V.LabelField].DirectParentField=-1;
        vertexVector[V.LabelField].GroupRootField=V.LabelField;
        vertexVector[V.LabelField].SizeField=1;
        #ifndef NDEBUG
        dsu.link[V.LabelField]=V.LabelField;
        dsu.directParent[V.LabelField]=-1;
        dsu.groupRoot[V.LabelField]=V.LabelField;
        dsu.size[V.LabelField]=1;
        #endif
    }
}

// Constructor
template <typename IT>
void Frontier<IT>::updateTree(){      
    for (auto &V : state.tree) {
        V.TreeField=vertexVector[V.LabelField].TreeField;
        V.BridgeField=vertexVector[V.LabelField].BridgeField;
        V.ShoreField=vertexVector[V.LabelField].ShoreField;
        V.AgeField=vertexVector[V.LabelField].AgeField;
        V.LinkField=vertexVector[V.LabelField].LinkField;
        V.DirectParentField=vertexVector[V.LabelField].DirectParentField;
        V.GroupRootField=vertexVector[V.LabelField].GroupRootField;
        V.SizeField=vertexVector[V.LabelField].SizeField;
    }
}

// Constructor
template <typename IT>
void Frontier<IT>::clear(){
    state.clear();
    path.clear();
}

#endif // FRONTIER_H
