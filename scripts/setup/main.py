import mlnx_ofed 
import zookeeper
import pprof
import pathlib
import sys

curr_path = pathlib.Path(__file__).absolute()
sys.path.append(str(curr_path.parent.parent))
import common  # nopep8

if __name__ == "__main__":
    common.check_dir()
    resources = []
    resources.append(zookeeper.Zookeeeper(common.config))
    resources.append(pprof.Pprof(common.config))
    resources.append(mlnx_ofed.MlnxOfed(common.config))
    for r in resources:
        r.setup()