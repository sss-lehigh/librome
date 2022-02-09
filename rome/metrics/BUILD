package(default_visibility = ["//rome:__subpackages__"])

cc_library(
    name = "metric",
    hdrs = ["metric.h"],
    deps = ["@absl//absl/status:statusor"],
)

cc_library(
    name = "counter",
    hdrs = ["counter.h"],
    deps = [
        ":metric",
        "@absl//absl/status:statusor",
    ],
)

cc_library(
    name = "summary",
    hdrs = ["summary.h"],
    deps = [
        ":metric",
        "//rome/internal/atree",
        "@absl//absl/status:statusor",
    ],
)

cc_test(
    name = "metric_test",
    srcs = ["metric_test.cc"],
    deps = [
        ":counter",
        ":metric",
        ":summary",
        "//rome/testutil:status_matcher",
        "@absl//absl/status:statusor",
        "@gtest//:gtest_main",
    ],
)

cc_library(
    name = "stopwatch",
    srcs = ["stopwatch.cc"],
    hdrs = ["stopwatch.h"],
    deps = [
        ":metric",
        "//rome/logging",
    ],
)

cc_test(
    name = "stopwatch_test",
    srcs = ["stopwatch_test.cc"],
    deps = [
        ":stopwatch",
        "//rome/util:clocks",
        "@gtest//:gtest_main",
    ],
)
