// Copyright (C) 2023 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.
// SPDX-License-Identifier: BSD-2-Clause

#include "Graph.h"
#include "Matcher.h"
#include <chrono>
using namespace std::chrono;
#include <list>
int main(int argc, char **argv) {
    if (argc < 2) {
        std::cout << "Usage:" << std::endl;
        std::cout << argv[0] << " <file>.mtx" << std::endl;
        std::cout << std::endl;
        return 0;
    } else {
        std::cout << "READING " <<argv[1] << std::endl;
    }

    std::filesystem::path in_path{argv[1]};
    double start_time_csc_2_g, end_time_csc_2_g;
    Graph<int64_t, std::string>  G(in_path);

    // A map is used for the frontier to limit copying N vertices.
    //std::unordered_map<int64_t, Vertex<int64_t>> vertexMap;
    // A vector is used for the frontier to allocate once all the memory ever needed.
    std::vector<Vertex<int64_t>> vertexVector;
    // std::list is used for its splicing efficiency
    // Transfers elements from one list to another. 
    // No elements are copied or moved, only the internal 
    // pointers of the list nodes are re-pointed.
    // https://en.cppreference.com/w/cpp/container/list/splice
    std::list<int64_t> stack;
    auto allocate_start = high_resolution_clock::now();
    G.matching.resize(G.getN(),-1);
    vertexVector.resize(G.getN());
    auto allocate_end = high_resolution_clock::now();
    auto duration_alloc = duration_cast<milliseconds>(allocate_end - allocate_start);
    std::cout << "|V|-vector memory allocation time: "<< duration_alloc.count() << " milliseconds" << std::endl;
    auto match_start = high_resolution_clock::now();
    Matcher::match<int64_t, std::string>(G,stack,vertexVector);
    auto match_end = high_resolution_clock::now();
    auto duration = duration_cast<seconds>(match_end - match_start);
    std::cout << "Maximum matching time: "<< duration.count() << " seconds" << std::endl;
    return 0;
}
