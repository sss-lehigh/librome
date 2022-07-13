load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("//:rome/private/util.bzl", "maybe")

# TODO: Use http_archive instead of git_repository (https://docs.bazel.build/versions/4.2.2/external.html#repository-rules)

def rome_deps():
    """ Handles all dependencies.
    """
    maybe(
        git_repository,
        name = "gtest",
        commit = "e2239ee6043f73722e7aa812a459f54a28552929",
        remote = "https://github.com/google/googletest",
        shallow_since = "1623433346 -0700",
    )

    maybe(
        http_archive,
        name = "bazel_skylib",
        urls = ["https://github.com/bazelbuild/bazel-skylib/archive/refs/tags/1.1.1.zip"],
        sha256 = "fc64d71583f383157e3e5317d24e789f942bc83c76fde7e5981cadc097a3c3cc",
        strip_prefix = "bazel-skylib-1.1.1",
    )

    maybe(
        git_repository,
        name = "absl",
        branch = "lts_2022_06_23",
        # commit = "273292d1cfc0a94a65082ee350509af1d113344d",
        remote = "https://github.com/abseil/abseil-cpp",
    )

    maybe(
        git_repository,
        name = "hedron_compile_commands",
        commit = "e085566bf35e020402a2e32258360b16446fbad8",
        remote = "https://github.com/hedronvision/bazel-compile-commands-extractor",
        shallow_since = "1638167585 -0800",
    )

    maybe(
        http_archive,
        name = "io_bazel_rules_go",
        sha256 = "2b1641428dff9018f9e85c0384f03ec6c10660d935b750e3fa1492a281a53b0f",
        urls = [
            "https://mirror.bazel.build/github.com/bazelbuild/rules_go/releases/download/v0.29.0/rules_go-v0.29.0.zip",
            "https://github.com/bazelbuild/rules_go/releases/download/v0.29.0/rules_go-v0.29.0.zip",
        ],
    )

    maybe(
        http_archive,
        name = "bazel_gazelle",
        sha256 = "de69a09dc70417580aabf20a28619bb3ef60d038470c7cf8442fafcf627c21cb",
        urls = [
            "https://mirror.bazel.build/github.com/bazelbuild/bazel-gazelle/releases/download/v0.24.0/bazel-gazelle-v0.24.0.tar.gz",
            "https://github.com/bazelbuild/bazel-gazelle/releases/download/v0.24.0/bazel-gazelle-v0.24.0.tar.gz",
        ],
    )

    # Download boost
    maybe(
        git_repository,
        name = "com_github_nelhage_rules_boost",
        commit = "9f9fb8b2f0213989247c9d5c0e814a8451d18d7f",
        remote = "https://github.com/nelhage/rules_boost",
        shallow_since = "1570056263 -0700",
    )

    maybe(
        http_archive,
        name = "fmt",
        build_file = "@//:third_party/BUILD.fmt",
        sha256 = "d8e9f093b2241c3a9fc3895e23231ef9de00c762cfa0a9c65e4748755bc352ae",
        strip_prefix = "fmt-8.1.0",
        urls = ["https://github.com/fmtlib/fmt/releases/download/8.1.0/fmt-8.1.0.zip"],
    )

    maybe(
        http_archive,
        name = "spdlog",
        build_file = "@//:third_party/BUILD.spdlog",
        sha256 = "130bd593c33e2e2abba095b551db6a05f5e4a5a19c03ab31256c38fa218aa0a6",
        strip_prefix = "spdlog-1.9.2",
        urls = ["https://github.com/gabime/spdlog/archive/refs/tags/v1.9.2.zip"],
    )
