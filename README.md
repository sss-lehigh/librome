*UNDER CONSTRUCTION: Proceed with caution*

# Overview

librome is a collection of useful utilities for research-oriented programming. 
This is the ethos of this project, to accumulate helpful tools that can serve as the basis for development.

Some current features are:
* Workload driver library (see `rome/colosseum`) for experimental evaluation
* Data structures (see `rome/internal`)
* Logging utilities (see `rome/logging`)
* Measurements library (see `rome/metrics`)
* Test utilities (see `rome/testutil`)
* Sundry other utilities (see `rome/utilitites`)

The wishlist includes:
* Portability utilities
* Allocation libraries
* RDMA support
* NUMA utilities

As they say, "Rome wasn't built in a day".

# Directory Structure

* include : contains all the include files
* src : contains sources to be compiled
* tools : contains tools for interacting with rome
* scripts : contains setup scripts
* share : contains cmake files for installation
* tests : contains tests for building and testing rome
* cmake : contains files to find dependencies
* gladiators : experimental tests used for improving rome

# Workload Driver Library (`rome/colosseum`)
In order to test a system, there must be some kind of workload delivered to it.
The focus of this library is to provide an easy-to-use library to do so.
The first important concept is a `ClientAdaptor`, which represents an entity that translates operations provided by a workload driver into the actual operations that need to be performed on a given client.
Operations are produced by a `Stream`, which is an abstraction wrapping a sequence of outputs that may be infinite, or not.
A `MappedStream` outputs a single stream from one or more input streams.
This is the primary way to generate operations passed to the `ClientAdaptor`.
It is the job of a `WorkloadDriver` to take the operations produced by a `Stream` and apply them to a `ClientAdaptor`.
This process continues until either the `Stream` terminates or there is an error and can be controlled by a `QpsController` that limits the throughput of operations.
Using a `QpsController` is helpful to determine max throughput by ramping up the throughput over time, or to hold the QPS at a constant value to understand behavior under a certain load.

For more details and examples of how to use this portion of the project, take a peek at the code and tests in `rome/colosseum`.
The name of the folder is tounge-in-cheek, but the Colosseum was where gladiators put their abilities to the test, so it felt suitable.

# Setup Instructions

Make sure to have cmake version 3.23 or higher installed as well as a suitable compiler.

librome has been tested with:

|OS           |  Compiler            |
|-------------|----------------------|
|Ubuntu 22.04 | gcc-11               |
|Ubuntu 22.04 | gcc-12               |
|Ubuntu 22.04 | clang-15             |
|Ubuntu 22.04 | clang-14             |
|Ubuntu 22.04 | clang-15 & nvcc-12.2 |

Get your dependencies from your OS vendor or provide your own. 

librome uses:

* spdlog
* gtest
* gmock
* google benchmark
* absl
* rdmacm
* ibverbs
* fmt
* protobuf
* C++ coroutines - provided through gcc or clang

For Ubuntu 22.04 the following packages can be installed through apt: 

* libabsl-dev
* librdmacm-dev
* libibverbs-dev
* libgtest-dev
* libbenchmark-dev
* libfmt-dev
* libspdlog-dev
* protobuf-compiler
* libgmock-dev

`cicd/install_dependencies_ubuntu.sh` is a script for installing these on Ubuntu 22.04.

The build process consists of creating a build directory and from that directory running:

```
$ cmake <project directory>
$ make -j
```

Compilation can optionally be configured through defining the following:

* `HAVE_RDMA_CARD`
* `USE_CUDA`
* `LOG_LEVEL`
* `CXX_STANDARD`

These options can be defined by passing -D\<option\>=\<value\> to cmake.

The Environment variables `CXX` and `CUDAHOSTCXX` can be defined to set the compiler (e.g. `CXX=g++-12 cmake .. && make -j` will compile with g++-12).

Make sure to clear your build directory if recompiling with a different compiler.

`make install` will install librome in your default installation location or the directory passed through defining `CMAKE_INSTALL_PREFIX`.

## Docker

Create and run a docker container to emulate the build enviornment with the minimum dependencies installed

```{bash}
docker build --tag sss-dev --file DevDockerfile .
docker run --privileged --rm -v {MOUNT_DIR}:/home --name sss -it sss-dev
```

You can then develop from that container using the Dev Container extension so you can take full advantage of syntax highlighting and be able to build locally.

# Old Setup instructions (unsure if this still works)
The Dockerfile contains all the dependencies required by this project and handles automatically setting up the correct development environment.
There are enough comments in the Dockerfile itself to understand what is going on, but at a high level its main purpose is to install the tooling necessary to build the project.
The manually installed packages include (see `Dockerfile` for details):
* Golang
* Bazel 
* GCC and Clang compilers w/ C++20 support
* Miniconda

Together, these form the basis for my typical C++ development environment, building projects with Bazel and using Python for scripting.
Golang is included because it is the easiest way to install Bazel and its build tools.

<!-- Setup assumes that the user is using `bash`, but perhaps it would be better to leave that as a configurable option... -->
For now, we assume that the user's shell of choice is `bash`, but that could change in the future.

## VirtualBox (host=MacOS, guest=Ubuntu)
First, set up a shared folder in the settings to point to the project directory on the host.
For illustration purposes, let's assume that location to be `/home/dev/rome` and the name of the shared folder to be `rome`.
Next, in the `/etc/fstab` file on the guest OS, run the following command:
`echo -e "\n# Added for VBox\nrome /mnt/rome vboxsf defaults,uid=$(id -u) 0 1" | sudo tee -a /etc/fstab 1> /dev/null`.
At startup, the machine will mount the shared folder called `rome` at the mount point `/mnt/rome` on the guest.
To allow a user to write to the mounted directory, we must also provide a users ID to the `uid` option so that the owner of the mount point is that particular user.
Do not forget to add port forwarding so that we can connect to the machine over `ssh` (needed for VSCode)

Next, you'll want to enable symlink capabilities to the VM so that Bazel behaves as expected.
For MacOS this can be done with `VBoxManage setextradata VM_NAME VBoxInternal2/SharedFoldersEnableSymlinksCreate/SHARE_NAME 1`

## UTM (host=MacOS, guest=Ubuntu)
Similar to VirtualBox, UTM provides a hypervisor for VMs running on MacOS.
The benefit of running UTM is that it is amenable to Apple Silicon.
The user interface is also much nicer, in my opinion.
Generally, the setup is basically the same except that the file system type is `davfs`.
This requires that you first install the `davfs2` and then create a mount point similar to VirtualBox except that the endpoint is not a hosted WebDAV file system at `localhost:9843`.
To mount the shared folder, you need to call `mount -t davfs -o <options> http://localhost:9843 /mnt/rome`.
Equivalently, you can also set up `/etc/fstab` to mount the remote file system on boot.
One peculiarity for UTM's `davfs` setup is that it requires a username and password, which don't exist.
When prompted, just hit enter.
To avoid the prompt altogether, you can update `/etc/davfs2/secrets` to include a line for the hosted files.
In my configuration, I simply put the following line: `http://localhost:9843 user passwd`.
