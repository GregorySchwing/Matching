#ifndef BLOSSOM_H
#define BLOSSOM_H

#include "Vertex.h"
#include "Edge.h"

class Blossom {
    public:
        // Static method to find the root of a vertex
        template <typename IT>
        static Vertex<IT>* Base(Vertex<IT>* x);
        // Static method to find the root of a vertex
        template <typename IT, typename VT>
        static Vertex<IT>* Shrink(const Graph<IT, VT>& graph, const IT stackEdge, std::vector<Vertex<IT>> & vertexVector, std::list<IT> &stack);
    private:

        // Helper function for path compression
        template <typename IT>
        static Vertex<IT>* FindSet(Vertex<IT>* x);

        // Static method for Union-Find with path compression
        template <typename IT>
        static Vertex<IT>* SetUnion(Vertex<IT>* x, Vertex<IT>* y);
};

template <typename IT>
Vertex<IT>* Blossom::Base(Vertex<IT>* x) {
    return FindSet(x);
}

template <typename IT, typename VT>
Vertex<IT>* Blossom::Shrink(const Graph<IT, VT>& graph, const IT stackEdge, std::vector<Vertex<IT>> & vertexVector, std::list<IT> &stack){
    Vertex<IT> *FromBase,*ToBase, *nextVertex;
    Vertex<IT> *EdgeFromVertex,*EdgeToVertex;
    IT nextEdge, matchedEdge, treeEdge;

    nextEdge = stackEdge;
    EdgeFromVertex = &vertexVector[Edge<IT,VT>::EdgeFrom(graph,nextEdge)];
    FromBase = Blossom::Base(EdgeFromVertex);
    EdgeToVertex = &vertexVector[Edge<IT,VT>::EdgeTo(graph,nextEdge)];
    ToBase = Blossom::Base(EdgeToVertex);

    if(ToBase->AgeField > FromBase->AgeField){
        std::swap(FromBase,ToBase);
        std::swap(EdgeFromVertex,EdgeToVertex);
    }

    /*
    * Walk up the alternating tree from vertex V to vertex A, shrinking
    * the blossoms into a superblossom.  Edges incident to the odd vertices
    * on the path from V to A are pushed onto stack S, to later search from.
    */
    bool Found = false;
    while(FromBase!=ToBase){
        matchedEdge = FromBase->MatchField;
        ptrdiff_t FromBase_VertexID = FromBase - &vertexVector[0];
        nextVertex = &vertexVector[Edge<IT,VT>::Other(graph,matchedEdge,FromBase_VertexID)];
        nextVertex->BridgeField = nextEdge;
        ptrdiff_t EdgeFromVertex_VertexID = EdgeFromVertex - &vertexVector[0];
        nextVertex->ShoreField = EdgeFromVertex_VertexID;
        treeEdge = nextVertex->TreeField;
        ptrdiff_t nextVertex_VertexID = nextVertex - &vertexVector[0];

        if (!Found){
            //pushEdgesOntoStack<IT,VT>(graph,vertexVector,nextVertex_VertexID,stack,matchedEdge,treeEdge);

        }
    }

    return nullptr;
}


template <typename IT>
Vertex<IT>* Blossom::SetUnion(Vertex<IT>* x, Vertex<IT>* y) {
    Vertex<IT>* rootX = FindSet(x);
    Vertex<IT>* rootY = FindSet(y);

    // Check if they are already in the same set
    if (rootX == rootY) {
        return rootX;
    }

    // Perform Union by rank
    if (rootX->RankField < rootY->RankField) {
        rootX->ParentField = rootY;
        return rootY;
    } else if (rootX->RankField > rootY->RankField) {
        rootY->ParentField = rootX;
        return rootX;
    } else {
        // If ranks are the same, arbitrarily choose one as the parent and increment its rank
        rootY->ParentField = rootX;
        rootX->RankField++;
        return rootX;
    }
}

// Helper function for path compression
template <typename IT>
Vertex<IT>* Blossom::FindSet(Vertex<IT>* x) {
    if (x != x->ParentField) {
        x->ParentField = FindSet(x->ParentField); // Path compression
    }
    return x->ParentField;
}

#endif // BLOSSOM_H
