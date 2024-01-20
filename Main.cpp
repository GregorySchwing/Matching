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
#include <cxxopts.hpp>
typedef int32_t INDEX_TYPE;

int main(int argc, char **argv) {
    cxxopts::Options options("matcher", "A multi-threaded DFS-based Edmonds Blossom solver.");
    options.add_options()
        ("file", "Input file", cxxopts::value<std::string>())
        ("iterations", "Number of iterations", cxxopts::value<int>()->default_value("1"))
        ("threads", "Number of threads", cxxopts::value<int>()->default_value("1"));

    options.parse_positional({"file", "iterations", "threads"});
    auto result = options.parse(argc, argv);

    if (!result.count("file")) {
        std::cout << "Usage: " << argv[0] << " <file>.mtx <num_iterations> <num_threads>" << '\n';
        return 0;
    } else {
        std::cout << "READING " << result["file"].as<std::string>() << '\n';
    }

    std::filesystem::path in_path{result["file"].as<std::string>()};
    FileReader<INDEX_TYPE, std::string>  FR(in_path);
    //Graph<int64_t, std::string>  G(in_path);
    // A map is used for the frontier to limit copying N vertices.
    //std::unordered_map<int64_t, Vertex<int64_t>> vertexMap;
    // A vector is used for the frontier to allocate once all the memory ever needed.
    auto allocate_start = high_resolution_clock::now();
    Graph<INDEX_TYPE, std::string>  G(FR.indptr,FR.indices,FR.original_rows,
    FR.original_cols,FR.original_vals,FR.N,FR.M);
    auto allocate_end = high_resolution_clock::now();
    auto duration_alloc = duration_cast<milliseconds>(allocate_end - allocate_start);
    std::cout << "Matching (|V|) memory allocation time: "<< duration_alloc.count() << " milliseconds" << '\n';
    // Assign all elements in the vector to false
    // Read the number of iterations and number of threads from cxxopts results
    int num_iters = result["iterations"].as<int>();
    int num_threads = result["threads"].as<int>();
    std::vector<double> matching_times; // To store matching times for each iteration

    for (int i = 0;i<num_iters;++i){
        for (auto& atomicBool : G.matching) {
            atomicBool.store(-1);
        }
        auto match_start = high_resolution_clock::now();
        if (num_threads==1){
            //Matcher::match<int64_t, std::string>(G,stats);
            Matcher::match<INDEX_TYPE, std::string>(G);
        } else {
            Matcher::match_wl<INDEX_TYPE, std::string>(G,num_threads);
        }
        auto match_end = high_resolution_clock::now();
        auto duration = duration_cast<seconds>(match_end - match_start);
        std::cout << "Maximum matching time: "<< duration.count() << " seconds" << '\n';
        auto count = std::count_if(G.matching.begin(), G.matching.end(),[&](auto const& val){ return val > -1; });
        std::cout << "Maximum matching size: "<<  count/2 << '\n';
        matching_times.push_back(duration.count());
        //Statistics<INDEX_TYPE> stats(G.getN());
        //stats.write_file(argv[1]);
    }
    // Calculate mean and standard deviation
    double mean = std::accumulate(matching_times.begin(), matching_times.end(), 0.0) / matching_times.size();
    double stdev = 0.0;
    for (const auto& time : matching_times) {
        stdev += std::pow(time - mean, 2);
    }
    stdev = std::sqrt(stdev / matching_times.size());

    // Print mean and standard deviation
    std::cout << "Mean matching time: " << mean << " seconds" << '\n';
    std::cout << "Standard deviation: " << stdev << " seconds" << '\n';

    //Matcher::match_wl<int64_t, std::string>(G,stats);
    std::vector<INDEX_TYPE> match_count(G.getM(),0);
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
