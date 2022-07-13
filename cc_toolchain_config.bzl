""" Implements a custom Clang and GCC toolchain.  """

load(
    "@bazel_tools//tools/cpp:cc_toolchain_config_lib.bzl",
    "feature",
    "flag_group",
    "flag_set",
    "tool_path",
    "with_feature_set",
)
load("@bazel_tools//tools/build_defs/cc:action_names.bzl", "ACTION_NAMES")

all_cpp_link_actions = [
    ACTION_NAMES.cpp_link_executable,
    ACTION_NAMES.cpp_link_dynamic_library,
    ACTION_NAMES.cpp_link_nodeps_dynamic_library,
]

all_cpp_compile_actions = [
    ACTION_NAMES.cpp_compile,
    ACTION_NAMES.cpp_header_parsing,
    ACTION_NAMES.cpp_module_codegen,
    ACTION_NAMES.cpp_module_compile,
]

all_cpp_actions = [
    ACTION_NAMES.cpp_compile,
    ACTION_NAMES.cpp_header_parsing,
    ACTION_NAMES.cpp_module_codegen,
    ACTION_NAMES.cpp_module_compile,
    ACTION_NAMES.cpp_link_executable,
    ACTION_NAMES.cpp_link_dynamic_library,
    ACTION_NAMES.cpp_link_nodeps_dynamic_library,
]

common_compile_flags = [
    "-xc++",
    "-Wall",
    "-std=c++20",
]

common_dbg_flags = [
    "-fno-omit-frame-pointer",
    "-fno-inline",
    "-O0",
    "-ggdb3",
]

common_opt_flags = [
    "-O3",
    "-DNDEBUG",
]

common_linker_flags = [
    "-lstdc++",
    "-lm",
]

clang_compile_flags = [
    # "-Werror",
    "-stdlib=libc++",
    "-fcoroutines-ts",
]

gcc_compile_flags = [
    "-Wno-comment",
    "-fcoroutines",
]

def _clang_impl(ctx):
    tool_paths = [
        tool_path(
            name = "gcc",
            path = "/usr/bin/clang",
        ),
        tool_path(
            name = "ld",
            path = "/usr/bin/ld",
        ),
        tool_path(
            name = "ar",
            path = "/usr/bin/ar",
        ),
        tool_path(
            name = "cpp",
            path = "/usr/bin/clang++",
        ),
        tool_path(
            name = "gcov",
            path = "/usr/bin/gcov",
        ),
        tool_path(
            name = "nm",
            path = "/usr/bin/nm",
        ),
        tool_path(
            name = "objdump",
            path = "/usr/bin/objdump",
        ),
        tool_path(
            name = "strip",
            path = "/usr/bin/strip",
        ),
    ]

    dbg_feature = feature(name = "dbg")
    fastbuild_feature = feature(name = "fastbuild")
    opt_feature = feature(name = "opt")
    features = [
        dbg_feature,
        fastbuild_feature,
        opt_feature,
        feature(
            name = "default_linker_flags",
            enabled = True,
            flag_sets = [
                flag_set(
                    actions = all_cpp_link_actions,
                    flag_groups = ([
                        flag_group(
                            flags = common_linker_flags + [
                                "-stdlib=libc++",
                            ],
                        ),
                    ]),
                ),
            ],
        ),
        feature(
            name = "default_dbg_flags",
            enabled = True,
            flag_sets = [
                flag_set(
                    with_features = [
                        with_feature_set(
                            features = ["dbg"],
                        ),
                    ],
                    actions = all_cpp_actions,
                    flag_groups = ([
                        flag_group(
                            flags = common_dbg_flags,
                        ),
                    ]),
                ),
            ],
        ),
        feature(
            name = "default_opt_flags",
            enabled = True,
            flag_sets = [
                flag_set(
                    with_features = [
                        with_feature_set(
                            features = ["opt"],
                        ),
                    ],
                    actions = all_cpp_actions,
                    flag_groups = ([
                        flag_group(
                            flags = common_opt_flags,
                        ),
                    ]),
                ),
            ],
        ),
        feature(
            name = "default_compile_flags",
            enabled = True,
            flag_sets = [
                flag_set(
                    actions = all_cpp_compile_actions,
                    flag_groups = ([
                        flag_group(
                            flags = common_compile_flags + clang_compile_flags + [
                                "-stdlib=libc++",
                            ],
                        ),
                    ]),
                ),
            ],
        ),
    ]

    return cc_common.create_cc_toolchain_config_info(
        ctx = ctx,
        features = features,
        cxx_builtin_include_directories = [
            "/usr/lib/llvm-12/lib/clang/12.0.0/include",
            "/usr/lib/llvm-12/include",
            "/usr/include",
        ],
        toolchain_identifier = "local",
        host_system_name = "local",
        target_system_name = "local",
        target_cpu = "k8",
        target_libc = "unknown",
        compiler = "clang",
        abi_version = "unknown",
        abi_libc_version = "unknown",
        tool_paths = tool_paths,
    )

