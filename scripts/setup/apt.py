from resources import __Resource__
from util import checked_call
import json

class APTPackages(__Resource__):
    packages = None
    repos = None

    def __init__(self, config):
        assert(config is not None)
        section_config = config["apt"]
        self.packages = json.loads(section_config["packages"])
        self.repos = json.loads(section_config["repos"])
    
    def prepared(self):
        return True  # Never skip the setup

    def setup(self):
        repos_string = ' '.join(r for r in self.repos)
        packages_string = ' '.join(p for p in self.packages)
        checked_call("sudo add-apt-repository -y " + repos_string + " && sudo apt-get update && sudo apt-get -y upgrade")
        checked_call(
            "sudo apt-get update && sudo apt-get install -y " + packages_string)
