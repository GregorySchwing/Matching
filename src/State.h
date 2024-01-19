#ifndef STATE_H
#define STATE_H
#include <vector>
#include "Vertex.h"

template <typename IT>
class State  {
public:    
    State();

    State(size_t N, size_t M);
    void clear();
    // Other member functions...
    std::vector<IT> stack;
    std::vector<Vertex<IT>> tree;
    IT time;
};
// Constructor
template <typename IT>
State<IT>::State():time(0){
}
// Constructor
template <typename IT>
State<IT>::State(size_t N, size_t M): tree(N),stack(M),time(0){
}

template <typename IT>
void State<IT>::clear(){
    stack.clear();
    tree.clear();
}

#endif