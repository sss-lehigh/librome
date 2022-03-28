import subprocess
import configparser
import os
import shutil
import hashlib

config = configparser.ConfigParser(
    interpolation=configparser.ExtendedInterpolation())


def init(configfile):
    config.read(configfile)


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


def __check_env(line):
    return subprocess.run(
        "cat " + config['workspace']['env_file'] + " | grep -q '" + line + "'", shell=True).returncode == 0


def __add_env(line):
    checked_call("echo '" + line + "'  | sudo tee -a " + config['workspace']['env_file'])


# When adding a new path, we first check if the path exists then add it to both the `.bashrc` file and to the `PATH` environment variable.
def try_add_path(path):
    if not __check_env(path):
        __add_env('export PATH=$PATH:' + path)
        checked_call('export PATH=$PATH:' + path)


def try_add_bashrc(line):
    if not __check_env(line):
        checked_call("echo '" + line + "' | sudo tee -a " + config['workspace']['env_file'])
