#include "memory_pool.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <random>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "remote_ptr.h"
#include "rome/rdma/connection_manager/connection_manager.h"

namespace rome::rdma {
namespace {

constexpr char kIpAddress[] = "10.0.0.1";
const MemoryPool::Peer kServer = {1, kIpAddress, 18018};
const MemoryPool::Peer kClient = {2, kIpAddress, 18015};

class MemoryPoolTest : public ::testing::Test {
 protected:
  void SetUp() { ROME_INIT_LOG(); }
};

class LoopbackPolicy : public ::testing::Test {
 protected:
  void SetUp() {
    ROME_INIT_LOG();
    using CM = ConnectionManager<MemoryPool::channel_type>;
    mp_ = std::make_unique<MemoryPool>(p_, std::make_unique<CM>(p_.id));
    ASSERT_OK(mp_->Init(1ul << 12, {p_}));  // Set up loopback
  }

  template <typename T>
  remote_ptr<T> AllocateClient(size_t s = 1) {
    return AllocateServer<T>(s);
  }

  template <typename T>
  remote_ptr<T> AllocateServer(size_t s = 1) {
    return mp_->Allocate<T>(s);
  }

  template <typename T>
  remote_ptr<T> Read(remote_ptr<T> ptr,
                     remote_ptr<T> prealloc = remote_nullptr) {
    return mp_->Read(ptr, prealloc);
  }

  template <typename T>
  remote_ptr<T> PartialRead(remote_ptr<T> ptr, size_t offset, size_t bytes,
                            remote_ptr<T> prealloc = remote_nullptr) {
    return mp_->PartialRead(ptr, offset, bytes, prealloc);
  }

  template <typename T>
  void Write(remote_ptr<T> ptr, const T& value,
             remote_ptr<T> prealloc = remote_nullptr) {
    return mp_->Write(ptr, value, prealloc);
  }

  template <typename T>
  T AtomicSwap(remote_ptr<T> ptr, uint64_t swap, uint64_t hint = 0) {
    return mp_->AtomicSwap(ptr, swap, hint);
  }

  template <typename T>
  T CompareAndSwap(remote_ptr<T> ptr, uint64_t expected, uint64_t swap) {
    return mp_->CompareAndSwap(ptr, expected, swap);
  }

  MemoryPool::DoorbellBatchBuilder CreateDoorbellBatchBuilder(int num_ops) {
    return MemoryPool::DoorbellBatchBuilder(mp_.get(), p_.id, num_ops);
  }

  void Execute(MemoryPool::DoorbellBatch* batch) { return mp_->Execute(batch); }

 private:
  const MemoryPool::Peer p_ = kServer;
  std::unique_ptr<MemoryPool> mp_;
};

class ClientServerPolicy : public ::testing::Test {
 protected:
  void SetUp() {
    ROME_INIT_LOG();
    using CM = ConnectionManager<MemoryPool::channel_type>;
    server_mp_ =
        std::make_unique<MemoryPool>(server_, std::make_unique<CM>(server_.id));
    client_mp_ =
        std::make_unique<MemoryPool>(client_, std::make_unique<CM>(client_.id));
    // No loopback

    std::thread t([&]() {
      ASSERT_OK(this->server_mp_->Init(1ul << 12, {this->client_}));
    });
    ASSERT_OK(client_mp_->Init(1ul << 12, {server_}));
    t.join();
  }

  template <typename T>
  remote_ptr<T> AllocateClient(size_t s = 1) {
    return client_mp_->Allocate<T>(s);
  }

  template <typename T>
  remote_ptr<T> AllocateServer(size_t s = 1) {
    return server_mp_->Allocate<T>(s);
  }

  template <typename T>
  remote_ptr<T> Read(remote_ptr<T> ptr,
                     remote_ptr<T> prealloc = remote_nullptr) {
    return client_mp_->Read(ptr, prealloc);
  }

  template <typename T>
  remote_ptr<T> PartialRead(remote_ptr<T> ptr, size_t offset, size_t bytes,
                            remote_ptr<T> prealloc = remote_nullptr) {
    return client_mp_->PartialRead(ptr, offset, bytes, prealloc);
  }

  template <typename T>
  void Write(remote_ptr<T> ptr, const T& value,
             remote_ptr<T> prealloc = remote_nullptr) {
    return client_mp_->Write(ptr, value, prealloc);
  }

  template <typename T>
  T AtomicSwap(remote_ptr<T> ptr, uint64_t swap, uint64_t hint = 0) {
    return client_mp_->AtomicSwap(ptr, swap, hint);
  }

  template <typename T>
  T CompareAndSwap(remote_ptr<T> ptr, uint64_t expected, uint64_t swap) {
    return client_mp_->CompareAndSwap(ptr, expected, swap);
  }

