from resources import Archive
import os


class Pprof(Archive):
    def __init__(self, config):
        super().__init__(config, "pprof")

    def setup(self):
        super().setup()
        path = os.path.join(self.dest, self.name,
                            self.name + "-" + self.archive_name)
        if os.system(f"cd {path} && go install .") != 0:
            raise
