#pragma once

#include <rdma/rdma_cma.h>

#include "absl/status/status.h"

namespace rome {

class RdmaAccessor {
 public:
  virtual ~RdmaAccessor() = default;
  virtual absl::Status Placeholder() = 0;
};

class EmptyRdmaAccessor : public RdmaAccessor {
 public:
  ~EmptyRdmaAccessor() = default;
  explicit EmptyRdmaAccessor(rdma_cm_id* id) {}
  absl::Status Placeholder() override { return absl::OkStatus(); }
};

}  // namespace rome