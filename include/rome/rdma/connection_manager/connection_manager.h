#pragma once

#include <infiniband/verbs.h>
#include <rdma/rdma_cma.h>

#include <chrono>
#include <limits>
#include <memory>
#include <optional>
#include <string_view>
#include <thread>
#include <unordered_set>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "connection.h"
#include "rome/rdma/channel/rdma_accessor.h"
#include "rome/rdma/channel/rdma_channel.h"
#include "rome/rdma/channel/twosided_messenger.h"
#include "rome/rdma/rdma_broker.h"
#include "rome/rdma/rdma_device.h"
#include "rome/rdma/rdma_memory.h"
#include "rome/rdma/rdma_receiver.h"
#include "rome/util/coroutine.h"

namespace rome::rdma {

template <typename ChannelType>
class ConnectionManager : public RdmaReceiverInterface {
 public:
  typedef Connection<ChannelType> conn_type;

  ~ConnectionManager();
  explicit ConnectionManager(uint32_t my_id);

  absl::Status Start(std::string_view addr, std::optional<uint16_t> port);

  // Getters.
  std::string address() const { return broker_->address(); }
  uint16_t port() const { return broker_->port(); }
  ibv_pd* pd() const { return broker_->pd(); }

  int GetNumConnections() {
    Acquire(my_id_);
    int size = established_.size();
    Release();
    return size;
  }

  // `RdmaReceiverInterface` implementaiton
  void OnConnectRequest(rdma_cm_id* id, rdma_cm_event* event) override;
  void OnEstablished(rdma_cm_id* id, rdma_cm_event* event) override;
  void OnDisconnect(rdma_cm_id* id) override;

  // `RdmaClientInterface` implementation
  absl::StatusOr<conn_type*> Connect(uint32_t node_id, std::string_view server,
                                     uint16_t port);

  absl::StatusOr<conn_type*> GetConnection(uint32_t node_id);

  void Shutdown();

 private:
  // The size of each memory region dedicated to a single connection.
  static constexpr int kCapacity = 1 << 12;  // 4 KiB
  static constexpr int kMaxRecvBytes = 64;

  static constexpr int kMaxWr = kCapacity / kMaxRecvBytes;
  static constexpr int kMaxSge = 1;
  static constexpr int kMaxInlineData = 0;

  static constexpr char kPdId[] = "ConnectionManager";

  static constexpr int kUnlocked = -1;

  static constexpr uint32_t kMinBackoffUs = 100;
  static constexpr uint32_t kMaxBackoffUs = 5000000;

  // Each `rdma_cm_id` can be associated with some context, which is represented
  // by `IdContext`. `node_id` is the numerical identifier for the peer node of
  // the connection and `conn_param` is used to provide private data during the
  // connection set up to send the local node identifier upon connection setup.
  struct IdContext {
    uint32_t node_id;
    rdma_conn_param conn_param;
    ChannelType* channel;

    static inline uint32_t GetNodeId(void* ctx) {
      return reinterpret_cast<IdContext*>(ctx)->node_id;
    }

    static inline ChannelType* GetRdmaChannel(void* ctx) {
      return reinterpret_cast<IdContext*>(ctx)->channel;
    }
  };

  // Lock acquisition will spin until either the lock is acquired successfully
  // or the locker is an outgoing connection request from this node.
  inline bool Acquire(int peer_id) {
    for (int expected = kUnlocked;
         !mu_.compare_exchange_weak(expected, peer_id); expected = kUnlocked) {
      if (expected == my_id_) {
        ROME_DEBUG(
            "[Acquire] (Node {}) Giving up lock acquisition: actual={}, "
            "swap={}",
            my_id_, expected, peer_id);
        return false;
      }
    }
    return true;
  }

  inline void Release() { mu_ = kUnlocked; }

  constexpr ibv_qp_init_attr DefaultQpInitAttr() {
    ibv_qp_init_attr init_attr;
    std::memset(&init_attr, 0, sizeof(init_attr));
    init_attr.cap.max_send_wr = init_attr.cap.max_recv_wr = kMaxWr;
    init_attr.cap.max_send_sge = init_attr.cap.max_recv_sge = kMaxSge;
    init_attr.cap.max_inline_data = kMaxInlineData;
    init_attr.sq_sig_all = 0;  // Must request completions.
    init_attr.qp_type = IBV_QPT_RC;
    return init_attr;
  }

  constexpr ibv_qp_attr DefaultQpAttr() {
    ibv_qp_attr attr;
    std::memset(&attr, 0, sizeof(attr));
    attr.qp_access_flags = IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ |
                           IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_ATOMIC;
    attr.max_dest_rd_atomic = 8;
    attr.path_mtu = IBV_MTU_4096;
    attr.min_rnr_timer = 12;
    attr.rq_psn = 0;
    attr.sq_psn = 0;
    attr.timeout = 12;
    attr.retry_cnt = 7;
    attr.rnr_retry = 1;
    attr.max_rd_atomic = 8;
    return attr;
  }

  absl::StatusOr<conn_type*> ConnectLoopback(rdma_cm_id* id);

  // Whether or not to stop handling requests.
  volatile bool accepting_;

  // Current status
  absl::Status status_;

  uint32_t my_id_;
  std::unique_ptr<RdmaBroker> broker_;
  ibv_pd* pd_;  // Convenience ptr to protection domain of `broker_`

  // Maintains connection information for a given Internet address. A connection
  // manager only maintains a single connection per node. Nodes are identified
  // by a string representing their IP address.
  std::atomic<int> mu_;
  std::unordered_map<uint32_t, std::unique_ptr<conn_type>> requested_;
  std::unordered_map<uint32_t, std::unique_ptr<conn_type>> established_;

  uint32_t backoff_us_{0};

  rdma_cm_id* loopback_id_ = nullptr;
};

}  // namespace rome::rdma

#include "connection_manager_impl.h"