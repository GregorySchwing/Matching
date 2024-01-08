#ifndef BLOSSOM_H
#define BLOSSOM_H

#include "Vertex.h"
//#include "Edge.h"

class Blossom {
    public:
        // Static method to find the root of a vertex
        template <typename IT>
        static Vertex<IT>* Base(Vertex<IT>* x);
        template <typename IT>
        static Vertex<IT>* Root(Vertex<IT>* x);
        // Static method to find the root of a vertex
        template <typename IT, typename VT>
        static void Shrink(const Graph<IT, VT>& graph, const IT stackEdge, std::vector<Vertex<IT>> & vertexVector, std::list<IT> &stack);
    private:

        // Helper function for path compression
        template <typename IT>
        static Vertex<IT>* FindSet(Vertex<IT>* x);

        // Static method for Union-Find with path compression
        template <typename IT>
        static Vertex<IT>* SetUnion(Vertex<IT>* x, Vertex<IT>* y);
};

template <typename IT>
Vertex<IT>* Blossom::Root(Vertex<IT>* x) {
    return FindSet(x);
}

template <typename IT>
Vertex<IT>* Blossom::Base(Vertex<IT>* x) {
    return FindSet(x)->BaseField;
}

template <typename IT, typename VT>
void Blossom::Shrink(const Graph<IT, VT>& graph, const IT stackEdge, std::vector<Vertex<IT>> & vertexVector, std::list<IT> &stack){
    // V,W
    Vertex<IT> *EdgeFromVertex,*EdgeToVertex;
    // X,Y
    Vertex<IT> *FromRoot,*ToRoot;
    // A,B
    Vertex<IT> *FromBase,*ToBase;
    // E
    IT nextEdge;

    nextEdge = stackEdge;
    // V = EdgeFrom(E);
    EdgeFromVertex = &vertexVector[Graph<IT,VT>::EdgeFrom(graph,nextEdge)];
    // W = EdgeTo(E);
    EdgeToVertex = &vertexVector[Graph<IT,VT>::EdgeTo(graph,nextEdge)];
    // X = Blossom(V);
    FromRoot = Blossom::Root(EdgeFromVertex);
    // Y = Blossom(W);
    ToRoot = Blossom::Root(EdgeToVertex);
    // B = Base(X);
    FromBase = Blossom::Base(FromRoot);
    // A = Base(Y);
    ToBase = Blossom::Base(ToRoot);

    // if (Age(A) > Age(B))
    if(ToBase->AgeField > FromBase->AgeField){
        std::swap(FromBase,ToBase);
        std::swap(FromRoot,ToRoot);
        std::swap(EdgeFromVertex,EdgeToVertex);
    }

    /*
    * Walk up the alternating tree from vertex V to vertex A, shrinking
    * the blossoms into a superblossom.  Edges incident to the odd vertices
    * on the path from V to A are pushed onto stack S, to later search from.
    */
    bool Found = false;
    // while (B != A)
    while(FromBase!=ToBase){
        IT matchedEdge, treeEdge;
        ptrdiff_t FromBase_VertexID = FromBase - &vertexVector[0];
        // M = Match(B);
        matchedEdge = graph.GetMatchField(FromBase_VertexID);

        // W = Other(M, B);
        EdgeToVertex = &vertexVector[Graph<IT,VT>::Other(graph,matchedEdge,FromBase_VertexID)];
        
        // Bridge(W) = E;
        EdgeToVertex->BridgeField = nextEdge;

        // Shore(W) = V;
        ptrdiff_t EdgeFromVertex_VertexID = EdgeFromVertex - &vertexVector[0];
        EdgeToVertex->ShoreField = EdgeFromVertex_VertexID;

        // T = Tree(W);
        treeEdge = EdgeToVertex->TreeField;
        ptrdiff_t EdgeToVertex_VertexID = EdgeToVertex - &vertexVector[0];
        if (!Found){
            Found = Graph<IT,VT>::pushEdgesOntoStack(graph,vertexVector,EdgeToVertex_VertexID,stack,matchedEdge,treeEdge);
        }

        // Little unsure of this logic.
        // Y = Blossom(W);
        ToRoot = Blossom::Root(EdgeToVertex);
        // X = SetUnion(Y, X);
        FromRoot = SetUnion(ToRoot, FromRoot);
        // E = T;
        nextEdge = treeEdge;
        // V = Other(E, W);
        EdgeFromVertex = &vertexVector[Graph<IT,VT>::Other(graph,nextEdge,EdgeToVertex_VertexID)];

        // Y = Blossom(V);
        ToRoot = Blossom::Root(EdgeFromVertex);
        FromRoot = SetUnion(ToBase, FromRoot);
        FromBase = Blossom::Base(FromRoot);
    }
}


template <typename IT>
Vertex<IT>* Blossom::SetUnion(Vertex<IT>* x, Vertex<IT>* y) {
    // E = SetFind(E);
    x = FindSet(x);
    // F = SetFind(F);
    y = FindSet(y);

    // Check if they are already in the same set
    // if (E == F)
    if (x == y) {
        // return E;
        return x;
    }

    // D = E->Label;
    Vertex<IT>* D = x->BaseField;
    // Perform Union by rank
    // if (E->Rank < F->Rank)
    if (x->RankField < y->RankField) {
        // E->Up = F;
        x->ParentField = y;
        // F->Label = D;
        y->BaseField = D;
        return y;
    // else if (E->Rank > F->Rank)
    } else if (x->RankField > y->RankField) {
        // F->Up = E;
        y->ParentField = x;
        return x;
    } else {
        // If ranks are the same, arbitrarily choose one as the parent and increment its rank
        // E->Rank += 1;
        x->RankField++;
        // F->Up = E;
        y->ParentField = x;
        return x;
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
