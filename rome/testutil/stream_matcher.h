#pragma once

#include "gmock/gmock-matchers.h"
#include "status_matcher.h"
#include "absl/strings/str_cat.h"
#include "rome/colosseum/stream.h"

namespace testutil {

MATCHER(IsStreamTerminatedStatus,
        absl::StrCat("is ", negation ? "not " : "", "terminated")) {
  return rome::IsStreamTerminated(GetStatus(arg));
}

}  // namespace testutil