from resources import Archive
import os


class MlnxOfed(Archive):
    ofed = None

    def __init__(self, config):
        self.ofed = config["mlnx_ofed"]["ofed"]
        super().__init__(config, self.ofed)

    def setup_mlx4(self):
        pass

    def setup_mlx5(self):
        path = os.path.join(self.dest, self.name, self.archive_name)
        os.system(
            f"cd {path} && sudo ./mlnxofedinstall --without-fw-update --force --without-neohost-backend")

    def setup(self):
        super().setup()
        if self.ofed == "mlx4_ofed":
            self.setup_mlx4()
        elif self.ofed == "mlx5_ofed":
            self.setup_mlx5()
        else:
            raise
