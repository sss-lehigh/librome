""""""

load("@com_github_nelhage_rules_boost//:boost/boost.bzl", "boost_deps")
load("@hedron_compile_commands//:workspace_setup.bzl", "hedron_compile_commands_setup")
load("@rules_cc//cc:repositories.bzl", "rules_cc_dependencies")
load("@rules_proto//proto:repositories.bzl", "rules_proto_dependencies", "rules_proto_toolchains")
load("@rules_fuzzing//fuzzing:repositories.bzl", "rules_fuzzing_dependencies")
load("@rules_fuzzing//fuzzing:init.bzl", "rules_fuzzing_init")

def rome_setup():
    """ Handles setup of sub-components.
    """
    boost_deps()
    hedron_compile_commands_setup()
    rules_cc_dependencies()
    rules_proto_dependencies()
    rules_proto_toolchains()
    rules_fuzzing_dependencies()
    rules_fuzzing_init()
