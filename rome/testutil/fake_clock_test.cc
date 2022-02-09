#include "fake_clock.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "rome/util/clocks.h"

namespace testutil {

using util::SteadyClock;
using util::SystemClock;

TEST(FakeClockTest, FakeClockIsNotSteady) {
  // Test plan: Check that the `FakeClock` is not steady.
  FakeClock::init();
  EXPECT_FALSE(FakeClock::is_steady);
}

TEST(FakeClockTest, FakeClockDoesNotAdvanceTest) {
  // Test plan: Construct a `FakeClock` and read the time using `now`. Then,
  // wait for some time before rereading it. Check that the time has not
  // changed.
  FakeClock::init();
  auto start = SystemClock::now();
  while (SystemClock::now() - start > std::chrono::seconds(1))
    ;
  EXPECT_EQ(FakeClock::now(), FakeClock::time_point(FakeClock::duration(0)));
}

TEST(FakeClockTest, FakeClockAdvancesByOne) {
  FakeClock::init();
  EXPECT_EQ(FakeClock::now(), FakeClock::time_point(FakeClock::duration(0)));
  FakeClock::advance();
  EXPECT_EQ(FakeClock::now(), FakeClock::time_point(FakeClock::duration(1)));
}

TEST(FakeClockTest, FakeClockAdvancesByX) {
  FakeClock::init();
  EXPECT_EQ(FakeClock::now(), FakeClock::time_point(FakeClock::duration(0)));
  FakeClock::advance(FakeClock::duration(10));
  EXPECT_EQ(FakeClock::now(), FakeClock::time_point(FakeClock::duration(10)));
}

}  // namespace testutil