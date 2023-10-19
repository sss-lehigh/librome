#include "sync_accessor.h"

#include <asm-generic/errno-base.h>
#include <infiniband/verbs.h>

#include "rome/logging/logging.h"
#include "rome/rdma/rdma_util.h"
#include "rome/util/status_util.h"

namespace rome::rdma {

using ::util::InternalErrorBuilder;

SyncRdmaAccessor::SyncRdmaAccessor(rdma_cm_id *id) : id_(id) {}

absl::Status SyncRdmaAccessor::PostInternal(ibv_send_wr *wr,
                                            ibv_send_wr **bad) {
  *bad = nullptr;
  RDMA_CM_CHECK(ibv_post_send, id_->qp, wr, bad);
  if (*bad != nullptr) {
    ROME_FATAL("WTF");
  }

  ibv_wc wc;
  auto ret = ibv_poll_cq(id_->send_cq, 1, &wc);
  while (ret == 0 || (ret < 0 && errno == EAGAIN)) {
    ret = ibv_poll_cq(id_->send_cq, 1, &wc);
  }
  ROME_CHECK_QUIET(ROME_RETURN(InternalErrorBuilder()
                               << "ibv_poll_cq(): "
                               << (ret < 0 ? strerror(errno)
                                           : ibv_wc_status_str(wc.status))),
                   ret == 1 && wc.status == IBV_WC_SUCCESS);
  return absl::OkStatus();
}

}  // namespace rome