  MemoryPool::DoorbellBatchBuilder CreateDoorbellBatchBuilder(int num_ops) {
    return MemoryPool::DoorbellBatchBuilder(client_mp_.get(), server_.id,
                                            num_ops);
  }

  void Execute(MemoryPool::DoorbellBatch* batch) {
    return client_mp_->Execute(batch);
  }

 private:
  const MemoryPool::Peer server_ = kServer;
  const MemoryPool::Peer client_ = kClient;
  std::unique_ptr<MemoryPool> server_mp_;
  std::unique_ptr<MemoryPool> client_mp_;
};

template <typename Policy>
class MemoryPoolTestFixture : public Policy {
 protected:
  MemoryPoolTestFixture() : Policy() {}
};

using TestTypes = ::testing::Types<LoopbackPolicy, ClientServerPolicy>;
TYPED_TEST_SUITE(MemoryPoolTestFixture, TestTypes);

TYPED_TEST(MemoryPoolTestFixture, InitTest) {
  // Test plan: Do nothing to ensure that setup is done correctly.
}

TYPED_TEST(MemoryPoolTestFixture, WriteTest) {
  // Test plan: Allocate some memory on the server then write to it.
  const int kValue = 42;
  auto target = TestFixture::template AllocateServer<int>();
  *target = -1;
  TestFixture::Write(target, kValue);
  EXPECT_EQ(*target, kValue);

  *target = -1;
  auto dest = TestFixture::template AllocateClient<int>();
  TestFixture::Write(target, kValue, dest);
  EXPECT_EQ(*target, kValue);
}

TYPED_TEST(MemoryPoolTestFixture, ReadTest) {
  // Test plan: Allocate some memory to write to then write to it.
  const int kValue = 42;
  auto target = TestFixture::template AllocateServer<int>();
  *target = kValue;
  auto result = TestFixture::Read(target);
  EXPECT_EQ(*result, kValue);

  auto dest = TestFixture::template AllocateClient<int>();
  result = TestFixture::Read(target, dest);
  EXPECT_EQ(*result, kValue);
  EXPECT_EQ(result, dest);
}

TYPED_TEST(MemoryPoolTestFixture, AtomicSwapTest) {
  // Test plan: Allocate some memory to write to then write to it.
  auto target = TestFixture::template AllocateServer<int64_t>();
  for (uint64_t value = 0; value < 1000; ++value) {
    *target = value;
    EXPECT_EQ(TestFixture::AtomicSwap(target, -1), value);
  }
}

TYPED_TEST(MemoryPoolTestFixture, CompareAndSwapTest) {
  // Test plan: Allocate some memory to write to then write to it.
  auto target = TestFixture::template AllocateServer<int64_t>();
  for (uint64_t value = 0; value < 1000; ++value) {
    *target = value;
    auto expected = (value / 2) * 2;  // Fails every other attempt
    EXPECT_EQ(TestFixture::CompareAndSwap(target, expected, 0), value);
  }
}

template <typename Policy, size_t S>
class PartialReadConfig : public Policy {
 private:
  static constexpr size_t kStructSize = 256;

 public:
  static constexpr size_t kReadSize = S;
  PartialReadConfig() : Policy() {}

