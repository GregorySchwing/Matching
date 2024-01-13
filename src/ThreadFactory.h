#ifndef THREAD_FAC_H
#define THREAD_FAC_H
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
#include "broadcast_queue.h"
#include "semaphore_waiting_strategy.h"
#include "concurrentqueue.h"

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
  printf("written_msgs: \t\t %zu message/sec\n",
         results.written_messages / (results.duration.count() / 1000));
  printf("avg_read_msgs: \t\t %zu message/sec\n",
         (tot_read_messages / results.num_readers) /
             (results.duration.count() / 1000));
  printf("\n");
}

void create_threads_bcast_queue(std::vector<std::thread> &threads, unsigned num_threads){
  int capacity = 1024;
  std::vector<size_t> read_messages;
  read_messages.resize(num_threads);
  static constexpr size_t CAPACITY = 1024;
  bool should_stop = false;

  auto duration = std::chrono::milliseconds(10000);
  broadcast_queue::sender<int,broadcast_queue::semaphore_waiting_strategy> send{CAPACITY};

  size_t written_messages = 0;

  std::thread sender_thread{[&]() {
    while (!should_stop) {
      send.push(written_messages++);
    }
  }};
  cpu_set_t my_set;
  CPU_ZERO(&my_set);
  CPU_SET(0, &my_set);
  if (sched_setaffinity(0, sizeof(cpu_set_t), &my_set)) {
    std::cout << "sched_setaffinity error: " << strerror(errno) << std::endl;
  }


  for (unsigned i = 1; i < num_threads+1; ++i) {
    threads[i-1] = std::thread([&, i] {
      auto receiver = send.subscribe();

      while (!should_stop) {
        broadcast_queue::Error error;
        int msg;

        error = receiver.try_dequeue(&msg);
        if (error == broadcast_queue::Error::None)
          read_messages[i-1]++;
      }
    });

    // Create a cpu_set_t object representing a set of CPUs. Clear it and mark
    // only CPU i as set.
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(i, &cpuset);
    int rc = pthread_setaffinity_np(threads[i-1].native_handle(),
                                    sizeof(cpu_set_t), &cpuset);
    if (rc != 0) {
      std::cerr << "Error calling pthread_setaffinity_np: " << rc << "\n";
    }
  }

  std::this_thread::sleep_for(duration);
  should_stop = true;

  sender_thread.join();

  print_results(BenchResult{num_threads, written_messages, read_messages, duration});
}

void create_threads_concurrentqueue(std::vector<std::thread> &threads, unsigned num_threads){
  std::vector<size_t> read_messages;
  read_messages.resize(num_threads);
  moodycamel::ConcurrentQueue<int> q;
  bool should_stop = false;

  auto duration = std::chrono::milliseconds(2000);

  size_t written_messages = 0;

  std::thread sender_thread{[&]() {
    int u = 0;
    while (u++<100000) {
    //while (!should_stop) {
      q.enqueue(written_messages++);
    }
  }};
  cpu_set_t my_set;
  CPU_ZERO(&my_set);
  CPU_SET(0, &my_set);
  if (sched_setaffinity(0, sizeof(cpu_set_t), &my_set)) {
    std::cout << "sched_setaffinity error: " << strerror(errno) << std::endl;
  }


  for (unsigned i = 1; i < num_threads+1; ++i) {
    threads[i-1] = std::thread([&, i] {

      while (!should_stop) {
        int item;
        if(q.try_dequeue(item))
          read_messages[i-1]++;
      }
    });

    // Create a cpu_set_t object representing a set of CPUs. Clear it and mark
    // only CPU i as set.
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(i, &cpuset);
    int rc = pthread_setaffinity_np(threads[i-1].native_handle(),
                                    sizeof(cpu_set_t), &cpuset);
    if (rc != 0) {
      std::cerr << "Error calling pthread_setaffinity_np: " << rc << "\n";
    }
  }

  std::this_thread::sleep_for(duration);
  while(q.size_approx()){}

  should_stop = true;

  sender_thread.join();

  print_results(BenchResult{num_threads, written_messages, read_messages, duration});
}

#endif