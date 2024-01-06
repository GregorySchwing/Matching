#ifndef VERTEX_H
#define VERTEX_H
#include "Enums.h"
// Define the Vertex class template
template <typename IT>
class Vertex {
public:
    // Blossoms will be contracted by updating this pointer.
    Vertex<IT>* BlossomField;
    IT* MatchField;
    IT* TreeField;
    IT* BridgeField;
    Vertex<IT>* ShoreField;
    short LabelField;
    IT AgeField;
    Vertex<IT>* BaseField;

    // Constructor
    Vertex(IT vid, IT age,short int Label)
        : BlossomField(this), MatchField(nullptr), TreeField(nullptr),
          BridgeField(nullptr), ShoreField(nullptr), LabelField(Label), BaseField(this), AgeField(age) {}

    // Copy constructor
    Vertex(const Vertex& other)
        : BlossomField(other.BlossomField), MatchField(other.MatchField),
          TreeField(other.TreeField), BridgeField(other.BridgeField),
          ShoreField(other.ShoreField), LabelField(other.LabelField),
          AgeField(other.AgeField) {}

    // Default constructor
    Vertex() : Vertex(-1,-1,Label::UnreachedLabel) {}

    // Method to check if the vertex is reached
    bool IsReached() const {
        return LabelField != Label::UnreachedLabel;
    }

    // Method to check if the vertex is matched
    bool IsMatched() const {
        return MatchField != nullptr;
    }

    // Utility function to print vertex information
    void print() const {
        std::cout << "BlossomField: " << BlossomField << ", "
                  << "MatchField: " << MatchField << ", "
                  << "TreeField: " << TreeField << ", "
                  << "BridgeField: " << BridgeField << ", "
                  << "ShoreField: " << ShoreField << ", "
                  << "LabelField: " << LabelField << ", "
                  << "BaseField: " << BaseField << ", "
                  << "AgeField: " << AgeField << ", "
                  << "IsReached: " << IsReached() << ", "
                  << "IsMatched: " << IsMatched() << ", "
                  << std::endl;

    }
};
#endif //VERTEX_H
