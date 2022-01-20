load("@bazel_skylib//rules:common_settings.bzl", "string_flag")

string_flag(
    name = "log_level",
    build_setting_default = "info",
)

config_setting(
    name = "trace",
    flag_values = {
        ":log_level": "trace",
    },
)

config_setting(
    name = "debug",
    flag_values = {
        ":log_level": "debug",
    },
)

config_setting(
    name = "info",
    flag_values = {
        ":log_level": "info",
    },
)

config_setting(
    name = "warn",
    flag_values = {
        ":log_level": "warn",
    },
)

config_setting(
    name = "error",
    flag_values = {
        ":log_level": "error",
    },
)

config_setting(
    name = "critical",
    flag_values = {
        ":log_level": "critical",
    },
)

config_setting(
    name = "off",
    flag_values = {
        ":log_level": "off",
    },
)

cc_library(
    name = "logging",
    hdrs = ["logging.h"],
    defines = select({
        ":trace": ["ROME_LOG_LEVEL=TRACE"],
        ":debug": ["ROME_LOG_LEVEL=DEBUG"],
        ":info": ["ROME_LOG_LEVEL=INFO"],
        ":warn": ["ROME_LOG_LEVEL=WARN"],
        ":error": ["ROME_LOG_LEVEL=ERROR"],
        ":critical": ["ROME_LOG_LEVEL=CRITICAL"],
        ":off": ["ROME_LOG_LEVEL=OFF"],
        "//conditions:default": ["ROME_LOG_LEVEL=INFO"],
    }),
    visibility = ["//visibility:public"],
    deps = [
        "@spdlog",
    ],
)

cc_test(
    name = "logging_test",
    srcs = ["logging_test.cc"],
    deps = [
        ":logging",
        "@gtest//:gtest_main",
    ],
)
