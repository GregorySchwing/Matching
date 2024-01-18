Follow instructions from here to install 
https://github.com/google/benchmark

Compile
g++ -std=c++11 -O2 -I/usr/local/include/benchmark -I/home/greg/Matching/concurrentqueue -I/home/greg/Matching/src bmark.cpp /usr/local/lib/libbenchmark.a -lpthread -o mybenchmark