  struct TestStruct {
    char buffer[kStructSize];
  };
};

template <typename Config>
class PartialReadTestFixture : public Config {
 protected:
  using config = Config;
  PartialReadTestFixture() : Config() {}
};

using PartialReadTestTypes =
    ::testing::Types<PartialReadConfig<LoopbackPolicy, 8>,
                     PartialReadConfig<LoopbackPolicy, 16>,
                     PartialReadConfig<LoopbackPolicy, 32>,
                     PartialReadConfig<LoopbackPolicy, 64>,
                     PartialReadConfig<LoopbackPolicy, 128>,
                     PartialReadConfig<LoopbackPolicy, 192>,
                     PartialReadConfig<ClientServerPolicy, 8>,
                     PartialReadConfig<ClientServerPolicy, 16>,
                     PartialReadConfig<ClientServerPolicy, 32>,
                     PartialReadConfig<ClientServerPolicy, 64>,
                     PartialReadConfig<ClientServerPolicy, 128>,
                     PartialReadConfig<ClientServerPolicy, 192>>;
TYPED_TEST_SUITE(PartialReadTestFixture, PartialReadTestTypes);

TYPED_TEST(PartialReadTestFixture, PartialReadTest) {
  // Test plan: Given a fixed size struct, fill the local copy with known bytes
  // then perform a partial read. Ensure that all expected bytes from the remote
  // object contain the expected value.
  using type = typename TestFixture::config::TestStruct;
  constexpr size_t size = sizeof(type);
  auto target = TestFixture::template AllocateServer<type>();
  auto target_addr = std::to_address(target);

  constexpr char kTargetByte = -1;
  std::memset(std::to_address(target), kTargetByte, size);

  auto local = TestFixture::template AllocateClient<type>();
  auto local_addr = std::to_address(local);
  constexpr char kLocalByte = 0;
  for (size_t offset = 0; offset < size - TestFixture::kReadSize; offset += 8) {
    std::memset(std::to_address(local), kLocalByte, size);

    TestFixture::PartialRead(target, offset, TestFixture::kReadSize, local);
    auto local_offset_addr = reinterpret_cast<char*>(local_addr) + offset;
    auto target_offset_addr = reinterpret_cast<char*>(target_addr) + offset;
    EXPECT_EQ(std::memcmp(local_offset_addr, target_offset_addr,
                          TestFixture::kReadSize),
              0);

    char expected[size];
    std::memset(expected, kLocalByte, size);
    if (offset != 0) {
      EXPECT_EQ(
          std::memcmp(reinterpret_cast<char*>(local_addr), expected, offset),
          0);
    }
    const size_t remainder_size = (size - offset) - TestFixture::kReadSize;
    EXPECT_EQ(std::memcmp(reinterpret_cast<char*>(local_offset_addr) +
                              TestFixture::kReadSize,
                          expected, remainder_size),
              0);
  }
}

template <typename Policy>
class DoorbellBatchTestConfig : public Policy {
 public:
  DoorbellBatchTestConfig() : Policy() {}
};

template <typename Config>
class DoorbellBatchTestFixture : public Config {
 protected:
  using config = Config;
  DoorbellBatchTestFixture() : Config() {}
};

using DoorbellBatchTestTypes =
    ::testing::Types<DoorbellBatchTestConfig<LoopbackPolicy>,
                     DoorbellBatchTestConfig<ClientServerPolicy>>;
TYPED_TEST_SUITE(DoorbellBatchTestFixture, DoorbellBatchTestTypes);

TYPED_TEST(DoorbellBatchTestFixture, SingleReadTest) {
  const uint64_t kValue = 0xf0f0f0f0f0f0f0f0;
  auto src = TestFixture::template AllocateServer<uint64_t>();
  *(std::to_address(src)) = kValue;
  ASSERT_EQ(*(std::to_address(src)), kValue);

  auto builder = TestFixture::CreateDoorbellBatchBuilder(1);
  auto dest = builder.AddRead(src);
  auto batch = builder.Build();
  TestFixture::Execute(batch.get());

  EXPECT_EQ(*(std::to_address(dest)), kValue);
}

TYPED_TEST(DoorbellBatchTestFixture, SingleWriteTest) {
  const uint64_t kValue = 0xf0f0f0f0f0f0f0f0;
  auto dest = TestFixture::template AllocateServer<uint64_t>();
  *(std::to_address(dest)) = 0;
  ASSERT_EQ(*(std::to_address(dest)), 0);

  auto builder = TestFixture::CreateDoorbellBatchBuilder(1);
  builder.AddWrite(dest, kValue);
  auto batch = builder.Build();
  TestFixture::Execute(batch.get());

  EXPECT_EQ(*(std::to_address(dest)), kValue);
}

TYPED_TEST(DoorbellBatchTestFixture, WriteThenReadTest) {
  const uint64_t kValue = 0xf0f0f0f0f0f0f0f0;
  auto dest = TestFixture::template AllocateServer<uint64_t>();
  *(std::to_address(dest)) = 0;
  ASSERT_EQ(*(std::to_address(dest)), 0);

  auto builder = TestFixture::CreateDoorbellBatchBuilder(2);
  builder.AddWrite(dest, kValue);
  auto read = builder.AddRead(dest);
  *(std::to_address(read)) = 0;
  auto batch = builder.Build();
  TestFixture::Execute(batch.get());

  EXPECT_EQ(*(std::to_address(dest)), kValue);
  EXPECT_EQ(*(std::to_address(read)), kValue);
}

TYPED_TEST(DoorbellBatchTestFixture, ReuseTest) {
  const uint64_t kValue = 0xf0f0f0f0f0f0f0f0;
  auto dest = TestFixture::template AllocateServer<uint64_t>();
  *dest = 0;
  ASSERT_EQ(*(std::to_address(dest)), 0);

  auto builder = TestFixture::CreateDoorbellBatchBuilder(2);
  auto src = TestFixture::template AllocateClient<uint64_t>();
  *src = kValue;
  builder.AddWrite(dest, src);
  auto read = builder.AddRead(dest);
  *read = 0;
  auto batch = builder.Build();
  TestFixture::Execute(batch.get());

  EXPECT_EQ(*(std::to_address(dest)), kValue);
  EXPECT_EQ(*(std::to_address(read)), kValue);

  *src = kValue + 1;
  TestFixture::Execute(batch.get());
  EXPECT_EQ(*dest, kValue + 1);
  EXPECT_EQ(*read, kValue + 1);
}

}  // namespace
}  // namespace rome::rdma