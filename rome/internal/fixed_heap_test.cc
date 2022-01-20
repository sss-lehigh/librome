#include "fixed_heap.h"

#include <functional>
#include <random>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "rome/testutil/status_matcher.h"

namespace rome {

TEST(FixedMaxHeapTest, GetRootReturnsNotFound) {
  // Test plan: Call `GetRoot` on an empty heap and validate that it returns not
  // found.
  FixedMaxHeap<int, 100> heap;
  EXPECT_THAT(testutil::GetStatus(heap.GetRoot()),
              testutil::StatusIs(absl::StatusCode::kNotFound));
}

TEST(FixedMaxHeapTest, GetRootReturnsMaxValue) {
  // Test plan: Create a FixedMaxHeap. Insert elements checking the root to
  // verify that the heap behaves as expected for each insertion.
  FixedMaxHeap<int, 100> heap;
  EXPECT_OK(heap.Push(10));
  EXPECT_THAT(heap.GetRoot(), testutil::IsOkAndHolds(testing::Eq(10)));

  for (int i = 0; i < 10; ++i) {
    EXPECT_OK(heap.Push(1));
    EXPECT_THAT(heap.GetRoot(), testutil::IsOkAndHolds(testing::Eq(10)));
  }

  EXPECT_OK(heap.Push(20));
  EXPECT_THAT(heap.GetRoot(), testutil::IsOkAndHolds(testing::Eq(20)));
}

TEST(FixedMaxHeapTest, PopReturnsAndRemovesMaxValue) {
  // Test plan: Create a FixedMaxHeap. Insert elements checking the root to
  // verify that the heap behaves as expected for each insertion.
  FixedMaxHeap<int, 100> heap;
  EXPECT_OK(heap.Push(10));
  EXPECT_THAT(heap.Pop(), testutil::IsOkAndHolds(testing::Eq(10)));
  EXPECT_THAT(heap.Size(), 0);

  std::mt19937_64 mt;
  std::uniform_int_distribution<int> dist(0, 100000);
  std::vector<int> keys;
  for (int i = 0; i < 50; ++i) {
    auto key = dist(mt);
    EXPECT_OK(heap.Push(key));
    keys.push_back(key);
  }

  std::sort(keys.begin(), keys.end(), [](int a, int b) { return a > b; });

  for (auto k : keys) {
    EXPECT_THAT(heap.Pop(), testutil::IsOkAndHolds(testing::Eq(k)));
  }
  EXPECT_THAT(heap.Size(), 0);
}

}  // namespace rome