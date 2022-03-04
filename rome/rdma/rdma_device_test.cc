#include "rdma_device.h"

#include <infiniband/verbs.h>

#include <algorithm>
#include <optional>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "rome/logging/logging.h"
#include "rome/testutil/status_matcher.h"

namespace rome {
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
  auto **device_list = ibv_get_device_list(&num_devices);
  auto got = RdmaDevice::GetAvailableDevices();
  if (num_devices == 0) {
    // There might be no available devices...
    ROME_WARN("No devices found");
    EXPECT_THAT(got, StatusIs(absl::StatusCode::kNotFound));
    EXPECT_THAT(got, HasMessage("No devices found"));
  } else {
    ASSERT_OK(got);
    ASSERT_EQ(got->size(), num_devices);
    for (int i = 0; i < num_devices; ++i) {
      auto dev_name = device_list[i]->name;
      EXPECT_NE(std::find(got->begin(), got->end(), dev_name), got->end());
    }
  }
}

TEST(SimpleRdmaDeviceTest, OpenDeviceOnFirstAvailablePort) {
  // Test-plan: Attempt to open a device without providing a port number.
  auto got = RdmaDevice::GetAvailableDevices();
  ASSERT_THAT(got, IsOkAndHolds(ResultOf(
                       std::bind(&std::vector<std::string>::size, got.value()),
                       Gt(0))));
  for (auto d : got.value()) {
    // Here, since we use RAII the construction of the device is the
    // initialization of the underlying resource. Therefore, a succeeding
    // constructor indicates that the device has been successfully open and a
    // port found.
    ROME_INFO("Testing device: {}", d);
    auto device = RdmaDevice::Create(d, std::nullopt);
    EXPECT_NE(device, nullptr);
  }
}

TEST(SimpleRdmaDeviceTest, OpenDeviceOnGivenPort) {
  // Test-plan: Open a device and get its port. Then, reopen it with the given
  // port and check that it succeeds.
  auto got = RdmaDevice::GetAvailableDevices();
  ASSERT_THAT(got, IsOkAndHolds(ResultOf(
                       std::bind(&std::vector<std::string>::size, got.value()),
                       Gt(0))));
  for (auto d : got.value()) {
    auto first = RdmaDevice::Create(d, std::nullopt);
    ASSERT_NE(first, nullptr);
    auto second = RdmaDevice::Create(d, first->port());
  }
}

class RdmaDeviceTest : public ::testing::Test {
 protected:
  void SetUp() {
    auto devices = RdmaDevice::GetAvailableDevices();
    ASSERT_THAT(devices,
                IsOkAndHolds(ResultOf(
                    std::bind(&std::vector<std::string>::size, devices.value()),
                    Gt(0))));

    for (auto d : devices.value()) {
      dev = RdmaDevice::Create(d, std::nullopt);  // Get whatever port.
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