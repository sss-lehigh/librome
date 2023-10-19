#include "workload_driver.h"

#include <chrono>
#include <memory>
#include <ratio>
#include <sstream>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "rome/colosseum/client_adaptor.h"
#include "rome/colosseum/streams/streams.h"
#include "rome/testutil/status_matcher.h"
#include "rome/util/clocks.h"

namespace rome {

using ::testutil::StatusIs;
using ::util::SystemClock;

struct FakeClientOp {
  std::chrono::milliseconds wait;
  std::string data;
};

class FakeClientAdaptor : public ClientAdaptor<FakeClientOp> {
 public:
  absl::Status Start() override { return absl::OkStatus(); }
  absl::Status Apply(const FakeClientOp& op) override {
    auto start = SystemClock::now();
    while (SystemClock::now() - start < op.wait)
      ;
    ss_ << op.data;
    return absl::OkStatus();
  }
  absl::Status Stop() override { return absl::OkStatus(); }

  std::string GetString() { return ss_.str(); }

 private:
  std::stringstream ss_;
};

TEST(WorkloadDriverTest, ClientHandlesSingleOp) {
  // Test plan: Create a workload driver that will feed the FakeClientAdaptor
  // with a single operation for which the client will wait 100ms then add
  // "Hello" to its internal string. Then start the driver, and wait 2 seconds.
  // After, check that the total runtime is one second (i.e., the actual time
  // spent processing requests) and that the string is as expected.
  auto expected_s = std::chrono::milliseconds(100);
  auto client = std::make_unique<FakeClientAdaptor>();
  auto* client_ptr = client.get();
  auto stream = std::make_unique<TestStream<FakeClientOp>>(
      std::vector<FakeClientOp>{{expected_s, "Hello"}});
  auto driver = WorkloadDriver<FakeClientOp>::Create(
      std::move(client), std::move(stream), nullptr,
      std::chrono::milliseconds(0));

  EXPECT_OK(driver->Start());
  auto start = SystemClock::now();
  while (SystemClock::now() - start < std::chrono::milliseconds(200))
    ;

  auto expected_ns = std::chrono::nanoseconds(expected_s);
  EXPECT_OK(driver->Stop());
  EXPECT_EQ(client_ptr->GetString(), "Hello");
  EXPECT_NEAR(driver->GetStopwatch()->GetRuntimeNanoseconds().count(),
              expected_ns.count(), expected_ns.count() * 0.01);
}

TEST(WorkloadDriverTest, ClientHandlesMultipleOps) {
  auto wait = std::chrono::milliseconds(100);
  auto client = std::make_unique<FakeClientAdaptor>();
  auto* client_ptr = client.get();
  auto stream = std::make_unique<TestStream<FakeClientOp>>(
      std::vector<FakeClientOp>{{wait, "Hello"},
                                {wait, ""},
                                {wait, ","},
                                {wait, " "},
                                {wait, "World!"}});
  auto driver = WorkloadDriver<FakeClientOp>::Create(
      std::move(client), std::move(stream), nullptr,
      std::chrono::milliseconds(0));
  ASSERT_OK(driver->Start());
  auto start = SystemClock::now();
  while (SystemClock::now() - start < std::chrono::seconds(1))
    ;

  ASSERT_OK(driver->Stop());
  EXPECT_EQ(client_ptr->GetString(), "Hello, World!");
}

TEST(WorkloadDriverTest, CannotRestartStoppedDriver) {
  // Test plan: Create a dummy driver that is immediately stopped after being
  // started, then check that trying to restart the driver fails.
  auto client = std::make_unique<FakeClientAdaptor>();
  auto stream = std::make_unique<TestStream<FakeClientOp>>(
      std::vector<FakeClientOp>{{std::chrono::seconds(0), ""}});
  auto driver = WorkloadDriver<FakeClientOp>::Create(
      std::move(client), std::move(stream), nullptr,
      std::chrono::milliseconds(0));
  ASSERT_OK(driver->Start());
  ASSERT_OK(driver->Stop());
  EXPECT_THAT(driver->Start(), StatusIs(absl::StatusCode::kUnavailable));
}

TEST(WorkloadDriverTest, CannotStopStoppedDriver) {
  // Test plan: Create a dummy driver that is immediately stopped after being
  // started, then chekc that trying to stop the driver again fails.
  auto client = std::make_unique<FakeClientAdaptor>();
  auto stream = std::make_unique<TestStream<FakeClientOp>>(
      std::vector<FakeClientOp>{{std::chrono::seconds(0), ""}});
  auto driver = WorkloadDriver<FakeClientOp>::Create(
      std::move(client), std::move(stream), nullptr,
      std::chrono::milliseconds(0));
  ASSERT_OK(driver->Start());
  ASSERT_OK(driver->Stop());
  EXPECT_THAT(driver->Stop(), StatusIs(absl::StatusCode::kUnavailable));
}

}  // namespace rome