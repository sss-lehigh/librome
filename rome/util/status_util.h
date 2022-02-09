#pragma once

#include <sstream>
#include <string_view>

#include "absl/status/status.h"

namespace util {

template <absl::StatusCode ErrorCode>
class StatusBuilder {
 public:
  StatusBuilder() = default;
  StatusBuilder(const StatusBuilder& other) : ss_(other.ss_.str()) {}
  StatusBuilder(StatusBuilder&& other) : ss_(other.ss_.str()) {}

  // Add a string `s` to the internal `stringstream` to build a message.
  StatusBuilder<ErrorCode>& operator<<(std::string_view s) {
    ss_ << s;
    return *this;
  }

  // Conversion function enabling the builder to be used exactly like an
  // `absl::Status` object.
  operator absl::Status() const { return absl::Status(ErrorCode, ss_.str()); }

 private:
  std::stringstream ss_;
};

using UnavailableErrorBuilder = StatusBuilder<absl::StatusCode::kUnavailable>;
using CancelledErrorBuilder = StatusBuilder<absl::StatusCode::kCancelled>;
using NotFoundErrorBuilder = StatusBuilder<absl::StatusCode::kNotFound>;
using UnknownErrorBuilder = StatusBuilder<absl::StatusCode::kUnknown>;
using AlreadyExistsErrorBuilder =
    StatusBuilder<absl::StatusCode::kAlreadyExists>;
using FailedPreconditionErrorBuilder =
    StatusBuilder<absl::StatusCode::kFailedPrecondition>;
using InternalErrorBuilder = StatusBuilder<absl::StatusCode::kInternal>;

}  // namespace rome