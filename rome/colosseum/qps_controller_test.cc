#include "qps_controller.h"

#include <chrono>
#include <thread>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "rome/testutil/fake_clock.h"
#include "rome/util/clocks.h"
#include "rome/util/duration_util.h"

namespace rome {
namespace {

using ::testutil::FakeClock;
using ::util::SteadyClock;

TEST(QpsControllerTest, FakeClockLeakyTokenBucketQpsControllerTest) {
  // Test plan: Create a controller that only allows 100 operations per-second.
  // Call `Wait` 100 times on the controller. Then, create a thread that calls
  // `Wait` then writes to a boolean `done`. Outside of the thread, check that
  // it has not completed, wait for one second and check again that nothing
  // changed during that period. Finally, advance the fake clock by one second
  // and check that the thread terminates and the boolean flag is set.
  auto qps_controller = LeakyTokenBucketQpsController<FakeClock>::Create(100);
  auto qps_controller_ptr = qps_controller.get();
  for (int i = 0; i < 100; ++i) {
    qps_controller->Wait();
  }

  bool done = false;
  std::thread t([&done, qps_controller_ptr]() {
    qps_controller_ptr->Wait();
    done = true;
  });

  EXPECT_FALSE(done);
  auto start = SteadyClock::now();
  while (SteadyClock::now() - start < std::chrono::seconds(1))
    ;
  EXPECT_FALSE(done);

  FakeClock::advance(std::chrono::seconds(1));
  t.join();
  EXPECT_TRUE(done);
}

TEST(QpsControllerTest, RealClockLeakyTokenBucketQpsControllerTest) {
  // Test plan: Creates a QPS controller that only allows 100 QPS. Then
  // repeatedly calls `Wait` on the controller and checks that the runtime is
  // within a margin of error of the expected value. This expected value is 2
  // seconds because the first 100 operations enter the controller without
  // delay, and the following 100 will take 1 second while the first 100 drain.
  // Likewise, the last 100 operations will take another second.
  auto qps_controller = LeakyTokenBucketQpsController<SteadyClock>::Create(100);
  auto start = SteadyClock::now();
  for (int i = 0; i < 300; ++i) {
    qps_controller->Wait();
  }
  auto end = SteadyClock::now();
  EXPECT_NEAR(util::ToDoubleMilliseconds(end - start), 2000.0, 0.05);
}

}  // namespace
}  // namespace rome