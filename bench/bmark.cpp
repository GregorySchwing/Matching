#include <benchmark/benchmark.h>
#include <concurrentqueue.h>
#include <vector>
#include <deque>
#include <list>
#include <Vertex.h>
#include <Stack.h>
#include <Frontier.h>


template <typename T>
void BM_Push(benchmark::State& state) {
    moodycamel::ConcurrentQueue<T> worklist;

    for (auto _ : state) {
        worklist.enqueue(T(1000));
    }
}
//BENCHMARK_TEMPLATE(BM_Push, int);
BENCHMARK_TEMPLATE(BM_Push, std::vector<int>);
BENCHMARK_TEMPLATE(BM_Push, std::deque<int>);

BENCHMARK_TEMPLATE(BM_Push, Stack<Vertex<int>>);
BENCHMARK_TEMPLATE(BM_Push, std::vector<Vertex<int>>);
BENCHMARK_TEMPLATE(BM_Push, Frontier<int>);

BENCHMARK_TEMPLATE(BM_Push, std::deque<Vertex<int>>);
BENCHMARK_TEMPLATE(BM_Push, std::list<Vertex<int>>);
//BENCHMARK_TEMPLATE(BM_Push, std::unordered_set<Vertex<int>>);

template <typename T>
void BM_Pop(benchmark::State& state) {
    moodycamel::ConcurrentQueue<T> worklist;
    worklist.enqueue(T(1000));

    for (auto _ : state) {
        T item;
        if (worklist.try_dequeue(item)) {
            benchmark::DoNotOptimize(item);
        }
    }
}
//BENCHMARK_TEMPLATE(BM_Pop, int);
BENCHMARK_TEMPLATE(BM_Pop, std::vector<int>);
BENCHMARK_TEMPLATE(BM_Pop, std::deque<int>);

BENCHMARK_TEMPLATE(BM_Pop, Stack<Vertex<int>>);
BENCHMARK_TEMPLATE(BM_Pop, std::vector<Vertex<int>>);
BENCHMARK_TEMPLATE(BM_Pop, std::deque<Vertex<int>>);
BENCHMARK_TEMPLATE(BM_Pop, std::list<Vertex<int>>);
//BENCHMARK_TEMPLATE(BM_Pop, std::unordered_set<Vertex<int>>);

BENCHMARK_MAIN();
