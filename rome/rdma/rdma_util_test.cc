#include "rdma_util.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

TEST(ibdev2netdevTest, Test) {
  // Test plan: Try it out bro!
  auto result = ibdev2netdev("mlx5_0");
  EXPECT_OK(result);
  EXPECT_EQ(result.value(), "ibp153s0");
  EXPECT_TRUE(true);
}