clang_toolchain_config = rule(
    implementation = _clang_impl,
    attrs = {},
    provides = [CcToolchainConfigInfo],
)

def _gcc_impl(ctx):
    tool_paths = [
        tool_path(
            name = "gcc",
            path = "/usr/bin/gcc",
        ),
        tool_path(
            name = "ld",
            path = "/usr/bin/ld",
        ),
        tool_path(
            name = "ar",
            path = "/usr/bin/ar",
        ),
        tool_path(
            name = "cpp",
            path = "/bin/false",
        ),
        tool_path(
            name = "gcov",
            path = "/bin/false",
        ),
        tool_path(
            name = "nm",
            path = "/bin/false",
        ),
        tool_path(
            name = "objdump",
            path = "/bin/false",
        ),
        tool_path(
            name = "strip",
            path = "/bin/false",
        ),
    ]

    dbg_feature = feature(name = "dbg")
    fastbuild_feature = feature(name = "fastbuild")

    features = [
        dbg_feature,
        fastbuild_feature,
        feature(
            name = "default_linker_flags",
            enabled = True,
            flag_sets = [
                flag_set(
                    actions = all_cpp_link_actions,
                    flag_groups = ([
                        flag_group(
                            flags = common_linker_flags,
                        ),
                    ]),
                ),
            ],
        ),
        feature(
            name = "default_dbg_flags",
            enabled = True,
            flag_sets = [
                flag_set(
                    with_features = [
                        with_feature_set(
                            features = ["dbg"],
                        ),
                    ],
                    actions = all_cpp_actions,
                    flag_groups = ([
                        flag_group(
                            flags = common_dbg_flags,
                        ),
                    ]),
                ),
            ],
        ),
        feature(
            name = "default_compile_flags",
            enabled = True,
            flag_sets = [
                flag_set(
                    actions = all_cpp_compile_actions,
                    # Only include the full set of flags if not compiling for clang-tidy.
                    # This is because there are some flags (e.g., -fcoroutines) that will
                    # cause complaints.
                    with_features = [
                        with_feature_set(
                            not_features = ["clang_tidy_flags"],
                        ),
                    ],
                    flag_groups = ([
                        flag_group(
                            flags = common_compile_flags + gcc_compile_flags,
                        ),
                    ]),
                ),
            ],
        ),
        feature(
            # This feature is used when creating the `.compile_commands.json` file. It
            # avoids introducing flags that aren't known to the Clang compiler because it
            # complains when it sees them, despite the fact that we are compiling with GCC.
            name = "clang_tidy_flags",
            enabled = False,
            flag_sets = [
                flag_set(
                    actions = all_cpp_compile_actions,
                    flag_groups = ([
                        flag_group(
                            flags = common_compile_flags,
                        ),
                    ]),
                ),
            ],
        ),
    ]

    return cc_common.create_cc_toolchain_config_info(
        ctx = ctx,
        features = features,
        cxx_builtin_include_directories = [
            "/usr/lib/gcc/x86_64-linux-gnu/10/include/",
            "/usr/include",
        ],
        toolchain_identifier = "local",
        host_system_name = "local",
        target_system_name = "local",
        target_cpu = "k8",
        target_libc = "unknown",
        compiler = "gcc",
        abi_version = "unknown",
        abi_libc_version = "unknown",
        tool_paths = tool_paths,
    )

gcc_toolchain_config = rule(
    implementation = _gcc_impl,
    attrs = {},
    provides = [CcToolchainConfigInfo],
)
