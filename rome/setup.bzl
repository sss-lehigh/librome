""""""

load(":rome/private/boost.bzl", "boost_setup")
load(":rome/private/compile_commands.bzl", "compile_commands_setup")

def rome_setup():
    """ Handles setup of sub-components.
    """
    boost_setup()
    compile_commands_setup()
