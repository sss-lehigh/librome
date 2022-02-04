package(default_visibility = ["//visibility:public"])

cc_library(
    name = "status_matcher",
    hdrs = ["status_matcher.h"],
    deps = [
        "@absl//absl/status",
        "@absl//absl/status:statusor",
        "@gtest",
    ],
)

cc_library(
    name = "stream_matcher",
    hdrs = ["stream_matcher.h"],
    deps = [
        "//rome/colosseum:stream",
        "@absl//absl/strings",
        "@gtest",
    ],
)

cc_library(
    name = "fake_clock",
    hdrs = ["fake_clock.h"],
    deps = [],
)

cc_test(
    name = "fake_clock_test",
    srcs = ["fake_clock_test.cc"],
    deps = [
        ":fake_clock",
        "//rome/util:clocks",
        "@gtest//:gtest_main",
    ],
)
