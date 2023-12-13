#include "rdma_device.h"

#include <infiniband/verbs.h>

#include <algorithm>
#include <optional>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "rome/logging/logging.h"
#include "rome/testutil/status_matcher.h"

namespace rome::rdma {
namespace {

using ::testing::Gt;
using ::testing::ResultOf;
using ::testutil::HasMessage;
using ::testutil::IsOkAndHolds;
using ::testutil::StatusIs;

TEST(SimpleRdmaDeviceTest, GetDeviceNamesReturnsAvailableDevices) {
  // Test plan: Call `GetDeviceNames` and compare it to the devices returned by
  // invoking the verbs library directly. This is just a sanity check that the
  // expected devices appear.
  std::vector<std::string> want;
  int num_devices;
  auto available = RdmaDevice::GetAvailableDevices();
  ibv_get_device_list(&num_devices);
  if (num_devices == 0) {
    // There might be no available devices...
    ROME_WARN("No devices found");
    EXPECT_THAT(available, StatusIs(absl::StatusCode::kNotFound));
    EXPECT_THAT(available, HasMessage("No devices found"));
  } else {
    ASSERT_OK(available);
    for (auto& a : *available) {
      auto dev = RdmaDevice::Create(a.first, a.second);
      EXPECT_NE(dev, nullptr);
    }
  }
}

TEST(SimpleRdmaDeviceTest, OpenDeviceOnFirstAvailablePort) {
  // Test-plan: Attempt to open a device without providing a port number.
  auto available = RdmaDevice::GetAvailableDevices();
  ASSERT_THAT(available,
              IsOkAndHolds(ResultOf(
                  std::bind(&std::vector<std::pair<std::string, int>>::size,
                            available.value()),
                  Gt(0))));
  std::string last = "";
  for (auto a : available.value()) {
    // Here, since we use RAII the construction of the device is the
    // initialization of the underlying resource. Therefore, a succeeding
    // constructor indicates that the device has been successfully open and a
    // port found.
    if (last == a.first) continue;

    last = a.first;
    ROME_INFO("Testing device: {}", a.first);
    auto device = RdmaDevice::Create(a.first, std::nullopt);
    EXPECT_NE(device, nullptr);
    EXPECT_EQ(device->port(), a.second);
  }
}

class RdmaDeviceTest : public ::testing::Test {
 protected:
  void SetUp() {
    auto available = RdmaDevice::GetAvailableDevices();
    ASSERT_THAT(available,
                IsOkAndHolds(ResultOf(
                    std::bind(&std::vector<std::pair<std::string, int>>::size,
                              available.value()),
                    Gt(0))));

    for (auto d : available.value()) {
      dev = RdmaDevice::Create(d.first, std::nullopt);  // Get whatever port.
      if (dev == nullptr) continue;
    }
    ASSERT_NE(dev, nullptr);
  }

  std::unique_ptr<RdmaDevice> dev;
};

TEST_F(RdmaDeviceTest, CreateProtectionDomain) {
  const std::string kDomainId = "MyDomain";
  EXPECT_OK(dev->CreateProtectionDomain(kDomainId));
  EXPECT_THAT(dev->GetProtectionDomain(kDomainId),
              IsOkAndHolds(testing::Ne(nullptr)));
}

}  // namespace
}  // namespace rome