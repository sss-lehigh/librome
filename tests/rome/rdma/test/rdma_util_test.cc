#include "rdma_util.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "rome/logging/logging.h"

TEST(ibdev2netdevTest, Test) {
  // Test plan: Try it out bro!
  ROME_INIT_LOG();
  auto result = ibdev2netip("mlx5_0");
  ASSERT_OK(result);
  EXPECT_EQ(result.value(), "10.0.0.1");
  EXPECT_TRUE(true);
}