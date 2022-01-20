#include "fake_clock.h"

#include "rome/util/clocks.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace rome::testutil {

using rome::SteadyClock;

TEST(FakeClockTest, FakeClockIsNotSteady) {
  // Test plan: Check that the `fake_clock` is not steady.
  fake_clock::init();
  EXPECT_FALSE(fake_clock::is_steady);
}

TEST(FakeClockTest, FakeClockDoesNotAdvanceTest) {
  // Test plan: Construct a `fake_clock` and read the time using `now`. Then,
  // wait for some time before rereading it. Check that the time has not
  // changed.
  fake_clock::init();
  auto start = SystemClock::now();
  while (SystemClock::now() - start > std::chrono::seconds(1))
    ;
  EXPECT_EQ(fake_clock::now(), fake_clock::time_point(fake_clock::duration(0)));
}

TEST(FakeClockTest, FakeClockAdvancesByOne) {
  fake_clock::init();
  EXPECT_EQ(fake_clock::now(), fake_clock::time_point(fake_clock::duration(0)));
  fake_clock::advance();
  EXPECT_EQ(fake_clock::now(), fake_clock::time_point(fake_clock::duration(1)));
}

TEST(FakeClockTest, FakeClockAdvancesByX) {
  fake_clock::init();
  EXPECT_EQ(fake_clock::now(), fake_clock::time_point(fake_clock::duration(0)));
  fake_clock::advance(fake_clock::duration(10));
  EXPECT_EQ(fake_clock::now(),
            fake_clock::time_point(fake_clock::duration(10)));
}

} // namespace rome::testutil