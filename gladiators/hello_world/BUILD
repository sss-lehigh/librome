cc_binary(
    name = "main",
    srcs = ["main.cc"],
    deps = [
        "//rome/colosseum:client_adaptor",
        "//rome/colosseum:qps_controller",
        "//rome/colosseum:workload_driver",
        "//rome/colosseum:streams",
        "//rome/metrics:summary",
        "@absl//absl/status:status",
        "@absl//absl/status:statusor",
        "@absl//absl/flags:flag",
        "@absl//absl/flags:parse",
    ],
)
