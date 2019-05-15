#include <random>
#include <vector>

#include "benchmark/benchmark.h"

#include "network_sort.h"

#define PPCAT_NX(A, B) A##B
#define PPCAT(A, B) PPCAT_NX(A, B)
#define STRINGIZE_NX(A) #A
#define STRINGIZE(A) STRINGIZE_NX(A)

#define BENCHMARK_COMPARE(SIZE)                                       \
                                                                      \
  static void PPCAT(BM_NetworkSort, SIZE)(benchmark::State & state) { \
    std::vector<int> data(SIZE);                                      \
    while (state.KeepRunning()) {                                     \
      std::generate(data.begin(), data.end(), std::rand);             \
      NetworkSort<SIZE>(data.begin());                                \
    }                                                                 \
  }                                                                   \
  BENCHMARK(BM_NetworkSort##SIZE);                                    \
                                                                      \
  static void PPCAT(BM_StdSort, SIZE)(benchmark::State & state) {     \
    std::vector<int> data(SIZE);                                      \
    while (state.KeepRunning()) {                                     \
      std::generate(data.begin(), data.end(), std::rand);             \
      std::sort(data.begin(), data.end());                            \
    }                                                                 \
  }                                                                   \
  BENCHMARK(BM_StdSort##SIZE);

BENCHMARK_COMPARE(2);
BENCHMARK_COMPARE(3);
BENCHMARK_COMPARE(4);
BENCHMARK_COMPARE(5);
BENCHMARK_COMPARE(6);
BENCHMARK_COMPARE(7);
BENCHMARK_COMPARE(8);
BENCHMARK_COMPARE(9);
BENCHMARK_COMPARE(10);
BENCHMARK_COMPARE(11);
BENCHMARK_COMPARE(12);
BENCHMARK_COMPARE(13);
BENCHMARK_COMPARE(14);
BENCHMARK_COMPARE(15);
BENCHMARK_COMPARE(16);
BENCHMARK_COMPARE(17);
BENCHMARK_COMPARE(18);
BENCHMARK_COMPARE(19);
BENCHMARK_COMPARE(20);
BENCHMARK_COMPARE(21);
BENCHMARK_COMPARE(22);
BENCHMARK_COMPARE(23);
BENCHMARK_COMPARE(24);
BENCHMARK_COMPARE(25);
BENCHMARK_COMPARE(26);
BENCHMARK_COMPARE(27);
BENCHMARK_COMPARE(28);
BENCHMARK_COMPARE(29);
BENCHMARK_COMPARE(30);
BENCHMARK_COMPARE(31);
BENCHMARK_COMPARE(32);

BENCHMARK_MAIN();
