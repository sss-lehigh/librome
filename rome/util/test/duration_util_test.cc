#include "rome/util/duration_util.h"

#include <chrono>
#include <ratio>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace util {

TEST(UtilTest, ToDoubleNanosecondsTest) {
  std::chrono::nanoseconds d(1000);
  EXPECT_THAT(ToDoubleNanoseconds(d), testing::DoubleEq(1000.0));
}

TEST(UtilTest, ToDoubleMicrosecondsTest) {
  std::chrono::microseconds d(1000);
  EXPECT_THAT(ToDoubleMicroseconds(d), testing::DoubleEq(1000.0));
}

TEST(UtilTest, ToDoubleMillisecondsTest) {
  std::chrono::milliseconds d(1000);
  EXPECT_THAT(ToDoubleMilliseconds(d), testing::DoubleEq(1000.0));
}

TEST(UtilTest, ToDoubleSecondsTest) {
  std::chrono::seconds d(1000);
  EXPECT_THAT(ToDoubleSeconds(d), testing::DoubleEq(1000.0));
}

}  // namespace util