import subprocess
import configparser
import os
import shutil
import hashlib

config = configparser.ConfigParser(
    interpolation=configparser.ExtendedInterpolation())
config.read("config.ini")


class ValidationError(Exception):
    pass


class DownloadError(Exception):
    pass


def download(url, dest):
    """ Tries to download the archive at `url` into `dest` folder with the given `name`.

    `url` remote archive to download

    `dest` local download destination
    """
    if subprocess.run(["wget", "-c", "--read-timeout=5",
                       "--tries=0", url, "-P", dest]).returncode != 0:
        raise DownloadError(f"Failed to download: {url}")


def validate_checksum(file, mode, expected):
    actual = ""
    if mode == "sha256":
        with open(file, "rb") as f:
            actual = hashlib.sha256(f.read()).hexdigest()
    elif mode == "sha512":
        with open(file, "rb") as f:
            actual = hashlib.sha512(f.read()).hexdigest()
    else:
        raise

    if actual != expected:
        raise ValidationError(f"Checksum failed: {actual} != {expected}")


def unpack(path, dest):
    shutil.unpack_archive(path, extract_dir=dest)


def rm(path):
    os.remove(path)



def check_dir():
    cwd = os.getcwd()
    expected = os.path.join(config["workspace"]["root"], "scripts")
    if cwd != expected:
        raise RuntimeError(
            'Calling script from unexpected directory: expected={0}, got={1}'.format(expected, cwd))


def checked_call(cmd):
    subprocess.run(cmd, shell=True, check=True)


def build_marker(path):
    return "  # <!-- " + path + " -->"


def __add_path(path):
    checked_call("echo \"export PATH=\$PATH:" + path + build_marker(path) +
                 "\" >>~/.bashrc")


def __check_path(path):
    return subprocess.run("cat ~/.bashrc | grep -q '" + build_marker(path) + "'", shell=True).returncode == 0


def try_add_path(path):
    if not __check_path(path):
        __add_path(path)
