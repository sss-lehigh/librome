#!/bin/bash

if [[ $EUID != 0 ]]; then
  echo "$0: Rerun as root"
  exit 1
fi

apt-get -y upgrade && apt-get update
DEBIAN_FRONTEND=noninteractive apt-get install -y sudo git wget openssh-server make libnuma-dev libpapi-dev apt-utils software-properties-common vim mlocate zip unzip texlive net-tools

# Install GCC 11
if [[ ! -e "/usr/bin/g++-11" ]]; then
  add-apt-repository -y ppa:ubuntu-toolchain-r/test && apt-get update
  apt-get install -y g++-11 gcc-11
fi

# Install Clang 10
if [[ ! -e "/usr/bin/clang-10" ]]; then
  apt-get install -y clang-10 libc++-10-dev libc++abi-10-dev
fi

# Set up update-alternatives for our compilers
if [[ ! -e "/usr/bin/c++" ]]; then
  update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-11 50 --slave /usr/bin/gcc gcc /usr/bin/gcc-11
  update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-10 50 --slave /usr/bin/clang clang /usr/bin/clang-10
  update-alternatives --install /usr/bin/c++ c++ /usr/bin/g++ 50
  update-alternatives --install /usr/bin/c++ c++ /usr/bin/clang++ 51
fi

if [[ ! -e "/usr/local/go" ]]; then
  GOLANG_SHA=231654bbf2dab3d86c1619ce799e77b03d96f9b50770297c8f4dff8836fc8ca2
  GOLANG_ARCHIVE=go1.17.6.linux-amd64.tar.gz
  GOLANG_URL=https://go.dev/dl/${GOLANG_ARCHIVE}
  cd /usr/local && wget ${GOLANG_URL}
  cd /usr/local &&
    echo "${GOLANG_SHA}  ${GOLANG_ARCHIVE}" | sha256sum -c --status &&
    tar -xf ${GOLANG_ARCHIVE} &&
    rm ${GOLANG_ARCHIVE}
  echo "PATH=\$PATH:/usr/local/go/bin" >>/etc/profile
fi

# Download and install Miniconda under `/opt/conda` with the group `conda`. Then, do some
# cleanup and add the conda binaries to PATH. Note that it is possible that the checksum
# is not up to date since we install the latest version of Miniconda. If you are having
# trouble with the checksum failing, go to https://docs.conda.io/en/latest/miniconda.html
# and ensure that the below value is correct.
if [[ ! -e "/opt/conda" ]]; then
  CONDA_SHA=1ea2f885b4dbc3098662845560bc64271eb17085387a70c2ba3f29fff6f8d52f
  CONDA_INSTALLER=Miniconda3-latest-Linux-x86_64.sh
  groupadd conda
  mkdir -p /opt
  cd /tmp && wget https://repo.anaconda.com/miniconda/${CONDA_INSTALLER} &&
    echo "${CONDA_SHA} ${CONDA_INSTALLER}" | sha256sum --check &&
    chmod +x ${CONDA_INSTALLER} &&
    ./${CONDA_INSTALLER} -b -p /opt/conda
  rm /tmp/${CONDA_INSTALLER}
  /opt/conda/bin/conda init bash
  chgrp -R conda /opt/conda && chmod -R g+w /opt/conda
  find /opt/conda/ -follow -type f -name '*.a' -delete &&
    find /opt/conda/ -follow -type f -name '*.js.map' -delete &&
    /opt/conda/bin/conda clean -afy
fi
