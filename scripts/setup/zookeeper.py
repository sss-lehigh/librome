from resources import Archive


class Zookeeeper(Archive):
    def __init__(self, config):
        super().__init__(config, "zookeeper")