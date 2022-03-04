package(default_visibility = ["//visibility:public"])

cc_library(
    name = "rdma_util",
    hdrs = ["rdma_util.h"],
    deps = [
        "//rome/logging",
        "//rome/util:status_util",
    ],
)

cc_library(
    name = "rdma_device",
    srcs = ["rdma_device.cc"],
    hdrs = ["rdma_device.h"],
    linkopts = ["-libverbs"],
    deps = [
        ":rdma_util",
        "@absl//absl/status",
        "@absl//absl/status:statusor",
        "//rome/logging",
        "//rome/util:status_util",
    ],
)

cc_test(
    name = "rdma_device_test",
    srcs = ["rdma_device_test.cc"],
    deps = [
        ":rdma_device",
        "@gtest//:gtest_main",
        "//rome/testutil:status_matcher",
    ],
)

cc_library(
    name = "rdma_memory",
    srcs = ["rdma_memory.cc"],
    hdrs = ["rdma_memory.h"],
    linkopts = ["-libverbs"],
    deps = [
        ":rdma_util",
        "@absl//absl/status",
        "@absl//absl/status:statusor",
        "//rome/logging",
        "//rome/util:status_util",
    ],
)

cc_test(
    name = "rdma_memory_test",
    srcs = ["rdma_memory_test.cc"],
    deps = [
        ":rdma_device",
        ":rdma_memory",
        "@absl//absl/status",
        "@gtest//:gtest_main",
        "//rome/testutil:status_matcher",
    ],
)

cc_library(
    name = "rdma_receiver",
    hdrs = ["rdma_receiver.h"],
    linkopts = [
        "-lrdmacm",
    ],
    deps = [
        "@absl//absl/status",
    ],
)

cc_library(
    name = "rdma_client",
    hdrs = ["rdma_client.h"],
    deps = [
        ":rdma_util",
        "@absl//absl/status",
    ],
)

cc_library(
    name = "rdma_broker",
    srcs = ["rdma_broker.cc"],
    hdrs = ["rdma_broker.h"],
    linkopts = [
        "-libverbs",
        "-lrdmacm",
    ],
    deps = [
        ":rdma_device",
        ":rdma_receiver",
        ":rdma_util",
        "@absl//absl/status",
        "//rome/logging",
        "//rome/util:coroutine",
        "//rome/util:status_util",
    ],
)

cc_test(
    name = "rdma_broker_test",
    srcs = ["rdma_broker_test.cc"],
    deps = [
        ":rdma_broker",
        ":rdma_client",
        ":rdma_receiver",
        "@gtest//:gtest_main",
        "//rome/testutil:status_matcher",
        "//rome/util:coroutine",
        "//rome/util:status_util",
    ],
)
