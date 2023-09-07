#include "logging.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

class LoggingTest : public ::testing::Test {
 protected:
  void SetUp() { ROME_INIT_LOG(); }
  void TearDown() { ROME_DEINIT_LOG(); }
};

TEST_F(LoggingTest, InitLogTest) {
  // Test plan: test_plan
  EXPECT_TRUE(true);
}

TEST_F(LoggingTest, Trace) {
  ROME_TRACE("Hello, World!");
  EXPECT_TRUE(true);
}

TEST_F(LoggingTest, Debug) {
  ROME_DEBUG("Hello, World!");
  EXPECT_TRUE(true);
}

TEST_F(LoggingTest, Info) {
  ROME_INFO("Hello, World!");
  EXPECT_TRUE(true);
}

TEST_F(LoggingTest, Warn) {
  ROME_WARN("Hello, World!");
  EXPECT_TRUE(true);
}

TEST_F(LoggingTest, Error) {
  ROME_ERROR("Hello, World!");
  EXPECT_TRUE(true);
}

TEST_F(LoggingTest, CRITICAL) {
  ROME_CRITICAL("Hello, World!");
  EXPECT_TRUE(true);
}