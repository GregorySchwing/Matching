#ifndef DSU2_H
#define DSU2_H
#include <vector>
#include <cassert>
#include "Vertex.h"
template<typename IT>
class DisjointSetUnionHelper {
public:
    static void reset(IT n, std::vector<Vertex<IT>> & vv);
    static Vertex<IT> find(Vertex<IT> a, std::vector<Vertex<IT>> & vv);
    static Vertex<IT> getBase(const Vertex<IT>& a, std::vector<Vertex<IT>> & vv);
    static void linkTo(Vertex<IT> a, Vertex<IT> b, std::vector<Vertex<IT>> & vv);
};

template<typename IT>
void DisjointSetUnionHelper<IT>::reset(IT n, std::vector<Vertex<IT>> & vv) {

}

template<typename IT>
IT DisjointSetUnionHelper<IT>::find(Vertex<IT> a, std::vector<Vertex<IT>> & vv) {
    return a.LinkField = (a.LinkField == link[a] ? a : find(link[a]));
}

template<typename IT>
Vertex<IT> DisjointSetUnionHelper<IT>::getBase(const Vertex<IT>& a, std::vector<Vertex<IT>> & vv) {

}

template<typename IT>
void DisjointSetUnionHelper<IT>::linkTo(Vertex<IT> a, Vertex<IT> b, std::vector<Vertex<IT>> & vv) {

}
/*
template<typename IT>
IT DisjointSetUnionHelper<IT>::find(IT a) {
    return link[a] = (a == link[a] ? a : find(link[a]));
}

template<typename IT>
IT DisjointSetUnionHelper<IT>::operator[](const IT& a) {
    return groupRoot[find(a)];
}

template<typename IT>
void DisjointSetUnionHelper<IT>::linkTo(IT a, IT b) {
    assert(directParent[a] == -1);
    assert(directParent[b] == -1);
    directParent[a] = b;
    a = find(a);
    b = find(b);
    IT gr = groupRoot[b];
    assert(a != b);

    if (size[a] > size[b])
        std::swap(a, b);
    link[b] = a;
    size[a] += size[b];
    groupRoot[a] = gr;
}
*/
#endif