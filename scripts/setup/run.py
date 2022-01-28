import mlnx_ofed
import zookeeper
import apt
import go
import conda
import pprof
import pathlib
import sys
import argparse

curr_path = pathlib.Path(__file__).absolute()
sys.path.append(str(curr_path.parent.parent))
from util import *  # nopep8


def main(args):
    check_dir()
    resources = []
    for r in args.resources:
        if r == 'apt':
            resources.append(apt.APTPackages(config))
        elif r == 'golang':
            resources.append(go.Golang(config))
        elif r == 'go':
            resources.append(go.GoPackages(config))
        elif r == 'conda':
            resources.append(conda.Conda(config))
        elif r == 'zookeeper':
            resources.append(zookeeper.Zookeeeper(config))
        elif r == 'pprof':
            resources.append(pprof.Pprof(config))
        elif r == 'mlnx_ofed':
            resources.append(mlnx_ofed.MlnxOfed(config))
        else:
            print(f"Unknown resource: {r}")

    for r in resources:
        try:
            r.setup()
        except Exception as e:
            print(e)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Do environment setup")
    parser.add_argument(
        '--resources', metavar="R", type=str, nargs='+',
        help="Names of resources corresponding to headers in `config.ini` to install")
    main(parser.parse_args())
