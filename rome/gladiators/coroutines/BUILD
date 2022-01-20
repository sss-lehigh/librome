cc_library(
    name = "request_handler",
    hdrs = ["request_handler.h"],
    deps = ["//rome/util:thread_pool"],
)

cc_library(
    name = "c++20_coroutines",
    hdrs = ["c++20_coroutines.h"],
    deps = [
        ":request_handler",
        "//rome/util:coroutine",
        "//rome/util:thread_pool",
        "@absl//absl/status",
    ],
)

cc_library(
    name = "boost_coroutines",
    hdrs = ["boost_coroutines.h"],
    deps = [
        ":request_handler",
        "@boost//:coroutine2",
    ],
)

# cc_binary(
#     name = "main_boost",
#     srcs = ["main.cc"],
#     copts = ["-DUSE_BOOST_COROUTINES"],
#     deps = [
#         ":boost_coroutines",
#         ":c++20_coroutines",
#         "//rome/util:clocks",
#         "//rome/util:thread_pool",
#         "@absl//absl/status",
#     ],
# )

cc_binary(
    name = "main_c++20",
    srcs = ["main.cc"],
    deps = [
        ":boost_coroutines",
        ":c++20_coroutines",
        "//rome/util:clocks",
        "//rome/util:thread_pool",
        "@absl//absl/status",
    ],
)
