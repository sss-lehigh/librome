package(default_visibility = ["//rome:__subpackages__"])

cc_library(
    name = "metric",
    hdrs = ["metric.h"],
    visibility = ["//visibility:public"],
    deps = ["@absl//absl/status:statusor"],
)

cc_library(
    name = "counter",
    hdrs = ["counter.h"],
    visibility = ["//visibility:public"],
    deps = [
        ":metric",
        "//protos:metrics_cc_proto",
        "@absl//absl/status:statusor",
    ],
)

cc_library(
    name = "summary",
    hdrs = ["summary.h"],
    visibility = ["//visibility:public"],
    deps = [
        ":metric",
        "//protos:metrics_cc_proto",
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
        "//protos:metrics_cc_proto",
        "//rome/testutil:status_matcher",
        "@absl//absl/status:statusor",
        "@gtest//:gtest_main",
    ],
)

cc_library(
    name = "stopwatch",
    srcs = ["stopwatch.cc"],
    hdrs = ["stopwatch.h"],
    visibility = ["//visibility:public"],
    deps = [
        ":metric",
        "//protos:metrics_cc_proto",
        "//rome/logging",
    ],
)

cc_test(
    name = "stopwatch_test",
    srcs = ["stopwatch_test.cc"],
    deps = [
        ":stopwatch",
        "//protos:metrics_cc_proto",
        "//rome/util:clocks",
        "@gtest//:gtest_main",
    ],
)
