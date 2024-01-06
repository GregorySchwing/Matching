// Copyright (C) 2023 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.
// SPDX-License-Identifier: BSD-2-Clause

#include <filesystem>
#include <fstream>
#include <iostream>
#include <numeric>
#include <vector>
#include <fast_matrix_market/fast_matrix_market.hpp>
#include <chrono>
using namespace std::chrono;
namespace fmm = fast_matrix_market;

template <typename IT, typename VT>
class Graph {
public:
    // Constructor
    Graph(const std::filesystem::path& in_path) {
        read_file(in_path);
    }

    // Other member functions...

private:
    void generateCSR(const std::vector<IT>& rows, const std::vector<IT>& columns, IT numVertices, std::vector<IT>& rowPtr, std::vector<IT>& colIndex) {
        rowPtr.resize(numVertices + 1, 0);
        colIndex.resize(2*rows.size());

        #pragma omp parallel for
        for (IT i = 0; i < rows.size(); ++i) {
            #pragma omp atomic
            rowPtr[rows[i] + 1]++;
            #pragma omp atomic
            rowPtr[columns[i] + 1]++;
        }

        for (IT i = 1; i <= numVertices; ++i) {
            rowPtr[i] += rowPtr[i - 1];
        }

        auto rowPtr_duplicate = rowPtr;

        #pragma omp parallel for
        for (IT i = 0; i < rows.size(); ++i) {
            IT source = rows[i];
            IT destination = columns[i];

            IT index;
            #pragma omp atomic capture
            index = rowPtr_duplicate[source]++;
            colIndex[index] = i;
            #pragma omp atomic capture
            index = rowPtr_duplicate[destination]++;
            colIndex[index] = i;
        }
    }

    void read_file(const std::filesystem::path& in_path) {
        fmm::matrix_market_header header;
        fmm::read_options options;
        options.generalize_symmetry = false;
        auto f_to_el_start = high_resolution_clock::now();
        std::ifstream f(in_path);
        fmm::read_matrix_market_triplet(f, header, original_rows, original_cols, original_vals, options);
        auto f_to_el_end = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(f_to_el_end - f_to_el_start);
        std::cout << "MTX to EdgeList conversion time: "<< duration.count() << " milliseconds" << std::endl;
        auto el_to_csr_start = high_resolution_clock::now();
        generateCSR(original_rows, original_cols, header.ncols, indptr, indices);
        auto el_to_csr_end = high_resolution_clock::now();
        duration = duration_cast<milliseconds>(el_to_csr_end - el_to_csr_start);
        // To get the value of duration use the count()
        // member function on the duration object
        std::cout << "EdgeList to CSR conversion time: "<< duration.count() << " milliseconds" << std::endl;
        std::cout << "Undirected general graph |V|: "<< indptr.size()-1 << " |E|: " << indices.size()/2 << std::endl;
    }

private:
    std::vector<IT> indptr;
    std::vector<IT> indices;
    std::vector<IT> original_rows;
    std::vector<IT> original_cols;
    std::vector<VT> original_vals;
};
