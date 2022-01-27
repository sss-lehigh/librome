import configparser
import os
import shutil
import hashlib

config = configparser.ConfigParser(
    interpolation=configparser.ExtendedInterpolation())
config.read("config.ini")


def download(url, dest):
    """ Tries to download the archive at `url` into `dest` folder with the given `name`.

    `url` remote archive to download

    `dest` local download destination
    """
    os.system(
        f"""wget -c --read-timeout=5 --tries=0 '{url}' -P '{dest}'""")


def check(file, mode, expected):
    actual = ""
    print(file)
    if mode == "sha256":
        with open(file, "rb") as f:
            actual = hashlib.sha256(f.read()).hexdigest()
    elif mode == "sha512":
        with open(file, "rb") as f:
            actual = hashlib.sha512(f.read()).hexdigest()
    else:
        raise

    if actual != expected:
        raise Exception(f"{actual} != {expected}")


def unpack(path, dir, name):
    shutil.unpack_archive(path, extract_dir=os.path.join(dir, name))


def rm(path):
    os.remove(path)


def ok(resource):
    """ Checks whether a resource is well formed (i.e., its members are not None).

    `resource` is the object to check
    """
    for field in resource.required():
        try:
            v = getattr(resource, field)
            if v is None:
                print(f"self.{field} cannot be None")
                return False
        except:
            print(f"self.{field} does not exist")
            return False
    return True


def check_dir():
    cwd = os.getcwd()
    if cwd != os.path.join(config["workspace"]["root"], "scripts"):
        raise RuntimeError(
            'Calling script from unexpected directory: {0}'.format(cwd))
