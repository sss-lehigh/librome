#include "rdma_memory.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "rdma_device.h"
#include "rome/logging/logging.h"
#include "rome/testutil/status_matcher.h"

namespace rome::rdma {
namespace {

class RdmaMemoryTest : public ::testing::Test {
 protected:
  ibv_pd* GetTestPd() { return pd_; }

  void SetUp() {
    auto devices = RdmaDevice::GetAvailableDevices();
    ASSERT_OK(devices);
    auto device = devices->front();
    dev_ = RdmaDevice::Create(device.first, std::nullopt);
    ASSERT_OK(dev_->CreateProtectionDomain("test"));
    auto pd = dev_->GetProtectionDomain("test");
    ASSERT_OK(pd);
    pd_ = *pd;
  }

  std::unique_ptr<RdmaDevice> dev_;
  ibv_pd* pd_;
};

TEST_F(RdmaMemoryTest, NoHugepages) {
  // Test plan: Check that a new `RdmaMemory` object can be successfully created
  // when there is no hugepage support.
  const uint64_t kCapacity = 1UL << 20;  // 1 MiB
  ROME_INFO("Warning about hugepages is expected.");
  RdmaMemory rmem(kCapacity, GetTestPd());
}

TEST_F(RdmaMemoryTest, Hugepages) {
#if defined(X_NO_HUGEPAGES)
  ROME_WARN("Skipping hugepages test...");
#else
  const uint64_t kCapacity = 1UL << 10;  // 1 KiB
  RdmaMemory rmem(kCapacity, "/proc/sys/vm/nr_hugepages", GetTestPd());
#endif
}

}  // namespace
}  // namespace rome