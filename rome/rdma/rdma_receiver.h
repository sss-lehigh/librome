#pragma once

#include <rdma/rdma_cma.h>

#include "absl/status/status.h"

namespace rome {

// Interface used by `RdmaBroker` to handle connection requests.
class RdmaReceiverInterface {
 public:
  ~RdmaReceiverInterface() = default;

  // Prepares the new connection for the `rmda_accept` call. Receives `id`,
  // which is the new connection allocated upon receiving the connection
  // request. The implementation should then create or assign a QP to `id`.
  virtual absl::Status OnConnectRequest(rdma_cm_id* id) = 0;

  // The following are called whenever their respective signals are received by
  // an `RdmaBroker` object. Unlike `OnConnectRequest` there are no requirements
  // for the implementation. However, the `id` is destroyed after `OnDisconnect`
  // is called, and therefore the implementation should ensure that the endpoint
  // is not used after `OnDisconnect` is called.
  virtual void OnEstablished(rdma_cm_id* id) = 0;
  virtual void OnDisconnect(rdma_cm_id* id) = 0;
};

}  // namespace rome