#ifndef BENCH_H
#define BENCH_H

#include "Graph.h"
#include "Vertex.h"
#include <list>
#include <unordered_map>
#include "Enums.h"
#include "DSU.h"
#include "Blossom.h"
#include "Stack.h"
#include "Frontier.h"
#include "Statistics.h"
// Parallel
#include "ThreadFactory.h"
#include <cassert>


class Bench {
public:

    template <typename IT, typename VT>
    static void bench(Graph<IT, VT>& graph);

private:
    template <typename IT, typename VT>
    static void bench_parallel(Graph<IT, VT>& graph);
};


template <typename IT, typename VT>
void Bench::bench_parallel(Graph<IT, VT>& graph) {

  constexpr unsigned num_threads = 4;
  std::vector<std::thread> threads(num_threads);
  create_threads_bcast_queue(threads, num_threads);

  for (auto& t : threads) {
    t.join();
  }
  create_threads_concurrentqueue(threads, num_threads);

  for (auto& t : threads) {
    t.join();
  }
  return ;
}


#endif
