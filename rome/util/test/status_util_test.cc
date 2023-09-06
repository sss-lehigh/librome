#include "status_util.h"

#include "absl/status/status.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "rome/testutil/status_matcher.h"

namespace util {
namespace {

TEST(StatusUtilTest, BuildsStatus) {
  // Test plan: Create a StatusBuilder and check that the output status is
  // expected.
  StatusBuilder<absl::StatusCode::kUnavailable> builder;
  builder << "TESTING!";
  EXPECT_THAT(builder, testutil::StatusIs(absl::StatusCode::kUnavailable));
  EXPECT_THAT(builder, testutil::HasMessage("TESTING!"));
}

}  // namespace
}  // namespace util