package(default_visibility = ["//rome:__subpackages__"])

cc_library(
    name = "fixed_heap",
    hdrs = ["fixed_heap.h"],
    deps = ["@absl//absl/status:statusor"],
)

cc_test(
    name = "fixed_heap_test",
    srcs = ["fixed_heap_test.cc"],
    deps = [
        ":fixed_heap",
        "//rome/testutil:status_matcher",
        "@absl//absl/status:statusor",
        "@gtest//:gtest_main",
    ],
)
