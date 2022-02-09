#include "stopwatch.h"

#include <chrono>
#include <random>
#include <thread>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "rome/logging/logging.h"
#include "rome/util/clocks.h"

namespace rome::metrics {

TEST(StopwatchTest, TotalRuntimeIsAccurate) {
  // Test plan: Create a new Stopwatch, wait one second, then stop the stopwatch
  // and check that the total runtime is within some error of 1 second.
  ROME_INIT_LOG();
  std::mt19937_64 mt;
  std::uniform_int_distribution<> dist(1000, 1000000000);
  auto expected_ns = dist(mt);

  auto stopwatch = Stopwatch::Create("test");
  auto start = util::SystemClock::now();
  while (util::SystemClock::now() - start <
         std::chrono::nanoseconds(expected_ns))
    ;
  stopwatch->Stop();
  EXPECT_NEAR(stopwatch->GetRuntimeNanoseconds().count(), expected_ns,
              0.01 * expected_ns);
}

}  // namespace rome::metrics