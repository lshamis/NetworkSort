new_git_repository(
    name = "benchmark",
    remote = "https://github.com/google/benchmark.git",
    commit = "710c2b89d8c6e839e51ac148b4e840ce2c009dbb",
    build_file_content =
"""
cc_library(
    name = "benchmark",
    srcs = glob(["src/*.h", "src/*.cc"]),
    hdrs = glob(["include/benchmark/*.h"]),
    includes = [
        "include",
    ],
    visibility = ["//visibility:public"],
    copts = [
        "-DHAVE_STD_REGEX"
    ],
    linkopts = [
        "-pthread"
    ],
)
"""
)

