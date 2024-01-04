// Copyright (C) 2023 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.
// SPDX-License-Identifier: BSD-2-Clause

#include <filesystem>
#include <fstream>
#include <iostream>
#include <numeric>
#include <fast_matrix_market/fast_matrix_market.hpp>
// If you prefer more concise code then you can use this to abbreviate the
// rather long namespace name. Then just use `fmm::` instead of `fast_matrix_market::`.
// This is not used below only to make the code simpler to understand at a glance and for easy copy/paste.
namespace fmm = fast_matrix_market;

// Function to generate CSR from separate vectors for rows and columns in parallel
template <typename T>
void generateCSR(const std::vector<T>& rows, const std::vector<T>& columns, T numVertices, std::vector<T>& rowPtr, std::vector<T>& colIndex) {
    // Resize CSR vectors
    rowPtr.resize(2*numVertices + 1, 0);
    colIndex.resize(2*rows.size());

    // Count the number of edges incident to each vertex
    #pragma omp parallel for
    for (T i = 0; i < rows.size(); ++i) {
        #pragma omp atomic
        rowPtr[rows[i] + 1]++;
    }
    // Cumulative sum to get the row pointers
    for (T i = 1; i <= numVertices; ++i) {
        rowPtr[i] += rowPtr[i - 1];
    }

    auto rowPtr_duplicate = rowPtr;
    // Fill the colIndex array
    #pragma omp parallel for
    for (T i = 0; i < rows.size(); ++i) {
        T source = rows[i];
        T destination = columns[i];

        T index;
        #pragma omp atomic capture
        index = rowPtr_duplicate[source]++;

        colIndex[index] = destination;
    }

    // Copy values of colIndex from [0, rows.size() - 1] to [rows.size(), 2 * rows.size() - 1]
    #pragma omp parallel for
    for (T i = 0; i < rows.size(); ++i) {
        colIndex[rows.size() + i] = colIndex[i];
    }

    // Copy values of colIndex from [0, rows.size() - 1] to [rows.size(), 2 * rows.size() - 1]
    #pragma omp parallel for
    for (T i = 0; i < rows.size(); ++i) {
        colIndex[i] = colIndex[i]+numVertices;
    }

    // Copy values of rowPtr from [0, numVertices] to [numVertices, 2 * numVertices - 1] and add rows.size()
    #pragma omp parallel for
    for (T i = 0; i <= numVertices; ++i) {
        rowPtr[numVertices + i] = rowPtr[i] + rows.size();
    }
}

template <typename IT, typename VT>
void read_file(const std::filesystem::path& in_path,    
    std::vector<IT> &indptr,
    std::vector<IT> &indices) {
    std::vector<IT> original_rows;
    std::vector<IT> original_cols;
    std::vector<VT> original_vals;

    fmm::matrix_market_header header;
    
    // Load
    {
        fmm::read_options options;
        options.generalize_symmetry = true;
        std::ifstream f(in_path);
        fmm::read_matrix_market_triplet(f, header, original_rows, original_cols, original_vals, options);
    }

    generateCSR<IT>(original_rows, original_cols,header.ncols,indptr,indices);
    /*
    // Using a regular for loop
    std::cout << "Using a regular for loop:" << std::endl;
    for (size_t i = 0; i < indptr.size(); ++i) {
        std::cout << indptr[i] << " ";
    }
    std::cout << std::endl;
    for (size_t i = 0; i < indices.size(); ++i) {
        std::cout << indices[i] << " ";
    }
    std::cout << std::endl;
    
    {
        std::vector<IT>vals(header.nnz*4,1);
        fmm::matrix_market_header header2;
        header2=header;
        header2.nnz*=2;
        header2.ncols*=2;
        std::filesystem::path out_path{"test.mtx"};
        std::ofstream f(out_path);
        fast_matrix_market::write_options options;
        fast_matrix_market::write_matrix_market_csc(f,
                                                    {2*header.ncols, 2*header.ncols},
                                                    indptr, indices, vals,
                                                    true,
                                                    options);
    }
    */
    

}


#include <sys/time.h>
double getTimeOfDay()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;
}
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
    std::vector<int64_t> indptr;
    std::vector<int64_t> indices;

    double start_time_csc_2_g, end_time_csc_2_g;
    start_time_csc_2_g = getTimeOfDay();
    read_file<int64_t, std::string>(in_path, indptr, indices);
    end_time_csc_2_g = getTimeOfDay();
    printf("MTX to Graph conversion time: %f seconds\n", end_time_csc_2_g - start_time_csc_2_g);
    size_t N = indptr.size()-1;
    size_t NNZ = indices.size();
    printf("Undirected general graph |V|: %ld, |E|: %ld\n", N/2, NNZ/4);
    printf("Directed bipartite graph |V|: %ld, |E|: %ld\n", N, NNZ);
    std::vector<int64_t> matching(N/2,-1);
    // Tracks whether a vertex has been in the stack, and which edge is next.
    std::vector<int64_t> state(N,0);
    std::list<int64_t> K;
    std::list<int64_t> umatched_stack;
    // push all non-zero degree vertices onto stack
    for (int i = 0; i < N/2; ++i){
        if (indptr[i]!=indptr[i+1])
            umatched_stack.push_back(i);
    }

    return 0;
}
