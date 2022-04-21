#!/usr/bin/env python

from absl import app
from absl import flags
import configparser
import subprocess
import os
import threading
import time
from datetime import datetime

# Global variables for configuration and command line flags
config = configparser.ConfigParser(
    interpolation=configparser.ExtendedInterpolation())
common = {}
FLAGS = flags.FLAGS

# Configuration information.
flags.DEFINE_string(
    "configfile",
    "rexec.ini",
    "Configuration file to use in conjunction with the command line parameters.",
    short_name="c")
flags.DEFINE_string(
    "nodefile",
    None,
    "File containing a comma separated list of nodes to be included in the remote execution.",
    required=True,
    short_name='n')
flags.DEFINE_integer(
    'num_nodes',
    None,
    "Number of nodes to run on. Takes the first N nodes in the nodefile. Ignored when syncing with remote nodes.",
    short_name='N')

flags.DEFINE_list(
    "skip", [],
    "Hosts to skip while runnig an experiment. Acts as if the command was executed on the host for debugging."
)
flags.DEFINE_list(
    "offline", [],
    "Hosts that should be considered offline (i.e., are not included in the host set)."
)

flags.DEFINE_bool(
    "sync",
    False,
    "Sync the local copy of the repo with the remote nodes. If a command or experiment is also provided, the sync is performed first.",
    short_name="s")
flags.DEFINE_string("cmd", None, "Command to execute on the remote machines.")


flags.DEFINE_string(
    "outfile", None,
    "File to write output of experiments to on the remote machine")
flags.DEFINE_string("get_data",
                    None,
                    "Get data from the provided remote directory.",
                    short_name="D")
flags.DEFINE_bool("sudo", False, "Run experiment as sudo.", short_name="S")
flags.DEFINE_bool("debug", False, "Print out debug information.")

# Running experiments.
flags.DEFINE_string(
    "exp", None,
    "Name of experiment to run (must have a corresponding section in the configfile)."
)
flags.DEFINE_string(
    "exp_bin", None,
    "Binary file to run for a given experiment. Can be used to provide the same flow for multiple binary files to test different complie-time configurations."
)
flags.DEFINE_list(
    "exp_args", [],
    "Arguements to be passed during execution of the experiment named in the --exp flag."
)


def get_config_str():
  config_str = "Configuration:\n"
  for s in config.sections():
    config_str += "[" + s + "]\n"
    for k in config[s]:
      config_str += str(k) + "=" + config[s][k] + "\n"
    config_str += "\n"
  return config_str


def get_hosts(nodefile):
  nodes = get_nodes(nodefile)
  hosts = {}
  for n in nodes:
    if n[0] not in FLAGS.offline:
      hosts[n[0]] = build_hostname(n[2], n[1])
  return hosts


def get_nodes(nodefile):
  nodes = []
  with open(nodefile, "r") as infile:
    line = infile.readline()
    while line != "":
      nodes.append(tuple(line.strip().split(',')))
      line = infile.readline()
  return nodes


__utah__ = [
  "xl170",
  "d6515",
  "c6525-100g",
  "m510",
]

__emulab__ = [
  "r320",
]

def build_hostname(nodetype, id):
  if nodetype in __utah__:
    return id + ".utah.cloudlab.us"
  elif nodetype in __emulab__:
    return id + ".apt.emulab.net"
  else:
    print("Undefined host type in nodefile.")
    exit(1)


def DEBUG(msg):
  if FLAGS.debug:
    print(msg)


def prepare_log(name, host, testbed, cmd):
  log_dir = os.path.join(common["log_dir"], cmd, testbed)
  os.makedirs(log_dir, exist_ok=True)
  return os.path.join(log_dir, name + '.' + host + '.log')


# Executes a command locally but logs it according to the host and provide logging string
def lexec(cmd, log_path):
  with open(log_path, "w") as outfile:
    outfile.write(get_config_str())
    outfile.write(str(cmd) + "\n\n")
    outfile.flush()
    try:
      DEBUG(cmd)
      subprocess.run(cmd,
                     shell=True,
                     check=True,
                     stdout=outfile,
                     stderr=outfile)
    except subprocess.CalledProcessError:
      print("Check logs: {}".format(log_path))


def rexec(host, cmd, log_path):
  DEBUG(cmd)
  with open(log_path, "w") as outfile:
    outfile.write(get_config_str())
    outfile.write(str(cmd) + "\n\n")
    outfile.flush()
    cmd = common["ssh"] + " " + common[
        "remote_user"] + "@" + host + " 'bash -l -c \"" + cmd + "\"'"
    DEBUG(cmd)
    try:
      subprocess.run(cmd,
                     shell=True,
                     check=True,
                     stdout=outfile,
                     stderr=outfile)
    except subprocess.CalledProcessError:
      print("Check logs: {}".format(log_path))


def get_data(args):
  # print_config("get_data")
  pass


