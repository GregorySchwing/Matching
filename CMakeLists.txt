cmake_minimum_required(VERSION 3.11...3.25)
project(fmm_example VERSION 1.0.0)
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pg")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

# include fast_matrix_market
add_subdirectory(fast_matrix_market)

# simple example
add_executable(matcher Main.cpp)
target_include_directories(matcher PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
# Enable OpenMP
find_package(OpenMP REQUIRED)
if(OpenMP_CXX_FOUND)
    target_compile_options(matcher PRIVATE ${OpenMP_CXX_FLAGS})
    target_link_libraries(matcher PRIVATE fast_matrix_market::fast_matrix_market ${OpenMP_CXX_FLAGS})
else()
    target_link_libraries(matcher PRIVATE fast_matrix_market::fast_matrix_market)
endif()