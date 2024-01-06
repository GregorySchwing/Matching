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
    std::vector<int64_t> matching;
    matching.resize(G.getN(),-1);
    auto match_start = high_resolution_clock::now();
    Matcher::match<int64_t, std::string>(G,matching);
    auto match_end = high_resolution_clock::now();
    auto duration = duration_cast<seconds>(match_end - match_start);
    std::cout << "Maximum matching time: "<< duration.count() << " seconds" << std::endl;
    return 0;
}
