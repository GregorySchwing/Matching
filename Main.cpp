// Copyright (C) 2023 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.
// SPDX-License-Identifier: BSD-2-Clause
//#undef NDEBUG
#include <cassert>    // for assertions
#include "FileReader.h"
#include "Graph.h"
#include "Matcher.h"
#include <chrono>
using namespace std::chrono;
#include <list>
int main(int argc, char **argv) {
    if (argc < 2) {
        std::cout << "Usage:" << '\n';
        std::cout << argv[0] << " <file>.mtx" << '\n';
        std::cout << '\n';
        return 0;
    } else {
        std::cout << "READING " <<argv[1] << '\n';
    }

    std::filesystem::path in_path{argv[1]};
    FileReader<int64_t, std::string>  FR(in_path);
    //Graph<int64_t, std::string>  G(in_path);
    // A map is used for the frontier to limit copying N vertices.
    //std::unordered_map<int64_t, Vertex<int64_t>> vertexMap;
    // A vector is used for the frontier to allocate once all the memory ever needed.
    auto allocate_start = high_resolution_clock::now();
    Graph<int64_t, std::string>  G(FR.indptr,FR.indices,FR.original_rows,
    FR.original_cols,FR.original_vals,FR.N,FR.M);
    auto allocate_end = high_resolution_clock::now();
    auto duration_alloc = duration_cast<milliseconds>(allocate_end - allocate_start);
    std::cout << "Matching (|V|) memory allocation time: "<< duration_alloc.count() << " milliseconds" << '\n';
    // Assign all elements in the vector to false
    for (int i = 0;i<20;++i){
        Statistics<int64_t> stats(G.getN());
        for (auto& atomicBool : G.matching) {
            atomicBool.store(-1);
        }
        auto match_start = high_resolution_clock::now();
        Matcher::match_wl<int64_t, std::string>(G,stats);
        auto match_end = high_resolution_clock::now();
        auto duration = duration_cast<seconds>(match_end - match_start);
        std::cout << "Maximum matching time: "<< duration.count() << " seconds" << '\n';
        auto count = std::count_if(G.matching.begin(), G.matching.end(),[&](auto const& val){ return val > -1; });
        std::cout << "Maximum matching size: "<<  count/2 << '\n';
        // Writing data to file
        stats.write_file(argv[1]);
    }
    //Matcher::match_wl<int64_t, std::string>(G,stats);
    std::vector<int64_t> match_count(G.getM(),0);
    // Iterate through the matching vector and update the match_count array
    for (auto const& val : G.matching) {
        if (val > -1 && static_cast<size_t>(val) < match_count.size()) {
            // Increment the count at the specified index
            match_count[val]++;
        }
    }
    // Check if each value in match_count is either 0 or 2
    for (size_t i = 0; i < match_count.size(); ++i) {
        if (match_count[i] != 0 && match_count[i] != 2) {
            throw std::runtime_error("Error: Match count is not equal to 0 or 2");
        }
    }
    std::cout << "Maximum matching is valid." << '\n';


    return 0;
}
