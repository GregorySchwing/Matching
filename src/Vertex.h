#ifndef VERTEX_H
#define VERTEX_H
#include "Enums.h"
// Define the Vertex class template
template <typename IT>
class Vertex {
public:
    // Blossoms will be contracted by updating this pointer and RankField
    Vertex<IT>* ParentField;
    Vertex<IT>* BaseField;
    IT RankField;
    IT TreeField;
    IT BridgeField;
    IT ShoreField;
    short LabelField;
    IT AgeField;

    // Constructor
    Vertex(IT age,short int Label)
        : ParentField(this), BaseField(this), RankField(0), TreeField(-1),
          BridgeField(-1), ShoreField(-1), LabelField(Label), AgeField(age) {}

    // Copy constructor
    Vertex(const Vertex& other)
        : ParentField(other.ParentField), BaseField(other.BaseField), 
          RankField(other.RankField), TreeField(other.TreeField), 
          BridgeField(other.BridgeField),ShoreField(other.ShoreField), 
          LabelField(other.LabelField),AgeField(other.AgeField) {}
          

    // Default constructor
    Vertex() : ParentField(this), BaseField(this), RankField(0), TreeField(-1),
          BridgeField(-1), ShoreField(-1), LabelField(Label::UnreachedLabel), AgeField(-1) {}
    
    // Method to check if the vertex is reached
    bool IsReached() const {
        return LabelField != Label::UnreachedLabel;
    }

    // Method to check if the vertex is reached
    bool IsEven() const {
        return LabelField == Label::EvenLabel;
    }

    // Method to check if the vertex is reached
    bool IsOdd() const {
        return LabelField == Label::OddLabel;
    }

    // Utility function to print vertex information
    void print() const {
        std::cout << "BlossomField: " << ParentField << ", "
                  << "RankField: " << RankField << ", "
                  << "TreeField: " << TreeField << ", "
                  << "BridgeField: " << BridgeField << ", "
                  << "ShoreField: " << ShoreField << ", "
                  << "LabelField: " << LabelField << ", "
                  << "AgeField: " << AgeField << ", "
                  << "IsReached: " << IsReached() << ", "
                  << std::endl;

    }
};
#endif //VERTEX_H
