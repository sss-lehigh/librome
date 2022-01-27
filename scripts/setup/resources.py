from abc import ABC, abstractclassmethod
import configparser
import pathlib
import sys
import os

curr_path = pathlib.Path(__file__).absolute()
sys.path.append(str(curr_path.parent.parent))
import common  # nopep8


class __Resource(ABC):
    """ Defines the interface for a resource, which can be a tool, a library or basically anything that needs to be set up before things get started.
    """
    @abstractclassmethod
    def setup(self):
        pass


class Archive(__Resource):
    name = None
    archive = None
    url = None
    dest = None
    root_dir = None
    checksum = None
    mode = None

    def __init__(self, config, section_name):
        assert(configparser is not None)
        section_config = config[section_name]
        self.name = section_config["name"]
        self.archive_name = section_config["archive_name"]
        self.archive = section_config["archive"]
        self.url = section_config["url"]
        self.dest = section_config["dest"]
        self.root_dir = os.path.join(
            section_config["dest"], section_config["name"])
        self.checksum = section_config["checksum"]
        self.mode = section_config["mode"]

    def setup(self):
        if os.path.exists(self.root_dir):
            return
        common.download(self.url, self.dest)
        download_path = os.path.join(self.dest,
                                     self.archive)
        common.check(download_path, self.mode, self.checksum)
        common.unpack(download_path, self.dest, self.name)
        common.rm(download_path)