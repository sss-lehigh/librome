#!/usr/bin/env bash

wget https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2204/x86_64/cuda-keyring_1.1-1_all.deb
sudo dpkg -i cuda-keyring_1.1-1_all.deb
sudo apt-get update
sudo apt-get -y install cuda-compiler-12-2 cuda-cudart-dev-12-2 cuda-nvtx-12-2 cuda-nvrtc-dev-12-2 libcurand-dev-12-2 cuda-cccl-12-2

CUDA_PATH=/usr/local/cuda
echo "CUDA_PATH=${CUDA_PATH}"
export CUDA_PATH=${CUDA_PATH}
export PATH="$CUDA_PATH/bin:$PATH"
export LD_LIBRARY_PATH="$CUDA_PATH/lib:$LD_LIBRARY_PATH"
export LD_LIBRARY_PATH="$CUDA_PATH/lib64:$LD_LIBRARY_PATH"
# Check nvcc is now available.
nvcc -V

echo "CUDA_PATH=$CUDA_PATH" >> "$GITHUB_ENV"
echo "$CUDA_PATH/bin" >> "$GITHUB_ENV" # add to path
echo "LD_LIBRARY_PATH=${LD_LIBRARY_PATH}" >> "$GITHUB_ENV"

