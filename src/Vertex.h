#ifndef VERTEX_H
#define VERTEX_H
#include <iostream>
#include "Enums.h"
// Define the Vertex class template
template <typename IT>
class Vertex {
public:
    IT TreeField;
    IT BridgeField;
    IT ShoreField;
    IT AgeField;
    // For Reinitializing DSU Data
    IT LabelField;
    // DSU Data
    IT LinkField;
    IT SizeField;
    IT GroupRootField;
    IT DirectParentField;
    // Constructor
    Vertex(IT Label)
        : TreeField(-1),
          BridgeField(-1), 
          ShoreField(-1), 
          AgeField(-1), 
          // Wrong val
          LabelField(Label),
          // Wrong val
          LinkField(Label),
          SizeField(1), 
          // Wrong val
          GroupRootField(Label),
          DirectParentField(-1) {}

    // Copy constructor
    Vertex(const Vertex& other)
        : TreeField(other.TreeField), 
          BridgeField(other.BridgeField),
          ShoreField(other.ShoreField), 
          AgeField(other.AgeField),
          // Wrong val
          LabelField(-1),
          LinkField(other.LinkField),
          SizeField(other.SizeField), 
          GroupRootField(other.GroupRootField), 
          DirectParentField(other.DirectParentField) {}
          

    // Default constructor
    Vertex() : TreeField(-1),
          BridgeField(-1), 
          ShoreField(-1), 
          AgeField(-1),
          // Wrong val
          LabelField(-1),
          // Wrong val
          LinkField(-1),
          SizeField(1), 
          // Wrong val
          GroupRootField(-1),
          DirectParentField(-1) {}
    
    // Method to check if the vertex is reached
    bool IsReached() const {
        return AgeField != -1;
    }

    // Method to check if the vertex is reached
    bool IsEven() const {
        return AgeField % 2 == 0;
    }

    // Method to check if the vertex is reached
    bool IsOdd() const {
        return AgeField % 2;
    }

    // Utility function to print vertex information
    void print() const {
        std::cout 
                  << "TreeField: " << TreeField << ", "
                  << "BridgeField: " << BridgeField << ", "
                  << "ShoreField: " << ShoreField << ", "
                  << "AgeField: " << AgeField << ", "
                  << "LabelField: " << LabelField << ", "
                  << "LinkField: " << LinkField << ", "
                  << "SizeField: " << SizeField << ", "
                  << "GroupRootField: " << GroupRootField << ", "
                  << "DirectParentField: " << DirectParentField << ", "
                  << "IsReached: " << IsReached() << ", "
                  << std::endl;

    }
};
#endif //VERTEX_H
