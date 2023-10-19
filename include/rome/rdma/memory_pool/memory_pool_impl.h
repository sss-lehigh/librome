#pragma once

#include <asm-generic/errno-base.h>
#include <infiniband/verbs.h>

#include <cmath>
#include <cstddef>
#include <memory>

#include "memory_pool.h"
#include "rome/util/thread_util.h"

namespace rome::rdma {

template <typename T>
remote_ptr<T> MemoryPool::DoorbellBatchBuilder::AddRead(
    remote_ptr<T> rptr, bool fence, remote_ptr<T> prealloc) {
  auto local = (prealloc == remote_nullptr) ? pool_->Allocate<T>() : prealloc;
  AddReadInternal(rptr, 0, sizeof(T), sizeof(T), fence, local);
  return local;
}

template <typename T>
remote_ptr<T> MemoryPool::DoorbellBatchBuilder::AddPartialRead(
    remote_ptr<T> rptr, size_t offset, size_t bytes, bool fence,
    remote_ptr<T> prealloc) {
  auto local = (prealloc == remote_nullptr) ? pool_->Allocate<T>() : prealloc;
  AddReadInternal(rptr, offset, bytes, bytes, fence, local);
  return local;
}

template <typename T>
void MemoryPool::DoorbellBatchBuilder::AddReadInternal(remote_ptr<T> rptr,
                                                       size_t offset,
                                                       size_t bytes,
                                                       size_t chunk, bool fence,
                                                       remote_ptr<T> prealloc) {
  const int num_chunks = bytes % chunk ? (bytes / chunk) + 1 : bytes / chunk;
  const size_t remainder = bytes % chunk;
  const bool is_multiple = remainder == 0;

  T *local = std::to_address(prealloc);
  for (int i = 0; i < num_chunks; ++i) {
    auto wr_sge = batch_->Add();
    auto wr = wr_sge.first;
    auto sge = wr_sge.second;

    auto chunk_offset = offset + i * chunk;
    sge->addr = reinterpret_cast<uint64_t>(local) + chunk_offset;
    if (is_multiple) {
      sge->length = chunk;
    } else {
      sge->length = (i == num_chunks - 1 ? remainder : chunk);
    }
    sge->lkey = batch_->conn_info().lkey;

    wr->opcode = IBV_WR_RDMA_READ;
    if (fence) wr->send_flags |= IBV_SEND_FENCE;
    wr->wr.rdma.remote_addr = rptr.address() + chunk_offset;
    wr->wr.rdma.rkey = batch_->conn_info().rkey;
  }
}

template <typename T>
void MemoryPool::DoorbellBatchBuilder::AddWrite(remote_ptr<T> rptr,
                                                const T &value, bool fence) {
  auto local = pool_->Allocate<T>();
  std::memcpy(std::to_address(local), &value, sizeof(value));
  AddWrite(rptr, local, fence);
}

template <typename T>
void MemoryPool::DoorbellBatchBuilder::AddWrite(remote_ptr<T> rptr,
                                                remote_ptr<T> prealloc,
                                                bool fence) {
  auto wr_sge = batch_->Add();
  ibv_send_wr *wr = wr_sge.first;
  ibv_sge *sge = wr_sge.second;

  sge->addr = (uint64_t)std::to_address(prealloc);
  sge->length = sizeof(T);
  sge->lkey = batch_->conn_info().lkey;

  wr->opcode = IBV_WR_RDMA_WRITE;
  if (fence) wr->send_flags |= IBV_SEND_FENCE;
  wr->wr.rdma.remote_addr = (uint64_t)std::to_address(rptr);
  wr->wr.rdma.rkey = batch_->conn_info().rkey;
}

template <typename T>
void MemoryPool::DoorbellBatchBuilder::AddWriteBytes(remote_ptr<T> rptr,
                                                     remote_ptr<T> prealloc,
                                                     int bytes, bool fence) {
  auto wr_sge = batch_->Add();
  ibv_send_wr *wr = wr_sge.first;
  ibv_sge *sge = wr_sge.second;

  sge->addr = (uint64_t)std::to_address(prealloc);
  sge->length = bytes;
  sge->lkey = batch_->conn_info().lkey;

  wr->opcode = IBV_WR_RDMA_WRITE;
  if (fence) wr->send_flags |= IBV_SEND_FENCE;
  wr->wr.rdma.remote_addr = (uint64_t)std::to_address(rptr);
  wr->wr.rdma.rkey = batch_->conn_info().rkey;
}

inline std::unique_ptr<MemoryPool::DoorbellBatch>
MemoryPool::DoorbellBatchBuilder::Build() {
  const int size = batch_->size();
  const int capcity = batch_->capacity();
  ROME_ASSERT(size > 0, "Cannot build an empty batch.");
  ROME_ASSERT(size == capcity, "Batch must be full");
  for (int i = 0; i < size; ++i) {
    batch_->wrs_[i].wr_id = batch_->wrs_[i].wr.rdma.remote_addr;
  }
  return std::move(batch_);
}

MemoryPool::MemoryPool(
    const Peer &self,
    std::unique_ptr<ConnectionManager<channel_type>> connection_manager)
    : self_(self),
      connection_manager_(std::move(connection_manager)),
      rdma_per_read_("rdma_per_read", "ops", 10000) {}

absl::Status MemoryPool::Init(uint32_t capacity,
                              const std::vector<Peer> &peers) {      
  auto status = connection_manager_->Start(self_.address, self_.port);
  ROME_CHECK_OK(ROME_RETURN(status), status);
  rdma_memory_ = std::make_unique<rdma_memory_resource>(
      capacity + sizeof(uint64_t), connection_manager_->pd());
  mr_ = rdma_memory_->mr();

  auto alloc = rdma_allocator<uint64_t>(rdma_memory_.get());
  prev_ = alloc.allocate();

  for (const auto &p : peers) {
    auto connected = connection_manager_->Connect(p.id, p.address, p.port);
    while (absl::IsUnavailable(connected.status())) {
      connected = connection_manager_->Connect(p.id, p.address, p.port);
    } 
    ROME_CHECK_OK(ROME_RETURN(connected.status()), connected);
  }

  RemoteObjectProto rm_proto;
  rm_proto.set_rkey(mr_->rkey);
  rm_proto.set_raddr(reinterpret_cast<uint64_t>(mr_->addr));
  for (const auto &p : peers) {
    auto conn = VALUE_OR_DIE(connection_manager_->GetConnection(p.id));
    status = conn->channel()->Send(rm_proto);
    ROME_CHECK_OK(ROME_RETURN(status), status);
  }

  for (const auto &p : peers) {
    auto conn = VALUE_OR_DIE(connection_manager_->GetConnection(p.id));
    auto got = conn->channel()->TryDeliver<RemoteObjectProto>();
    while (!got.ok() && got.status().code() == absl::StatusCode::kUnavailable) {
      got = conn->channel()->TryDeliver<RemoteObjectProto>();
    }
    ROME_CHECK_OK(ROME_RETURN(got.status()), got);
    conn_info_.emplace(p.id, conn_info_t{conn, got->rkey(), mr_->lkey});
  }
  return absl::OkStatus();
}

template <typename T>
remote_ptr<T> MemoryPool::Allocate(size_t size) {
  return remote_ptr<T>(self_.id,
                       rdma_allocator<T>(rdma_memory_.get()).allocate(size));
}

template <typename T>
void MemoryPool::Deallocate(remote_ptr<T> p, size_t size) {
  ROME_ASSERT(p.id() == self_.id,
              "Alloc/dealloc on remote node not implemented...");
  rdma_allocator<T>(rdma_memory_.get()).deallocate(std::to_address(p), size);
}

inline void MemoryPool::Execute(DoorbellBatch *batch) {
  ibv_send_wr *bad;
  auto *conn = batch->conn_info().conn;
  RDMA_CM_ASSERT(ibv_post_send, conn->id()->qp, batch->wrs_, &bad);

  int poll;
  ibv_wc wc;
  while ((poll = ibv_poll_cq(conn->id()->send_cq, 1, &wc)) == 0) {
    if (batch->is_mortal() && *kill) return;
    cpu_relax();
  }
  ROME_ASSERT(poll == 1 && wc.status == IBV_WC_SUCCESS,
              "ibv_poll_cq(): {} (dest={})",
              (poll < 0 ? strerror(errno) : ibv_wc_status_str(wc.status)),
              remote_ptr<uint8_t>(wc.wr_id));
}

template <typename T>
remote_ptr<T> MemoryPool::Read(remote_ptr<T> ptr, remote_ptr<T> prealloc,
                               std::atomic<bool> *kill) {
  if (prealloc == remote_nullptr) prealloc = Allocate<T>();
  ReadInternal(ptr, 0, sizeof(T), sizeof(T), prealloc, kill);
  return prealloc;
}

template <typename T>
remote_ptr<T> MemoryPool::PartialRead(remote_ptr<T> ptr, size_t offset,
                                      size_t bytes, remote_ptr<T> prealloc) {
  if (prealloc == remote_nullptr) prealloc = Allocate<T>();
  ReadInternal(ptr, offset, bytes, sizeof(T), prealloc);
  return prealloc;
}

template <typename T>
void MemoryPool::ReadInternal(remote_ptr<T> ptr, size_t offset, size_t bytes,
                              size_t chunk_size, remote_ptr<T> prealloc,
                              std::atomic<bool> *kill) {
  const int num_chunks =
      bytes % chunk_size ? (bytes / chunk_size) + 1 : bytes / chunk_size;
  const size_t remainder = bytes % chunk_size;
  const bool is_multiple = remainder == 0;

  auto info = conn_info_.at(ptr.id());

  T *local = std::to_address(prealloc);
  ibv_sge sges[num_chunks];
  ibv_send_wr wrs[num_chunks];
  for (int i = 0; i < num_chunks; ++i) {
    auto chunk_offset = offset + i * chunk_size;
    sges[i].addr = reinterpret_cast<uint64_t>(local) + chunk_offset;
    if (is_multiple) {
      sges[i].length = chunk_size;
    } else {
      sges[i].length = (i == num_chunks - 1 ? remainder : chunk_size);
    }
    sges[i].lkey = mr_->lkey;

    wrs[i].num_sge = 1;
    wrs[i].sg_list = &sges[i];
    wrs[i].opcode = IBV_WR_RDMA_READ;
    wrs[i].send_flags = IBV_SEND_FENCE;
    if (i == num_chunks - 1) wrs[i].send_flags |= IBV_SEND_SIGNALED;
    wrs[i].wr.rdma.remote_addr = ptr.address() + chunk_offset;
    wrs[i].wr.rdma.rkey = info.rkey;
    wrs[i].next = (i != num_chunks - 1 ? &wrs[i + 1] : nullptr);
  }

  ibv_send_wr *bad;
  RDMA_CM_ASSERT(ibv_post_send, info.conn->id()->qp, wrs, &bad);
  ibv_wc wc;
  int poll = 0;
  if (kill == nullptr) {
    for (; poll == 0; poll = ibv_poll_cq(info.conn->id()->send_cq, 1, &wc))
      ;
    ROME_ASSERT(
        poll == 1 && wc.status == IBV_WC_SUCCESS, "ibv_poll_cq(): {} @ {}",
        (poll < 0 ? strerror(errno) : ibv_wc_status_str(wc.status)), ptr);
  } else {
    for (; poll == 0 && !(*kill);
         poll = ibv_poll_cq(info.conn->id()->send_cq, 1, &wc))
      ;
    if (!(*kill) && (poll != 1 || wc.status != IBV_WC_SUCCESS)) {
      ROME_FATAL("ibv_poll_cq(): {}",
                 (poll < 0 ? strerror(errno) : ibv_wc_status_str(wc.status)));
    }
  }
  rdma_per_read_ << num_chunks;
}

template <typename T>
void MemoryPool::Write(remote_ptr<T> ptr, const T &val,
                       remote_ptr<T> prealloc) {
  ROME_DEBUG("Write: {:x} @ {}", (uint64_t)val, ptr);
  auto info = conn_info_.at(ptr.id());

  T *local;
  if (prealloc == remote_nullptr) {
    auto alloc = rdma_allocator<T>(rdma_memory_.get());
    local = alloc.allocate();
    ROME_DEBUG("Allocated memory for Write: {} bytes @ 0x{:x}", sizeof(T),
               (uint64_t)local);
  } else {
    local = std::to_address(prealloc);
    ROME_DEBUG("Preallocated memory for Write: {} bytes @ 0x{:x}", sizeof(T),
               (uint64_t)local);
  }

  ROME_ASSERT((uint64_t)local != ptr.address(), "WTF");
  std::memset(local, 0, sizeof(T));
  *local = val;
  ibv_sge sge{};
  sge.addr = reinterpret_cast<uint64_t>(local);
  sge.length = sizeof(T);
  sge.lkey = mr_->lkey;

  send_wr_.num_sge = 1;
  send_wr_.sg_list = &sge;
  send_wr_.opcode = IBV_WR_RDMA_WRITE;
  send_wr_.send_flags = IBV_SEND_SIGNALED | IBV_SEND_FENCE;
  send_wr_.wr.rdma.remote_addr = ptr.address();
  send_wr_.wr.rdma.rkey = info.rkey;

  ibv_send_wr *bad = nullptr;
  RDMA_CM_ASSERT(ibv_post_send, info.conn->id()->qp, &send_wr_, &bad);
  ibv_wc wc;
  auto poll = ibv_poll_cq(info.conn->id()->send_cq, 1, &wc);
  while (poll == 0 || (poll < 0 && errno == EAGAIN)) {
    poll = ibv_poll_cq(info.conn->id()->send_cq, 1, &wc);
  }

  if (prealloc == remote_nullptr) {
    auto alloc = rdma_allocator<T>(rdma_memory_.get());
    alloc.deallocate(local);
  }
  ROME_ASSERT(poll == 1 && wc.status == IBV_WC_SUCCESS,
              "ibv_poll_cq(): {} ({})",
              (poll < 0 ? strerror(errno) : ibv_wc_status_str(wc.status)),
              (std::stringstream() << ptr).str());
}

template <typename T>
T MemoryPool::AtomicSwap(remote_ptr<T> ptr, uint64_t swap, uint64_t hint) {
  static_assert(sizeof(T) == 8);
  auto info = conn_info_.at(ptr.id());

  ibv_sge sge{};
  sge.addr = reinterpret_cast<uint64_t>(prev_);
  sge.length = sizeof(uint64_t);
  sge.lkey = mr_->lkey;

  send_wr_.num_sge = 1;
  send_wr_.sg_list = &sge;
  send_wr_.opcode = IBV_WR_ATOMIC_CMP_AND_SWP;
  send_wr_.send_flags = IBV_SEND_SIGNALED | IBV_SEND_FENCE;
  send_wr_.wr.atomic.remote_addr = ptr.address();
  send_wr_.wr.atomic.rkey = info.rkey;
  send_wr_.wr.atomic.compare_add = hint;
  send_wr_.wr.atomic.swap = swap;

  ibv_send_wr *bad = nullptr;
  while (true) {
    RDMA_CM_ASSERT(ibv_post_send, info.conn->id()->qp, &send_wr_, &bad);
    ibv_wc wc;
    auto poll = ibv_poll_cq(info.conn->id()->send_cq, 1, &wc);
    while (poll == 0 || (poll < 0 && errno == EAGAIN)) {
      poll = ibv_poll_cq(info.conn->id()->send_cq, 1, &wc);
    }
    ROME_ASSERT(poll == 1 && wc.status == IBV_WC_SUCCESS, "ibv_poll_cq(): {}",
                (poll < 0 ? strerror(errno) : ibv_wc_status_str(wc.status)));

    ROME_DEBUG("Swap: expected={:x}, swap={:x}, prev={:x} (id={})",
               send_wr_.wr.atomic.compare_add, (uint64_t)swap, *prev_,
               self_.id);
    if (*prev_ == send_wr_.wr.atomic.compare_add) break;
    send_wr_.wr.atomic.compare_add = *prev_;
  };
  return T(*prev_);
}

template <typename T>
T MemoryPool::CompareAndSwap(remote_ptr<T> ptr, uint64_t expected,
                             uint64_t swap) {
  static_assert(sizeof(T) == 8);
  auto info = conn_info_.at(ptr.id());
  
  ibv_sge sge{};
  sge.addr = reinterpret_cast<uint64_t>(prev_);
  sge.length = sizeof(uint64_t);
  sge.lkey = mr_->lkey;

  send_wr_.num_sge = 1;
  send_wr_.sg_list = &sge;
  send_wr_.opcode = IBV_WR_ATOMIC_CMP_AND_SWP;
  send_wr_.send_flags = IBV_SEND_SIGNALED | IBV_SEND_FENCE;
  send_wr_.wr.atomic.remote_addr = ptr.address();
  send_wr_.wr.atomic.rkey = info.rkey;
  send_wr_.wr.atomic.compare_add = expected;
  send_wr_.wr.atomic.swap = swap;

  ibv_send_wr *bad = nullptr;
  RDMA_CM_ASSERT(ibv_post_send, info.conn->id()->qp, &send_wr_, &bad);
  ibv_wc wc;
  auto poll = ibv_poll_cq(info.conn->id()->send_cq, 1, &wc);
  while (poll == 0 || (poll < 0 && errno == EAGAIN)) {
    poll = ibv_poll_cq(info.conn->id()->send_cq, 1, &wc);
  }
  ROME_ASSERT(poll == 1 && wc.status == IBV_WC_SUCCESS, "ibv_poll_cq(): {}",
              (poll < 0 ? strerror(errno) : ibv_wc_status_str(wc.status)));
  ROME_DEBUG("CompareAndSwap: expected={:x}, swap={:x}, actual={:x}  (id={})",
             expected, swap, *prev_, static_cast<uint64_t>(self_.id));
  return T(*prev_);
}

}  // namespace rome::rdma