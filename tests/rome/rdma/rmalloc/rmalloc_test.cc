#include "rmalloc.h"

#include <infiniband/verbs.h>

#include <memory>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "rome/rdma/connection_manager/connection_manager.h"
#include "rome/rdma/rdma_device.h"
#include "rome/rdma/rdma_util.h"

namespace rome::rdma {
namespace {

constexpr uint32_t kArenaCapacity = 1024;

template <uint32_t capacity>
struct MyStruct {
  std::array<std::byte, capacity> bytes;
};

class RdmaAllocatorTest : public ::testing::Test {
 protected:
  void SetUp() {
    ROME_INIT_LOG();
    dev_ = RdmaDevice::Create(RdmaDevice::GetAvailableDevices()->front().first,
                              std::nullopt);
    ASSERT_OK(dev_->CreateProtectionDomain("rdma_allocator"));
    pd_ = VALUE_OR_DIE(dev_->GetProtectionDomain("rdma_allocator"));
    memory_resource_ =
        std::make_unique<rdma_memory_resource>(kArenaCapacity, pd_);
  }

  std::unique_ptr<RdmaDevice> dev_;
  std::unique_ptr<rdma_memory_resource> memory_resource_;
  ibv_pd* pd_;
};

TEST_F(RdmaAllocatorTest, AllocateSingleUint64) {
  // Test plan: Allocate a single `uint64_t` element and check that the returned
  // memory is not `nullptr`.
  auto alloc = rdma_allocator<uint64_t>(memory_resource_.get());
  auto* x = alloc.allocate();
  EXPECT_NE(x, nullptr);
}

TEST_F(RdmaAllocatorTest, AllocateSingleUint64Repeated) {
  // Test plan: Allocate a single `uint64_t` element and check that the returned
  // memory is not `nullptr`.
  for (int i = 0; i < 10; ++i) {
    auto alloc = rdma_allocator<uint64_t>(memory_resource_.get());
    auto* x = alloc.allocate();
    EXPECT_NE(x, nullptr);
  }
}

TEST_F(RdmaAllocatorTest, AllocateMultipleUint64) {
  // Test plan: Allocate mutliple `uint64_t` elements and check that the
  // returned memory is not `nullptr`.
  auto alloc = rdma_allocator<uint64_t>(memory_resource_.get());
  auto* x = alloc.allocate(10);
  EXPECT_NE(x, nullptr);
}

TEST_F(RdmaAllocatorTest, ReallocateMultipleUint64) {
  // Test plan: Allocate mutliple `uint64_t` elements and check that the
  // returned memory is not `nullptr`. Then, deallocate them and check that a
  // reallocation of the same size returns valid memory.
  auto alloc = rdma_allocator<uint64_t>(memory_resource_.get());
  auto* x = alloc.allocate(10);
  ASSERT_NE(x, nullptr);
  alloc.deallocate(x, 10);
  x = alloc.allocate(10);
  EXPECT_NE(x, nullptr);
}

TEST_F(RdmaAllocatorTest, AllocateStruct) {
  // Test plan: Allocate a struct the size of the memory capacity of the
  // underlying `rdma_memory_resource` and check that the returned pointer to
  // memory is not `nullptr`.
  using TestStruct = MyStruct<kArenaCapacity>;
  auto alloc = rdma_allocator<TestStruct>(memory_resource_.get());
  auto* x = alloc.allocate();
  EXPECT_NE(x, nullptr);
}

TEST_F(RdmaAllocatorTest, AllocateStructFailure) {
  // Test plan: Allocate a struct greater than the size of the memory capacity
  // of the underlying `rdma_memory_resource` and check that the returned
  // pointer to memory is `nullptr`. In this case, the memory allocation request
  // cannot be serviced.
  using TestStruct = MyStruct<kArenaCapacity + 1>;
  auto alloc = rdma_allocator<TestStruct>(memory_resource_.get());
  auto* x = alloc.allocate();
  EXPECT_EQ(x, nullptr);
}

TEST_F(RdmaAllocatorTest, ReallocateStruct) {
  // Test plan: Allocate a struct the size of the memory capacity of the
  // underlying `rdma_memory_resource` and check that the returned pointer to
  // memory is not `nullptr`. Then, deallocate the struct and try again to
  // ensure that the memory is pulled from the freelist. Since the struct is the
  // memory capacity, a non-freelist allocation would fail due to the bump
  // allocation running out of free memory.
  using TestStruct = MyStruct<kArenaCapacity>;
  auto alloc = rdma_allocator<TestStruct>(memory_resource_.get());
  auto* x = alloc.allocate();
  ASSERT_NE(x, nullptr);
  ASSERT_EQ(sizeof(*x), sizeof(TestStruct));

  alloc.deallocate(x);
  x = alloc.allocate();
  EXPECT_NE(x, nullptr);
  EXPECT_EQ(sizeof(*x), sizeof(TestStruct));
}

TEST_F(RdmaAllocatorTest, ReallocateDifferentType) {
  // Test plan: Allocate a region of memory pointing to `uint8_t` that is
  // equivalent to the size of the underlying memory capacity. Then, deallocated
  // and create a new allocator that shares the underlying
  // `rdma_memory_resource` via a conversion constructor and reallocate a new
  // region of memory with the same size but different type.
  auto alloc = rdma_allocator<uint8_t>(memory_resource_.get());
  auto* x = alloc.allocate(kArenaCapacity);
  ASSERT_NE(x, nullptr);
  alloc.deallocate(x, kArenaCapacity);

  using TestStruct = MyStruct<kArenaCapacity>;
  rdma_allocator<TestStruct> new_alloc(alloc);
  auto* y = new_alloc.allocate();
  EXPECT_NE(y, nullptr);
}

TEST_F(RdmaAllocatorTest, RemotelyAccessMemory) {
  // Test plan: Allocate a region of memory then test that we can remotely
  // access it. Exemplifies how to use the allocator in practice.
  constexpr int kServerId = 0;
  constexpr int kClientId = 1;
  constexpr char kAddress[] = "10.0.0.1";
  using TestStruct = MyStruct<512>;

  ConnectionManager<RdmaChannel<EmptyRdmaMessenger, EmptyRdmaAccessor>> server(
      kServerId);
  ASSERT_OK(server.Start(kAddress, std::nullopt));
  rdma_memory_resource server_memory_resource(sizeof(TestStruct), server.pd());
  rdma_allocator<TestStruct> server_rmalloc(&server_memory_resource);
  auto* remote = server_rmalloc.allocate(1);

  ConnectionManager<RdmaChannel<EmptyRdmaMessenger, EmptyRdmaAccessor>> client(
      kClientId);
  ASSERT_OK(client.Start(kAddress, std::nullopt));
  rdma_memory_resource client_memory_resource(sizeof(TestStruct), client.pd());
  rdma_allocator<TestStruct> client_rmalloc(&client_memory_resource);
  auto* local = client_rmalloc.allocate(1);

  auto* client_conn =
      VALUE_OR_DIE(client.Connect(kServerId, server.address(), server.port()));

  ibv_sge sge;
  std::memset(&sge, 0, sizeof(sge));
  sge.addr = reinterpret_cast<uint64_t>(local);
  sge.length = sizeof(TestStruct);  // Read `num_nodes` nodes.
  sge.lkey = client_memory_resource.mr()->lkey;

  ibv_send_wr wr;
  std::memset(&wr, 0, sizeof(wr));
  wr.send_flags = IBV_SEND_SIGNALED;
  wr.num_sge = 1;
  wr.opcode = IBV_WR_RDMA_WRITE;
  wr.sg_list = &sge;
  wr.wr.rdma.remote_addr = reinterpret_cast<uint64_t>(remote);
  wr.wr.rdma.rkey = server_memory_resource.mr()->rkey;

  ROME_DEBUG("Accessing {} bytes @ {}", sge.length, fmt::ptr(remote));

  ibv_send_wr* bad;
  RDMA_CM_ASSERT(ibv_post_send, client_conn->id()->qp, &wr, &bad);

  ibv_wc wc;
  int ret = ibv_poll_cq(client_conn->id()->send_cq, 1, &wc);
  while ((ret < 0 && errno == EAGAIN) || ret == 0) {
    ret = ibv_poll_cq(client_conn->id()->send_cq, 1, &wc);
  }

  ROME_DEBUG(ibv_wc_status_str(wc.status));

  EXPECT_EQ(ret, 1);
  EXPECT_EQ(wc.status, IBV_WC_SUCCESS);
}

}  // namespace
}  // namespace rome::rdma