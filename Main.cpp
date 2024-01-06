// Copyright (C) 2023 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.
// SPDX-License-Identifier: BSD-2-Clause

#include "Graph.h"
#include "Matcher.h"

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
    std::vector<int64_t> matching;
    matching.resize(G.getN(),-1);
    Matcher::match<int64_t, std::string>(G,matching);
    //size_t N = indptr.size()-1;
    //size_t NNZ = indices.size();
    //printf("Undirected general graph |V|: %ld, |E|: %ld\n", N, NNZ);
    // Tracks whether a vertex has been in the stack, and which edge is next.
    //std::vector<int64_t> state(N,0);
    //std::list<int64_t> K;

    return 0;
}
