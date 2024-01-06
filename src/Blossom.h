#ifndef BLOSSOM_H
#define BLOSSOM_H

#include "Vertex.h"

template <typename IT>
class Blossom {
public:
    // Static method for Union-Find with path compression
    static Vertex<IT>* SetUnion(Vertex<IT>* x, Vertex<IT>* y) {
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

    // Static method to find the root of a vertex
    static Vertex<IT>* Base(Vertex<IT>* x) {
        return FindSet(x);
    }

private:
    // Helper function for path compression
    static Vertex<IT>* FindSet(Vertex<IT>* x) {
        if (x != x->ParentField) {
            x->ParentField = FindSet(x->ParentField); // Path compression
        }
        return x->ParentField;
    }
};

#endif // BLOSSOM_H
