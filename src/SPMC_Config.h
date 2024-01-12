#include <bits/stdc++.h>
#include "SPMCQueue.h"
#ifndef SPMC_Config_H
#define SPMC_Config_H

using namespace std;

struct Msg
{
  uint64_t tsc;
  uint64_t E_ndx;
  uint64_t V_ndx;
};

inline uint64_t rdtsc() {
  return __builtin_ia32_rdtsc();
}

bool cpupin(int cpuid) {
  cpu_set_t my_set;
  CPU_ZERO(&my_set);
  CPU_SET(cpuid, &my_set);
  if (sched_setaffinity(0, sizeof(cpu_set_t), &my_set)) {
    std::cout << "sched_setaffinity error: " << strerror(errno) << std::endl;
    return false;
  }

  return true;
}

//const uint64_t MaxI = 1000000;

#endif