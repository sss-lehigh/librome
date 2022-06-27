package(default_visibility = ["//visibility:public"])

cc_library(
    name = "rdma_accessor",
    hdrs = ["rdma_accessor.h"],
    deps = [],
)

cc_library(
    name = "rdma_messenger",
    hdrs = ["rdma_messenger.h"],
    deps = [],
)

cc_library(
    name = "twosided_messenger_impl",
    hdrs = ["twosided_messenger_impl.h"],
    deps = [],
)

cc_library(
    name = "twosided_messenger",
    hdrs = ["twosided_messenger.h"],
    deps = [
        ":rdma_messenger",
        ":twosided_messenger_impl",
        "//rome/logging",
        "//rome/rdma:rdma_memory",
        "//rome/util:status_util",
        "@absl//absl/status",
    ],
)

cc_library(
    name = "rdma_channel",
    srcs = ["rdma_channel.cc"],
    hdrs = ["rdma_channel.h"],
    deps = [
        ":rdma_accessor",
        ":rdma_messenger",
        "//rome/rdma:rdma_memory",
        "//rome/util:status_util",
        "@absl//absl/status",
    ],
)

cc_test(
    name = "twosided_messenger_test",
    srcs = ["twosided_messenger_test.cc"],
    deps = [
        ":rdma_accessor",
        ":rdma_channel",
        ":twosided_messenger",
        "//protos:testutil_cc_proto",
        "//rome/rdma:rdma_broker",
        "@gtest//:gtest_main",
    ],
)

cc_library(
    name = "sync_accessor",
    srcs = ["sync_accessor.cc"],
    hdrs = ["sync_accessor.h"],
    deps = [
        ":rdma_accessor",
        "//rome/rdma:rdma_util",
        "@absl//absl/status",
    ],
)
