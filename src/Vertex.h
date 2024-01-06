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
    IT* ShoreField;
    short LabelField;
    IT AgeField;
    IT BaseField;

    // Constructor
    Vertex(IT vid, IT age)
        : BlossomField(this), MatchField(nullptr), TreeField(nullptr),
          BridgeField(nullptr), ShoreField(nullptr), LabelField(Label::UnreachedLabel), BaseField(vid), AgeField(age) {}

    // Copy constructor
    Vertex(const Vertex& other)
        : BlossomField(other.BlossomField), MatchField(other.MatchField),
          TreeField(other.TreeField), BridgeField(other.BridgeField),
          ShoreField(other.ShoreField), LabelField(other.LabelField),
          AgeField(other.AgeField) {}

    // Default constructor
    Vertex() : Vertex(-1,-1) {}

    // Utility function to print vertex information
    void print() const {
        std::cout << "BlossomField: " << BlossomField << ", "
                  << "MatchField: " << MatchField << ", "
                  << "TreeField: " << TreeField << ", "
                  << "BridgeField: " << BridgeField << ", "
                  << "ShoreField: " << ShoreField << ", "
                  << "LabelField: " << LabelField << ", "
                  << "AgeField: " << AgeField << std::endl;
    }
};
#endif //VERTEX_H
