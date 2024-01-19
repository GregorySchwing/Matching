#ifndef THREAD_FACTORY2_H
#define THREAD_FACTORY2_H
#include <vector>
#include <thread>
#include <mutex>
#include <iostream>
#include <cassert>
#include <iostream>
#include <cstdint>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "Graph.h"
#include "Frontier.h"
#include "Matcher.h"

struct BenchResult {
  size_t num_readers;
  size_t written_messages;
  std::vector<size_t> read_messages;
  std::chrono::milliseconds duration;
};

void print_results(const BenchResult &results) {
  size_t tot_read_messages = 0;
  int ni = 0;
  for (size_t n : results.read_messages){
    printf("reader: \t\t %d read_messages %zu \n", ni++,n);
    tot_read_messages += n;
  }

  printf("duration: \t\t %zu millseconds\n", results.duration.count());
  printf("num_readers: \t\t %zu reader\n", results.num_readers);
  printf("written_msgs: \t\t %f message/millseconds\n",
         (float)results.written_messages / (results.duration.count()));
  printf("avg_read_msgs: \t\t %f message/millseconds\n",
         (float)(tot_read_messages / results.num_readers) /
             results.duration.count());
  printf("\n");
}


#include <iostream>
#include <vector>
#include <thread>
#include "concurrentqueue.h"

class ThreadFactory {
public:

    template <typename IT, typename VT>
    static bool create_threads_concurrentqueue_wl(std::vector<std::thread> &threads,
                                                    unsigned num_threads,
                                                    std::vector<size_t> &read_messages,
                                                    moodycamel::ConcurrentQueue<IT> &worklist,
                                                    Graph<IT, VT> &graph,
                                                    std::atomic<IT> & currentRoot,
                                                    std::atomic<bool>& finished_iteration,
                                                    std::atomic<bool>& incremented_iteration,
                                                    bool &finished_algorithm,
                                                    std::mutex & mtx,
                                                    std::condition_variable & cv,
                                                    std::atomic<IT> & num_enqueued,
                                                    std::atomic<IT> & num_dequeued,
                                                    std::atomic<IT> & num_running,
                                                    std::atomic<IT> & num_spinning,
                                                    std::vector<bool> &spinning,
                                                    std::vector<std::atomic<bool>> &atomicBoolVector);

};



template <typename IT, typename VT>
bool ThreadFactory::create_threads_concurrentqueue_wl(std::vector<std::thread> &threads,
                                                    unsigned num_threads,
                                                    std::vector<size_t> &read_messages,
                                                    moodycamel::ConcurrentQueue<IT> &worklist,
                                                    Graph<IT, VT> &graph,
                                                    std::atomic<IT> & currentRoot,
                                                    std::atomic<bool>& finished_iteration,
                                                    std::atomic<bool>& incremented_iteration,
                                                    bool &finished_algorithm,
                                                    std::mutex & mtx,
                                                    std::condition_variable & cv,
                                                    std::atomic<IT> & num_enqueued,
                                                    std::atomic<IT> & num_dequeued,
                                                    std::atomic<IT> & num_running,
                                                    std::atomic<IT> & num_spinning,
                                                    std::vector<bool> &spinning,
                                                    std::vector<std::atomic<bool>> &atomicBoolVector) {
    // Works, infers template types from args
    //Matcher::search(graph,0,*(frontiers[0]));
    for (unsigned i = 0; i < num_threads; ++i) {
        //threads[i] = std::thread(&Matcher::hello_world, i);
        threads[i] = std::thread( [&,i]{ Matcher::match_persistent_wl2<IT,VT>(graph,worklist,
          read_messages,finished_iteration,incremented_iteration,
          finished_algorithm,currentRoot,
          mtx,cv,i,num_enqueued,num_dequeued,num_running,num_spinning,spinning,atomicBoolVector,num_threads); } );

        // Create a cpu_set_t object representing a set of CPUs. Clear it and mark
        // only CPU i as set.
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(i, &cpuset);
        int rc = pthread_setaffinity_np(threads[i].native_handle(),
                                        sizeof(cpu_set_t), &cpuset);
        if (rc != 0) {
            std::cerr << "Error calling pthread_setaffinity_np: " << rc << "\n";
        }
    }
  return true;
}


#endif