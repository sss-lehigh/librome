load("//:cc_toolchain_config.bzl", "clang_toolchain_config", "gcc_toolchain_config")
load("@hedron_compile_commands//:refresh_compile_commands.bzl", "refresh_compile_commands")

package(default_visibility = ["//visibility:public"])

filegroup(name = "empty")

refresh_compile_commands(
    name = "refresh_compile_commands",

    # Specify the targets of interest.
    # For example, specify a dict of targets and their arguments:
    targets = {
        "//...": "--features=clang_tidy_flags",
    },
    # For more details, feel free to look into refresh_compile_commands.bzl if you want.
)

clang_toolchain_config(name = "k8_clang_toolchain_config")

cc_toolchain(
    name = "k8_clang_toolchain",
    all_files = ":empty",
    compiler_files = ":empty",
    dwp_files = ":empty",
    linker_files = ":empty",
    objcopy_files = ":empty",
    strip_files = ":empty",
    supports_param_files = 0,
    toolchain_config = ":k8_clang_toolchain_config",
    toolchain_identifier = "k8-toolchain",
)

cc_toolchain(
    name = "darwin_clang_toolchain",
    all_files = ":empty",
    compiler_files = ":empty",
    dwp_files = ":empty",
    linker_files = ":empty",
    objcopy_files = ":empty",
    strip_files = ":empty",
    supports_param_files = 0,
    toolchain_config = ":k8_clang_toolchain_config",
    toolchain_identifier = "k8-toolchain",
)

cc_toolchain_suite(
    name = "clang_suite",
    toolchains = {
        "k8": ":k8_clang_toolchain",
        "darwin" : "darwin_clang_toolchain",
    },
)

gcc_toolchain_config(name = "k8_gcc_toolchain_config")

cc_toolchain(
    name = "k8_gcc_toolchain",
    all_files = ":empty",
    compiler_files = ":empty",
    dwp_files = ":empty",
    linker_files = ":empty",
    objcopy_files = ":empty",
    strip_files = ":empty",
    supports_param_files = 0,
    toolchain_config = ":k8_gcc_toolchain_config",
    toolchain_identifier = "k8-toolchain",
)

cc_toolchain_suite(
    name = "gcc_suite",
    toolchains = {
        "k8": ":k8_gcc_toolchain",
        "darwin"  : "k8_gcc_toolchain",
    },
)
