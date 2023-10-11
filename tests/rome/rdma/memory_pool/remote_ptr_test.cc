#include "remote_ptr.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "rome/logging/logging.h"

namespace rome::rdma {
namespace {

class RemotePtrTest : public ::testing::Test {
  void SetUp() { ROME_INIT_LOG(); }
};

TEST(RemotePtrTest, Test) {
  // Test plan: test_plan
  remote_ptr<int> p = remote_nullptr;
  EXPECT_EQ(p, remote_nullptr);
}

TEST(RemotePtrTest, Equivalence) {
  remote_ptr<int> p1;
  EXPECT_TRUE(p1 == remote_nullptr);
}

TEST(RemotePtrTest, GettersTest) {
  remote_ptr<int> p;
  p = remote_ptr<int>(1, (uint64_t)0x0fedbeef);
  EXPECT_EQ(p.id(), 1);
  EXPECT_EQ(p.address(), 0x0fedbeef);
  EXPECT_EQ(p.raw(), (1ul << 48) | 0x0fedbeef);
}

TEST(RemotePtrTest, CopyTest) {
  remote_ptr<int> p;
  p = remote_ptr<int>(1, (uint64_t)0x0fedbeef);
  auto q = p;
  EXPECT_EQ(q.id(), 1);
  EXPECT_EQ(q.address(), 0x0fedbeef);
  EXPECT_EQ(q.raw(), (1ul << 48) | 0x0fedbeef);
}

TEST(RemotePtrTest, IncrementTest) {
  remote_ptr<int> p;
  p = remote_ptr<int>(4, (uint64_t)0);
  ++p;
  EXPECT_EQ(p.address(), sizeof(int));

  auto q = p++;
  EXPECT_EQ(q.address(), sizeof(int));
  EXPECT_EQ(p.address(), 2 * sizeof(int));

  auto r = (p += 4);
  EXPECT_EQ(r.address(), 6 * sizeof(int));
  EXPECT_EQ(p.address(), r.address());
}

}  // namespace
}  // namespace rome::rdma