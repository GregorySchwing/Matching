// TraversalPolicies.h

#ifndef TRAVERSAL_POLICIES_H
#define TRAVERSAL_POLICIES_H
#include "Graph.h"
#include <iostream>

template <typename T>
class SerialPolicy {
public:
    bool pushEdgesOntoStack();
};

template <typename T>
class ParallelPolicy {
public:
    bool pushEdgesOntoStack();
};

#endif // TRAVERSAL_POLICIES_H
