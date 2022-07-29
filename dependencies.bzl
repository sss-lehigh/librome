# TODO: Use http_archive instead of git_repository (https://docs.bazel.build/versions/4.2.2/external.html#repository-rules)
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def maybe(repo_rule, name, **kwargs):
    if name not in native.existing_rules():
        repo_rule(name = name, **kwargs)

def rome_dependencies():
    """ Handles all dependencies.
    """
    maybe(
        http_archive,
        name = "bazel_skylib",
        urls = ["https://github.com/bazelbuild/bazel-skylib/archive/refs/tags/1.1.1.zip"],
        sha256 = "fc64d71583f383157e3e5317d24e789f942bc83c76fde7e5981cadc097a3c3cc",
        strip_prefix = "bazel-skylib-1.1.1",
    )

    maybe(
        http_archive,
        name = "rules_cc",
        sha256 = "4dccbfd22c0def164c8f47458bd50e0c7148f3d92002cdb459c2a96a68498241",
        urls = ["https://github.com/bazelbuild/rules_cc/releases/download/0.0.1/rules_cc-0.0.1.tar.gz"],
    )

    maybe(
        http_archive,
        name = "rules_proto",
        sha256 = "66bfdf8782796239d3875d37e7de19b1d94301e8972b3cbd2446b332429b4df1",
        strip_prefix = "rules_proto-4.0.0",
        urls = [
            "https://github.com/bazelbuild/rules_proto/archive/refs/tags/4.0.0.tar.gz",
        ],
    )

    maybe(
        http_archive,
        name = "rules_python",
        sha256 = "b6d46438523a3ec0f3cead544190ee13223a52f6a6765a29eae7b7cc24cc83a0",
        urls = ["https://github.com/bazelbuild/rules_python/releases/download/0.1.0/rules_python-0.1.0.tar.gz"],
    )

    # Fuzzing
    maybe(
        http_archive,
        name = "rules_fuzzing",
        sha256 = "a5734cb42b1b69395c57e0bbd32ade394d5c3d6afbfe782b24816a96da24660d",
        strip_prefix = "rules_fuzzing-0.1.1",
        urls = ["https://github.com/bazelbuild/rules_fuzzing/archive/v0.1.1.zip"],
    )

    # Abseil
    maybe(
        http_archive,
        name = "absl",
        sha256 = "db546f89b33ebef0a397511a09c36267776fe11985a4ee35bac32c0ab516fca1",
        strip_prefix = "abseil-cpp-a4cc270df18b47685e568e01bb5c825493f58d25",
        urls = ["https://github.com/abseil/abseil-cpp/archive/a4cc270df18b47685e568e01bb5c825493f58d25.zip"],
    )

    # GoogleTest/GoogleMock framework. Used by most unit-tests.
    maybe(
        http_archive,
        name = "gtest",  # 2021-05-19T20:10:13Z
        sha256 = "8cf4eaab3a13b27a95b7e74c58fb4c0788ad94d1f7ec65b20665c4caf1d245e8",
        strip_prefix = "googletest-aa9b44a18678dfdf57089a5ac22c1edb69f35da5",
        urls = ["https://github.com/google/googletest/archive/aa9b44a18678dfdf57089a5ac22c1edb69f35da5.zip"],
    )

    # Google benchmark.
    maybe(
        http_archive,
        name = "benchmark",
        sha256 = "62e2f2e6d8a744d67e4bbc212fcfd06647080de4253c97ad5c6749e09faf2cb0",
        strip_prefix = "benchmark-0baacde3618ca617da95375e0af13ce1baadea47",
        urls = ["https://github.com/google/benchmark/archive/0baacde3618ca617da95375e0af13ce1baadea47.zip"],
    )

    maybe(
        http_archive,
        name = "hedron_compile_commands",
        sha256 = "0dfe793b5779855cf73b3ee9f430e00225f51f38c70555936d4dd6f1b3c65e66",
        strip_prefix = "bazel-compile-commands-extractor-d6734f1d7848800edc92de48fb9d9b82f2677958",
        urls = ["https://github.com/hedronvision/bazel-compile-commands-extractor/archive/d6734f1d7848800edc92de48fb9d9b82f2677958.zip"],
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
        http_archive,
        name = "com_github_nelhage_rules_boost",
        sha256 = "8e415bb553833e67c9c0b8ce5577ba6fcbe458eb8274cf14720f9914cd1f39d7",
        strip_prefix = "rules_boost-06d26e60e9614bddd42a8ed199179775f7d7f90d",
        urls = ["https://github.com/nelhage/rules_boost/archive/06d26e60e9614bddd42a8ed199179775f7d7f90d.zip"],
    )

    maybe(
        http_archive,
        name = "fmt",
        build_file = "@rome//:BUILD.fmt",
        sha256 = "d8e9f093b2241c3a9fc3895e23231ef9de00c762cfa0a9c65e4748755bc352ae",
        strip_prefix = "fmt-8.1.0",
        urls = ["https://github.com/fmtlib/fmt/releases/download/8.1.0/fmt-8.1.0.zip"],
    )

    maybe(
        http_archive,
        name = "spdlog",
        build_file = "@rome//:BUILD.spdlog",
        sha256 = "130bd593c33e2e2abba095b551db6a05f5e4a5a19c03ab31256c38fa218aa0a6",
        strip_prefix = "spdlog-1.9.2",
        urls = ["https://github.com/gabime/spdlog/archive/refs/tags/v1.9.2.zip"],
    )
