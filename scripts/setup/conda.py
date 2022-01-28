import subprocess
import sys
from resources import HostedResource
from util import *
import os


class Conda(HostedResource):
    conda = None
    env_file = None
    env_name = None

    def __init__(self, config):
        super().__init__(config, "conda")
        self.conda = os.path.join(self.dest_path, "bin", "conda")
        self.env_name = config["conda"]["env_name"]
        self.env_file = config["conda"]["env_file"]

    def create_env(self):
        subprocess.run([self.conda, "env", "create", "-n", self.env_name, "-f", self.env_file, "-q"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

    def init(self):
        subprocess.run([self.conda, "init", "--all"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

    def setup(self):
        if subprocess.run(["conda --version"], shell=True, capture_output=False).returncode != 0:
            super().setup()
            # Install and intialize
            subprocess.run(f"sudo chmod +x {self.download_path}", shell=True)
            subprocess.run([self.download_path, "-b", "-p",
                            self.dest_path])

            # Cleanup
            subprocess.run(["rm", self.download_path])
            subprocess.run(
                f"find {self.dest_path} -follow -type f -name '*.a' -delete && find {self.dest_path} -follow -type f -name '*.js.map' -delete && {self.dest_path}/bin/conda clean -afy", shell=True)
        else:
            print("Conda already installed, skipping.")
        self.init()
        self.create_env()
        try_add_bashrc("conda activate " + self.env_name)
