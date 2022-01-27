#!/bin/bash

if [[ ${PWD##*/} != "scripts" ]]; then
  echo "$0: Please rerun from the setup subdirectory"
  exit 1
fi

# Execute the `root.sh` script as root. This script will handle
# installing various utilities that we need in our environment that
# need to be done as root.
sudo ./setup/root.sh

# Add Python packages to the conda environment
export PATH=$PATH:/opt/conda/bin

sudo usermod -a -G conda $(id -u -n)

if [[ $(cat $HOME/.bashrc | grep "conda initialize") == "" ]]; then
  conda init bash
fi

# Instal Bazel buildtools for convenience.
export PATH=$PATH:/usr/local/go/bin
if [[ $(command -v bazel) == "" ]]; then
  go install github.com/bazelbuild/bazelisk@latest
fi
if [[ $(command -v buildifier) == "" ]]; then
  go install github.com/bazelbuild/buildtools/buildifier@latest
fi
if [[ $(command -v buildozer) == "" ]]; then
  go install github.com/bazelbuild/buildtools/buildozer@latest
fi

if [[ $(cat $HOME/.bashrc | grep 'bootstrap.sh') == "" ]]; then
  echo "" >>$HOME/.bashrc
  echo "# Added by bootstrap.sh" >>$HOME/.bashrc
  echo "export PATH=\$PATH:\$(go env GOPATH)/bin" >>$HOME/.bashrc
  echo "alias bazel='bazelisk'" >>$HOME/.bashrc
fi
