load("@rules_cc//cc:defs.bzl", "cc_proto_library")
load("@com_google_protobuf//:protobuf.bzl", "py_proto_library")

package(default_visibility = ["//visibility:public"])

proto_library(
    name = "colosseum_proto",
    srcs = ["colosseum.proto"],
    deps = [":metrics_proto"],
)

cc_proto_library(
    name = "colosseum_cc_proto",
    deps = ["colosseum_proto"],
)

py_proto_library(
    name = "colosseum_py_proto",
    srcs = ["colosseum.proto"],
    deps = [":metrics_py_proto"],
)

proto_library(
    name = "metrics_proto",
    srcs = ["metrics.proto"],
)

cc_proto_library(
    name = "metrics_cc_proto",
    deps = [":metrics_proto"],
)

py_proto_library(
    name = "metrics_py_proto",
    srcs = ["metrics.proto"],
)

proto_library(
    name = "testutil_proto",
    srcs = ["testutil.proto"],
)

cc_proto_library(
    name = "testutil_cc_proto",
    deps = [":testutil_proto"],
)
