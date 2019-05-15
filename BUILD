cc_library(
  name = "network_sort",
  hdrs = ["network_sort.h"],
)

cc_binary(
  name = "run_benchmarks",
  srcs = ["run_benchmarks.cc"],
  deps = [
    ":network_sort",
    "@com_github_google_benchmark//:benchmark_main",
  ],
)

