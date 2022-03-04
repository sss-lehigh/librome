#pragma once

#include <infiniband/verbs.h>

#include <memory>

#include "rome/logging/logging.h"
#include "rome/util/status_util.h"

#define RDMA_CM_CHECK(func, ...)                                         \
  {                                                                      \
    int ret = func(__VA_ARGS__);                                         \
    ROME_CHECK_QUIET(ROME_RETURN(::util::InternalErrorBuilder()          \
                                 << #func << "(): " << strerror(errno)), \
                     ret == 0)                                           \
  }

#define RDMA_CM_ASSERT(func, ...)                                    \
  {                                                                  \
    int ret = func(__VA_ARGS__);                                     \
    ROME_ASSERT(ret == 0, "{}{}{}", #func, "(): ", strerror(errno)); \
  }

struct ibv_context_deleter {
  void operator()(ibv_context *device_ctx) { ibv_close_device(device_ctx); }
};
using ibv_context_unique_ptr =
    std::unique_ptr<ibv_context, ibv_context_deleter>;

struct ibv_pd_deleter {
  void operator()(ibv_pd *pd) { ibv_dealloc_pd(pd); }
};
using ibv_pd_unique_ptr = std::unique_ptr<ibv_pd, ibv_pd_deleter>;

struct ibv_device_list_deleter {
  void operator()(ibv_device **devices) { ibv_free_device_list(devices); }
};
using ibv_device_list_unique_ptr =
    std::unique_ptr<ibv_device *, ibv_device_list_deleter>;

struct ibv_mr_deleter {
  void operator()(ibv_mr *mr) { ibv_dereg_mr(mr); }
};
using ibv_mr_unique_ptr = std::unique_ptr<ibv_mr, ibv_mr_deleter>;