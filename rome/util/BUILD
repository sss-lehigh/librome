package(default_visibility = ["//visibility:public"])

cc_library(
    name = "duration_util",
    hdrs = ["duration_util.h"],
    deps = [
    ],
)

cc_test(
    name = "duration_util_test",
    srcs = ["duration_util_test.cc"],
    deps = [
        ":duration_util",
        "@gtest//:gtest_main",
    ],
)

cc_library(
    name = "clocks",
    hdrs = ["clocks.h"],
    deps = ["//rome/testutil:fake_clock"],
)

cc_library(
    name = "thread_pool",
    srcs = ["thread_pool.cc"],
    hdrs = ["thread_pool.h"],
    deps = [
        "//rome/logging",
        "@absl//absl/status",
    ],
)

cc_library(
    name = "status_util",
    hdrs = ["status_util.h"],
    deps = [
        "@absl//absl/status",
    ],
)

cc_test(
    name = "status_util_test",
    srcs = ["status_util_test.cc"],
    deps = [
        ":status_util",
        "//rome/testutil:status_matcher",
        "@absl//absl/status",
        "@absl//absl/status:statusor",
        "@gtest//:gtest_main",
    ],
)

cc_library(
    name = "coroutine",
    hdrs = ["coroutine.h"],
    deps = [
        "@absl//absl/status",
        "@rome//rome/logging",
    ],
)

cc_test(
    name = "coroutine_test",
    srcs = ["coroutine_test.cc"],
    deps = [
        ":coroutine",
        "@absl//absl/status",
        "@gtest//:gtest_main",
        "@rome//rome/logging",
    ],
)

cc_library(
    name = "macros",
    hdrs = ["macros.h"],
    deps = [],
)
