Follow instructions from here to install 
https://github.com/google/benchmark

git clone https://github.com/google/benchmark.git
cd benchmark
cmake -E make_directory "build"
cmake -E chdir "build" cmake -DBENCHMARK_DOWNLOAD_DEPENDENCIES=on -DCMAKE_BUILD_TYPE=Release ../
cmake --build "build" --config Release
cmake -E chdir "build" ctest --build-config Release
sudo cmake --build "build" --config Release --target install

Compile
g++ -std=c++11 -O2 -I/usr/local/include/benchmark -I/home/greg/Matching/concurrentqueue -I/home/greg/Matching/src bmark.cpp /usr/local/lib/libbenchmark.a -lpthread -o mybenchmark
