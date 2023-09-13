/// @file

#pragma once

#if defined(__NVCC__) || (defined(__clang__) && defined(__CUDA__))

#define ROME_HOST __host__ __forceinline__
#define ROME_DEVICE __device__ __forceinline__
#define ROME_HOST_DEVICE __host__ __device__ __forceinline__

#else

#define ROME_HOST inline
#define ROME_DEVICE inline
#define ROME_HOST_DEVICE inline

#endif

