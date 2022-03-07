#pragma once

#include "absl/status/status.h"

namespace rome {

class RdmaClientInterface {
 public:
  virtual absl::Status Connect(std::string_view server, uint16_t port) = 0;
};

}  // namespace rome