#ifndef VERTEX_H
#define VERTEX_H
#include "Enums.h"
// Define the Vertex class template
template <typename IT>
class Vertex {
public:
    // Blossoms will be contracted by updating this pointer and RankField
    Vertex<IT>* ParentField;
    IT RankField;

    IT* MatchField;
    IT* TreeField;
    IT* BridgeField;
    Vertex<IT>* ShoreField;
    short LabelField;
    IT AgeField;

    // Constructor
    Vertex(IT age,short int Label)
        : ParentField(this), RankField(0), MatchField(nullptr), TreeField(nullptr),
          BridgeField(nullptr), ShoreField(nullptr), LabelField(Label), AgeField(age) {}

    // Copy constructor
    Vertex(const Vertex& other)
        : ParentField(other.ParentField), RankField(other.RankField), MatchField(other.MatchField),
          TreeField(other.TreeField), BridgeField(other.BridgeField),
          ShoreField(other.ShoreField), LabelField(other.LabelField),
          AgeField(other.AgeField) {}

    // Default constructor
    Vertex() : ParentField(this), RankField(0), MatchField(nullptr), TreeField(nullptr),
          BridgeField(nullptr), ShoreField(nullptr), LabelField(Label::UnreachedLabel), AgeField(-1) {}
    
    // Method to check if the vertex is reached
    bool IsReached() const {
        return LabelField != Label::UnreachedLabel;
    }

    // Method to check if the vertex is reached
    bool IsEven() const {
        return LabelField == Label::EvenLabel;
    }

    // Method to check if the vertex is matched
    bool IsMatched() const {
        return MatchField != nullptr;
    }

    // Utility function to print vertex information
    void print() const {
        std::cout << "BlossomField: " << ParentField << ", "
                  << "RankField: " << RankField << ", "
                  << "MatchField: " << MatchField << ", "
                  << "TreeField: " << TreeField << ", "
                  << "BridgeField: " << BridgeField << ", "
                  << "ShoreField: " << ShoreField << ", "
                  << "LabelField: " << LabelField << ", "
                  << "AgeField: " << AgeField << ", "
                  << "IsReached: " << IsReached() << ", "
                  << "IsMatched: " << IsMatched() << ", "
                  << std::endl;

    }
};
#endif //VERTEX_H
