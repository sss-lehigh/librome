package(default_visibility = ["//rome:__subpackages__"])

cc_library(
    name = "value",
    hdrs = ["value.h"],
    deps = ["//rome/util:macros"],
)

cc_library(
    name = "metadata",
    hdrs = ["metadata.h"],
    deps = [],
)

cc_library(
    name = "visitor",
    hdrs = ["visitor.h"],
    deps = [],
)

cc_library(
    name = "atree",
    hdrs = ["atree.h"],
    deps = [
        ":metadata",
        ":value",
        ":visitor",
        "@absl//absl/status:statusor",
    ],
)

cc_test(
    name = "atree_test",
    srcs = ["atree_test.cc"],
    deps = [
        ":atree",
        "//rome/testutil:status_matcher",
        "@absl//absl/status:statusor",
        "@gtest//:gtest_main",
    ],
)
