FROM ubuntu:focal
ENV TERM xterm-color

# Install necessary utilities.
ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get upgrade && apt-get update
RUN apt-get install -y git wget openssh-server make sudo libnuma-dev libpapi-dev apt-utils software-properties-common vim mlocate zip unzip

# Install GCC 11 
RUN add-apt-repository -y ppa:ubuntu-toolchain-r/test && apt-get update
RUN apt-get install -y g++-11

# Install Clang 10
RUN apt-get install -y clang-10 libc++-10-dev libc++abi-10-dev

# Set up update-alternatives for our compilers
RUN update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-11 50 --slave /usr/bin/gcc gcc /usr/bin/gcc-11
RUN update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-10 50 --slave /usr/bin/clang clang /usr/bin/clang-10
RUN update-alternatives --install /usr/bin/c++ c++ /usr/bin/g++ 50 --slave /usr/bin/cc cc /usr/bin/gcc
RUN update-alternatives --install /usr/bin/c++ c++ /usr/bin/clang++ 51 --slave /usr/bin/cc cc /usr/bin/clang

# Download and install Golang
ARG GOLANG_SHA=231654bbf2dab3d86c1619ce799e77b03d96f9b50770297c8f4dff8836fc8ca2
ARG GOLANG_ARCHIVE=go1.17.6.linux-amd64.tar.gz
ARG GOLANG_URL=https://go.dev/dl/${GOLANG_ARCHIVE}
RUN cd /usr/local && wget ${GOLANG_URL}
RUN cd /usr/local \
  && echo "${GOLANG_SHA}  ${GOLANG_ARCHIVE}" | sha256sum -c --status \
  && tar -xf ${GOLANG_ARCHIVE} \
  && rm ${GOLANG_ARCHIVE}
RUN echo "PATH=\$PATH:/usr/local/go/bin" >>/etc/profile
ENV PATH $PATH:/usr/local/go/bin

# Download and install Miniconda under `/opt/conda` with the group `conda`. Then, do some
# cleanup and add the conda binaries to PATH. Note that it is possible that the checksum
# is not up to date since we install the latest version of Miniconda. If you are having
# trouble with the checksum failing, go to https://docs.conda.io/en/latest/miniconda.html
# and ensure that the below value is correct.
ARG CONDA_SHA=1ea2f885b4dbc3098662845560bc64271eb17085387a70c2ba3f29fff6f8d52f
ARG CONDA_INSTALLER=Miniconda3-latest-Linux-x86_64.sh
RUN groupadd conda
RUN mkdir -p /opt
RUN cd /tmp && wget https://repo.anaconda.com/miniconda/${CONDA_INSTALLER} && \
  echo "${CONDA_SHA} ${CONDA_INSTALLER}" | sha256sum --check && \
  chmod +x ${CONDA_INSTALLER} && \
  ./${CONDA_INSTALLER} -b -p /opt/conda
RUN rm /tmp/${CONDA_INSTALLER}
RUN /opt/conda/bin/conda init bash
RUN chgrp -R conda /opt/conda && chmod -R g+w /opt/conda
RUN find /opt/conda/ -follow -type f -name '*.a' -delete && \
  find /opt/conda/ -follow -type f -name '*.js.map' -delete && \
  /opt/conda/bin/conda clean -afy
ENV PATH /opt/conda/bin:$PATH

# Configure a user.
ARG USER=develop
RUN useradd -ms /bin/bash -G conda,sudo -k /etc/skel ${USER}
RUN echo "${USER}:${USER}" | chpasswd

# ====== EXECUTED AS USER =======
USER ${USER}

# Add Python packages to the conda environment
RUN conda init bash
COPY environment.yml /home/${USER}/environment.yml
RUN conda env create -f ~/environment.yml
RUN rm ~/environment.yml
RUN echo "conda activate ${USER}" >>~/.bashrc

# Instal Bazel buildtools for convenience.
RUN /usr/local/go/bin/go install github.com/bazelbuild/bazelisk@latest
RUN /usr/local/go/bin/go install github.com/bazelbuild/buildtools/buildifier@latest
RUN /usr/local/go/bin/go install github.com/bazelbuild/buildtools/buildozer@latest
RUN echo "export PATH=\$PATH:\$(go env GOPATH)/bin" >>~/.bashrc
RUN echo "alias bazel='bazelisk'" >>~/.bashrc
ENV PATH $PATH:/home/${USER}/go/bin 
# ===============================

USER root

# Download the newest bash completions script for Bazel via GitHub. We do not use this
# actually install Bazel because `bazelisk` will take care of installing the newest 
# version, which just makes our lives easier. 
#
# NOTE: This can take some time to build. Hopefully someday `bazelisk` will ship with
# the autocomplete scripts...
ARG BAZELBASH_URL=https://github.com/bazelbuild/bazel
ARG BAZELBASH_DIR=bazel
RUN cd /tmp && git clone ${BAZELBASH_URL}
RUN cd /tmp/${BAZELBASH_DIR} \
  && bazelisk build //scripts:bazel-complete.bash 
RUN cd /tmp/${BAZELBASH_DIR} \
  && cp bazel-bin/scripts/bazel-complete.bash /etc/bash_completion.d/bazelisk \
  && cp bazel-bin/scripts/bazel-complete.bash /etc/bash_completion.d/bazel \
  && cd .. && rm -r ${BAZELBASH_DIR}

# This image exposes a port that hosts an ssh server for users to connect to.
# The container port will be 22, but it must be mapped to host port when running
# the container. If you are using the command line interface, this can be
# accomplished by running the image with the command `docker run -p 8022:22 <image_name>`.
# Then, ssh-ing into the container using `ssh -p 8022 bundledrefs@localhost` and the 
# password `bundledrefs`.
EXPOSE 22
RUN service ssh start
CMD ["/usr/sbin/sshd", "-D"]