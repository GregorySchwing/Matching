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
void generateCSR(const std::vector<T>& rows, const std::vector<T>& columns, T numVertices, std::vector<T>& indptr, std::vector<T>& indices) {
    std::vector<T> degrees(numVertices, 0);
    // Count the number of edges incident to each vertex
    // rows and columns are currently indexed from 0
    #pragma omp parallel for
    for (T i = 0; i < rows.size(); ++i) {
        #pragma omp atomic
        degrees[rows[i]]++;
    }
    
    T nonZeroCount = 0;
    #pragma omp parallel for reduction(+:nonZeroCount)
    for (int i = 0; i < degrees.size(); ++i) {
        if (degrees[i]) {
            nonZeroCount++;
        }
    }

    // Resize CSR vectors
    // If you dont want s,t
    //indptr.resize(2*numVertices + 1, 0);
    //indices.resize(2*rows.size());

    indptr.resize(2*numVertices + 1 + 2, 0);
    indices.resize(2*rows.size() + 2*nonZeroCount);

    // Count the number of edges incident to each vertex
    #pragma omp parallel for
    for (T i = 0; i < rows.size(); ++i) {
        #pragma omp atomic
        indptr[rows[i] + 1]++;
    }
    // Cumulative sum to get the row pointers
    for (T i = 1; i <= numVertices; ++i) {
        indptr[i] += indptr[i - 1];
    }

    auto rowPtr_duplicate = indptr;
    // Fill the indices array
    #pragma omp parallel for
    for (T i = 0; i < rows.size(); ++i) {
        T source = rows[i];
        T destination = columns[i];

        T index;
        #pragma omp atomic capture
        index = rowPtr_duplicate[source]++;

        indices[index] = destination;
    }

    // Copy values of indices from [0, rows.size() - 1] to [rows.size(), 2 * rows.size() - 1]
    #pragma omp parallel for
    for (T i = 0; i < rows.size(); ++i) {
        indices[rows.size() + i] = indices[i];
    }

    // Copy values of indices from [0, rows.size() - 1] to [rows.size(), 2 * rows.size() - 1]
    #pragma omp parallel for
    for (T i = 0; i < rows.size(); ++i) {
        indices[i] = indices[i]+numVertices;
    }

    // Copy values of indptr from [0, numVertices] to [numVertices, 2 * numVertices - 1] and add rows.size()
    #pragma omp parallel for
    for (T i = 0; i <= numVertices; ++i) {
        indptr[numVertices + i] = indptr[i] + rows.size();
    }

    // Add S
    indptr[2*numVertices + 1] = indptr[2*numVertices]+nonZeroCount;
    // Add T
    indptr[2*numVertices + 2] = indptr[2*numVertices+1]+nonZeroCount;

    int atomicIndex = 0;
    int startOfS = indptr[2*numVertices];
    int startOfT = indptr[2*numVertices + 1];

    #pragma omp parallel for
    for (int i = 0; i < degrees.size(); ++i) {
        if (degrees[i]) {
            int currentIndex;
            #pragma omp atomic capture
            currentIndex = atomicIndex++;

            indices[startOfS+currentIndex] = i;
            indices[startOfT+currentIndex] = i+numVertices;
        }
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
    printf("Directed bipartite graph |V|: %ld, |E|: %ld\n", N, NNZ);
    std::vector<int64_t> matching(N/2,-1);
    // Tracks whether a vertex has been in the stack, and which edge is next.
    std::vector<int64_t> state(N,0);
    std::list<int64_t> K;

    return 0;
}