def main(argv):
  # Read configuration file
  global common
  global config
  config.read(FLAGS.configfile)
  common = config["common"]
  testbed = os.path.split(FLAGS.nodefile)[-1].split('.')[0]
  hosts = get_hosts(FLAGS.nodefile)
  skip = FLAGS.skip
  if FLAGS.num_nodes is None:
    FLAGS.num_nodes = len(hosts)

  DEBUG("configfile={}".format(FLAGS.configfile))
  DEBUG("\n{}".format(get_config_str()))
  DEBUG("nodefile={}".format(FLAGS.nodefile))
  DEBUG("testbed={}".format(testbed))
  DEBUG("hosts={}".format(hosts))

  # Perform a sync before any other operations
  threads = {}
  if FLAGS.sync:
    DEBUG("Syncing nodes")
    # Prepare the sync command
    src = os.path.normpath(config["sync"]["src"]) + "/"
    for h in hosts:
      dest = common["remote_user"] + "@" + hosts[h] + ":" + config["sync"][
          "dest"]
      cmd = "rsync -r -e '" + common["ssh"] + "' --exclude-from=" + config["sync"][
          "exclude_file"] + " --files-from=" + config["sync"]["include_file"] + " --progress -uva " + src + " " + dest

      t = threading.Thread(
          target=lexec, args=[cmd,
                              prepare_log(h, hosts[h], testbed, "sync")])
      threads[h] = t
      t.start()
    for h in threads.keys():
      DEBUG("Joining host: {}".format(h))
      threads[h].join()

  threads = {}
  if FLAGS.get_data is not None:
    DEBUG("Getting data from hosts")
    if FLAGS.exp is None:
      print(
          "Please provide the experiment name of the retrieved data (using the --exp flag)."
      )
      exit()
    for h in hosts:
      if (len(threads) >= FLAGS.num_nodes):
        break
      src = common["remote_user"] + "@" + hosts[h] + ":" + os.path.normpath(
          FLAGS.get_data) + "/"
      now_str = datetime.now().strftime("%Y%m%d%H%M%S")
      dest = os.path.join(config["get_data"]["dest"], now_str, FLAGS.exp,
                          testbed, h + "." + hosts[h] + ".data")
      DEBUG("Downloading to {}".format(dest))
      os.makedirs(dest, exist_ok=True)
      cmd = "rsync -e '" + common[
          "ssh"] + "'  --progress -uva " + src + " " + dest + "/"
      t = threading.Thread(
          target=lexec, args=[cmd,
                              prepare_log(h, hosts[h], testbed, "data")])
      threads[h] = t
      t.start()
    for h in threads.keys():
      DEBUG("Joining host: {}".format(h))
      threads[h].join()

  threads = {}
  active_hosts = [h for h in hosts.keys()]
  active_hosts = active_hosts[0:FLAGS.num_nodes]
  if FLAGS.cmd is not None:
    DEBUG("Running command: {}".format(FLAGS.cmd))
    for h in active_hosts:
      dest = common["remote_user"] + "@" + hosts[h]
      t = threading.Thread(
          target=rexec,
          args=[hosts[h], FLAGS.cmd,
                prepare_log(h, hosts[h], testbed, "cmd")])
      threads[h] = t
      t.start()
    for h in threads.keys():
      DEBUG("Joining host: {}".format(h))
      threads[h].join()

  threads = {}
  if FLAGS.exp is not None and not FLAGS.get_data:
    DEBUG("Running experiment: {}".format(FLAGS.exp))
    # Check that the experiment appears in the provided experimental config file.
    exp_sect = FLAGS.exp
    args_sect = FLAGS.exp + "_args"
    if not config.has_section(exp_sect) or not config.has_option(
        exp_sect, FLAGS.exp + "_bin"):
      print("Invalid experiment provided. Check the configfile.")
      exit(1)
    if not config.has_section(args_sect):
      print("Warning: No default configuration provided for experiment: {}".
            format(exp_sect))

    for arg in FLAGS.exp_args:
      key = arg.split('=')[0]
      val = arg.split('=')[1]
      config[args_sect][key] = val

    host_id = 0
    for h in active_hosts:
      # Maybe run a command if this host is the leader
      leader = False
      if config.has_option(exp_sect, "leader_cmd") and config.has_option(
          exp_sect, "leader"):
        leader = h == config[exp_sect]["leader"]
      if leader:
        rexec(hosts[h], config[exp_sect]["leader_cmd"],
              prepare_log(h, hosts[h], testbed, "leader"))

      # Build arguments from config and command line
      args = []
      for k in config[args_sect].keys():
        if k == "use_erpc":
          args.append("--" + k + "=" + str(config.getboolean(args_sect, k)))
        else:
          args.append("--" + k + "=" + config[args_sect][k])

      # Set host
      args.append("--hostname=" + h)
      args.append("--host_id=" + str(host_id))
      host_id += 1

      # Set remotes
      remotes = []
      for j in active_hosts:
        if h != j:
          remotes.append(j)
      args.append("--remotes=" + ','.join(remotes))

      # Flatten arguments to pass in remote execution
      arg_string = ""
      for arg in args:
        arg_string += arg + " "
      if leader:
        leader_args_sect = FLAGS.exp + "_leader_args"
        for arg in config[leader_args_sect].keys():
          arg_string += "--" + arg + "=" + config[leader_args_sect][arg] + " "
      bin_dir = config[exp_sect][FLAGS.exp + "_bin_dir"]
      bin_name = config[exp_sect][
          FLAGS.exp + "_bin"] if FLAGS.exp_bin is None else FLAGS.exp_bin
      cmd = "sudo " if FLAGS.sudo else ""
      cmd += os.path.join(bin_dir, bin_name) + " " + arg_string
      if FLAGS.outfile is not None:
        # Redirect output
        cmd += " &>" + FLAGS.outfile

      # Start thread (cannot be sequential)
      if h not in skip:
        t = threading.Thread(
            target=rexec,
            args=[hosts[h], cmd,
                  prepare_log(h, hosts[h], testbed, exp_sect)])
        threads[h] = t
        t.start()
      else:
        DEBUG("Ignoring {}".format(h))

      # Wait a hot sec for the first node to reset the barrier.
      if leader:
        time.sleep(2)
    for h in threads.keys():
      DEBUG("Joining host: {}".format(h))
      threads[h].join()


if __name__ == "__main__":
  app.run(main)
