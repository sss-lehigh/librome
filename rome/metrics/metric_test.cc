#include "metric.h"

#include <random>

#include "absl/status/statusor.h"
#include "counter.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "rome/testutil/status_matcher.h"
#include "summary.h"

namespace rome::metrics {
namespace {

template <typename T>
class CounterTest : public testing::Test {
 protected:
  Counter<T> counter = Counter<T>("TestCounter");
};

using TestTypes = testing::Types<int, double>;
TYPED_TEST_SUITE(CounterTest, TestTypes);

TYPED_TEST(CounterTest, ArithmeticTest) {
  // Test plan: Check that the arithemetic operations do as we expect.
  EXPECT_EQ(this->counter, 0);

  // Test pre- and postfix increment.
  EXPECT_EQ(++(this->counter), 1);
  EXPECT_EQ((this->counter)++, 1);
  EXPECT_EQ(this->counter, 2);

  // Test pre- and postfix decrement.
  EXPECT_EQ(--(this->counter), 1);
  EXPECT_EQ((this->counter)--, 1);
  EXPECT_EQ(this->counter, 0);

  // Test addition and subtraction.
  EXPECT_EQ(this->counter += 10, 10);
  EXPECT_EQ(this->counter -= 20, -10);

  // Test assignment
  EXPECT_EQ(this->counter = 100, 100);
}

TYPED_TEST(CounterTest, AccumulatesCounterWithSameId) {
  // Test plan: Check that accumulation works when the ids of the counters are
  // the same.
  this->counter += 100;
  auto counter2 = Counter<TypeParam>("TestCounter", 100);

  EXPECT_OK(this->counter.Accumulate(counter2));
  EXPECT_EQ(this->counter.GetCounter(), 200);
}

TEST(DistributionTest, DistributionGetTest) {
  // Test plan: Create a distribution object and add values to it so that it
  // must recompute its internal statistics more than once. After, retrieve the
  // various statistics from it and compare to the expected values.
  double expected_mean = 1000.0;
  double expected_stddev = 15.0;

  bool initialized = false;
  int64_t expected_min;
  int64_t expected_max;
  std::vector<int64_t> observed_values;

  std::mt19937_64 mt;
  std::normal_distribution<double> dist(expected_mean, expected_stddev);
  auto summary = Summary<int64_t>("TestDistribution", "", 100000);
  for (int i = 0; i < 500000; ++i) {
    auto v = int64_t(dist(mt));
    expected_min = initialized ? std::min(expected_min, v) : v;
    expected_max = initialized ? std::max(expected_max, v) : v;
    if (!initialized) initialized = true;
    observed_values.push_back(v);
    summary << v;
  }

  // Compute the real
  std::sort(observed_values.begin(), observed_values.end());
  int64_t expected_p50 =
      observed_values[std::ceil(observed_values.size() * 0.5)];
  int64_t expected_p90 =
      observed_values[std::ceil(observed_values.size() * 0.9)];
  int64_t expected_p95 =
      observed_values[std::ceil(observed_values.size() * 0.95)];
  int64_t expected_p99 =
      observed_values[std::ceil(observed_values.size() * 0.99)];
  int64_t expected_p999 =
      observed_values[std::ceil(observed_values.size() * 0.999)];

  EXPECT_EQ(summary.GetMin(), expected_min);
  EXPECT_NEAR(summary.Get50thPercentile(), expected_p50, 1);
  EXPECT_NEAR(summary.Get90thPercentile(), expected_p90, 1);
  EXPECT_NEAR(summary.Get95thPercentile(), expected_p95, 1);
  EXPECT_NEAR(summary.Get99thPercentile(), expected_p99, 1);
  EXPECT_NEAR(summary.Get999thPercentile(), expected_p999, 1);
  EXPECT_EQ(summary.GetMax(), expected_max);
  EXPECT_NEAR(summary.GetMean(), expected_mean, expected_mean * 0.01);
  EXPECT_NEAR(summary.GetStddev(), expected_stddev, expected_stddev * 0.01);
}

}  // namespace
}  // namespace rome::metrics