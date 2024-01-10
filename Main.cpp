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
    Graph<int64_t, std::string>  G(in_path);
    G.matching.resize(G.getN(),-1);
    auto match_start = high_resolution_clock::now();
    Matcher::match<int64_t, std::string>(G);
    auto match_end = high_resolution_clock::now();
    auto duration = duration_cast<seconds>(match_end - match_start);
    std::cout << "Maximum matching time: "<< duration.count() << " seconds" << std::endl;
    auto count = std::count_if(G.matching.begin(), G.matching.end(),[&](auto const& val){ return val > -1; });
    std::cout << "Maximum matching size: "<<  count/2 << std::endl;
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
    std::cout << "Maximum matching is valid." << std::endl;
    return 0;
}
