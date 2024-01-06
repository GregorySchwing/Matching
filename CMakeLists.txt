cmake_minimum_required(VERSION 3.11...3.25)
project(fmm_example VERSION 1.0.0)

# include fast_matrix_market
add_subdirectory(fast_matrix_market)

# simple example
add_executable(read1 main.cpp)
# Enable OpenMP
find_package(OpenMP REQUIRED)
if(OpenMP_CXX_FOUND)
    target_compile_options(read1 PRIVATE ${OpenMP_CXX_FLAGS})
    target_link_libraries(read1 PRIVATE fast_matrix_market::fast_matrix_market ${OpenMP_CXX_FLAGS})
else()
    target_link_libraries(read1 PRIVATE fast_matrix_market::fast_matrix_market)
endif()