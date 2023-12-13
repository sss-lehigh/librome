#include <arpa/inet.h>
#include <asm-generic/errno.h>
#include <fcntl.h>
#include <infiniband/verbs.h>
#include <netdb.h>
#include <netinet/in.h>
#include <rdma/rdma_cma.h>
#include <rdma/rdma_verbs.h>
#include <sys/socket.h>

#include <atomic>
#include <chrono>
#include <experimental/coroutine>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <thread>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "connection_manager.h"
#include "rome/rdma/channel/rdma_channel.h"
#include "rome/rdma/rdma_util.h"
#include "rome/util/coroutine.h"
#include "rome/util/status_util.h"

#define LOOPBACK_PORT_NUM 0
namespace rome::rdma {

using ::util::InternalErrorBuilder;

template <typename ChannelType>
ConnectionManager<ChannelType>::~ConnectionManager() {
  ROME_DEBUG("Shutting down: {}", fmt::ptr(this));
  Acquire(my_id_);
  Shutdown();

  ROME_DEBUG("Stopping broker...");
  if (broker_ != nullptr) auto s = broker_->Stop();

  auto cleanup = [this](auto& iter) {
    // A loopback connection is made manually, so we do not need to deal with
    // the regular `rdma_cm` handling. Similarly, we avoid destroying the event
    // channel below since it is destroyed along with the id.
    auto id = iter.second->id();
    if (iter.first != my_id_) {
      rdma_disconnect(id);
      rdma_cm_event* event;
      auto result = rdma_get_cm_event(id->channel, &event);
      while (result == 0) {
        RDMA_CM_ASSERT(rdma_ack_cm_event, event);
        result = rdma_get_cm_event(id->channel, &event);
      }
    }

    // We only allocate contexts for connections that were created by the
    // `RdmaReceiver` callbacks. Otherwise, we created an event channel so
    // that we could asynchronously connect (and avoid distributed deadlock).
    auto* context = id->context;
    auto* channel = id->channel;
    rdma_destroy_ep(id);

    if (iter.first != my_id_ && context != nullptr) {
      free(context);
    } else if (iter.first != my_id_) {
      rdma_destroy_event_channel(channel);
    }
  };

  std::for_each(established_.begin(), established_.end(), cleanup);
  Release();
  ROME_DEBUG("Connection Manager Deconstructed.");
}

template <typename ChannelType>
void ConnectionManager<ChannelType>::Shutdown() {
  // Stop accepting new requests.
  accepting_ = false;
}

template <typename ChannelType>
ConnectionManager<ChannelType>::ConnectionManager(uint32_t my_id)
    : accepting_(false), my_id_(my_id), broker_(nullptr), mu_(-1) {}

template <typename ChannelType>
absl::Status ConnectionManager<ChannelType>::Start(
    std::string_view addr, std::optional<uint16_t> port) {
  if (accepting_) {
    return InternalErrorBuilder() << "Cannot start broker twice";
  }
  accepting_ = true;

  broker_ = RdmaBroker::Create(addr, port, this);
  ROME_CHECK_QUIET(
      ROME_RETURN(InternalErrorBuilder() << "Failed to create broker"),
      broker_ != nullptr)
  return absl::OkStatus();
}

namespace {

[[maybe_unused]] inline std::string GetDestinationAsString(rdma_cm_id* id) {
  char addr_str[INET_ADDRSTRLEN];
  ROME_ASSERT(inet_ntop(AF_INET, &(id->route.addr.dst_sin.sin_addr), addr_str,
                        INET_ADDRSTRLEN) != nullptr,
              "inet_ntop(): {}", strerror(errno));
  std::stringstream ss;
  ss << addr_str << ":" << rdma_get_dst_port(id);
  return ss.str();
}

}  // namespace

template <typename ChannelType>
void ConnectionManager<ChannelType>::OnConnectRequest(rdma_cm_id* id,
                                                      rdma_cm_event* event) {
  if (!accepting_) return;

  // The private data is used to understand from what node the connection
  // request is coming from.
  ROME_ASSERT_DEBUG(event->param.conn.private_data != nullptr,
                    "Received connect request without private data.");
  uint32_t peer_id =
      *reinterpret_cast<const uint32_t*>(event->param.conn.private_data);
  ROME_DEBUG("[OnConnectRequest] (Node {}) Got connection request from: {}",
             my_id_, peer_id);

  if (peer_id != my_id_) {
    // Attempt to acquire lock when not originating from same node
    if (!Acquire(peer_id)) {
      ROME_DEBUG("Lock acquisition failed: {}", mu_);
      rdma_reject(event->id, nullptr, 0);
      rdma_destroy_ep(id);
      rdma_ack_cm_event(event);
      return;
    }

    // Check if the connection has already been established.
    if (auto conn = established_.find(peer_id);
        conn != established_.end() || requested_.contains(peer_id)) {
      rdma_reject(event->id, nullptr, 0);
      rdma_destroy_ep(id);
      rdma_ack_cm_event(event);
      if (peer_id != my_id_) Release();
      auto status =
          util::AlreadyExistsErrorBuilder()
          << "[OnConnectRequest] (Node " << my_id_ << ") Connection already "
          << (conn != established_.end() ? "established" : "requested") << ": "
          << peer_id;
      ROME_DEBUG(absl::Status(status).ToString());
      return;
    }

    // Create a new QP for the connection.
    ibv_qp_init_attr init_attr = DefaultQpInitAttr();
    ROME_ASSERT(id->qp == nullptr, "QP already allocated...?");
    RDMA_CM_ASSERT(rdma_create_qp, id, pd(), &init_attr);
  } else {
    // rdma_destroy_id(id);
    id = loopback_id_;
  }

  // Prepare the necessary resources for this connection. Includes a
  // `RdmaChannel` that holds the QP and memory for 2-sided communication.
  // The underlying QP is RC, so we reuse it for issuing 1-sided RDMA too. We
  // also store the `peer_id` associated with this id so that we can reference
  // it later.
  auto context = new IdContext{peer_id, {}, {}};
  std::memset(&context->conn_param, 0, sizeof(context->conn_param));
  context->conn_param.private_data = &context->node_id;
  context->conn_param.private_data_len = sizeof(context->node_id);
  context->conn_param.rnr_retry_count = 1;  // Retry forever
  context->conn_param.retry_count = 7;
  context->conn_param.responder_resources = 8;
  context->conn_param.initiator_depth = 8;
  id->context = context;

  auto iter = established_.emplace(
      peer_id,
      new Connection{my_id_, peer_id, std::make_unique<ChannelType>(id)});
  ROME_ASSERT_DEBUG(iter.second, "Insertion failed");

  ROME_DEBUG("[OnConnectRequest] (Node {}) peer={}, id={}", my_id_, peer_id,
             fmt::ptr(id));
  RDMA_CM_ASSERT(rdma_accept, id,
                 peer_id == my_id_ ? nullptr : &context->conn_param);
  rdma_ack_cm_event(event);
  if (peer_id != my_id_) Release();
}

template <typename ChannelType>
void ConnectionManager<ChannelType>::OnEstablished(rdma_cm_id* id,
                                                   rdma_cm_event* event) {
  rdma_ack_cm_event(event);
}

template <typename ChannelType>
void ConnectionManager<ChannelType>::OnDisconnect(rdma_cm_id* id) {
  // This disconnection originated from the peer, so we simply disconnect the
  // local endpoint and clean it up.
  //
  // NOTE: The event is already ack'ed by the caller.
  rdma_disconnect(id);

  uint32_t peer_id = IdContext::GetNodeId(id->context);
  Acquire(peer_id);
  if (auto conn = established_.find(peer_id);
      conn != established_.end() && conn->second->id() == id) {
    ROME_DEBUG("(Node {}) Disconnected from node {}", my_id_, peer_id);
    established_.erase(peer_id);
  }
  Release();
  auto* event_channel = id->channel;
  rdma_destroy_ep(id);
  rdma_destroy_event_channel(event_channel);
}

template <typename ChannelType>
absl::StatusOr<typename ConnectionManager<ChannelType>::conn_type*>
ConnectionManager<ChannelType>::ConnectLoopback(rdma_cm_id* id) {
  ROME_ASSERT_DEBUG(id->qp != nullptr, "No QP associated with endpoint");
  ROME_DEBUG("Connecting loopback...");
  ibv_qp_attr attr;
  int attr_mask;

  attr = DefaultQpAttr();
  attr.qp_state = IBV_QPS_INIT;
  attr.port_num = LOOPBACK_PORT_NUM; // id->port_num;
  attr_mask =
      IBV_QP_STATE | IBV_QP_PKEY_INDEX | IBV_QP_PORT | IBV_QP_ACCESS_FLAGS;
  ROME_TRACE("Loopback: IBV_QPS_INIT");
  RDMA_CM_CHECK(ibv_modify_qp, id->qp, &attr, attr_mask);

  ibv_port_attr port_attr;
  RDMA_CM_CHECK(ibv_query_port, id->verbs, LOOPBACK_PORT_NUM, &port_attr); // RDMA_CM_CHECK(ibv_query_port, id->verbs, id->port_num, &port_attr);
  attr.ah_attr.dlid = port_attr.lid;
  attr.qp_state = IBV_QPS_RTR;
  attr.dest_qp_num = id->qp->qp_num;
  attr.ah_attr.port_num = LOOPBACK_PORT_NUM; // id->port_num;
  attr_mask =
      (IBV_QP_STATE | IBV_QP_AV | IBV_QP_PATH_MTU | IBV_QP_DEST_QPN |
       IBV_QP_RQ_PSN | IBV_QP_MAX_DEST_RD_ATOMIC | IBV_QP_MIN_RNR_TIMER);
  ROME_TRACE("Loopback: IBV_QPS_RTR");
  RDMA_CM_CHECK(ibv_modify_qp, id->qp, &attr, attr_mask);

  attr.qp_state = IBV_QPS_RTS;
  attr_mask = (IBV_QP_STATE | IBV_QP_SQ_PSN | IBV_QP_TIMEOUT |
               IBV_QP_RETRY_CNT | IBV_QP_RNR_RETRY | IBV_QP_MAX_QP_RD_ATOMIC);
  ROME_TRACE("Loopback: IBV_QPS_RTS");
  RDMA_CM_CHECK(ibv_modify_qp, id->qp, &attr, attr_mask);

  RDMA_CM_CHECK(fcntl, id->recv_cq->channel->fd, F_SETFL,
                fcntl(id->recv_cq->channel->fd, F_GETFL) | O_NONBLOCK);
  RDMA_CM_CHECK(fcntl, id->send_cq->channel->fd, F_SETFL,
                fcntl(id->send_cq->channel->fd, F_GETFL) | O_NONBLOCK);

  // Allocate a new control channel to be used with this connection
  auto channel = std::make_unique<ChannelType>(id);
  auto iter = established_.emplace(
      my_id_, new Connection{my_id_, my_id_, std::move(channel)});
  ROME_ASSERT(iter.second, "Unexepected error");
  Release();
  return established_[my_id_].get();
}

template <typename ChannelType>
absl::StatusOr<typename ConnectionManager<ChannelType>::conn_type*>
ConnectionManager<ChannelType>::Connect(uint32_t peer_id,
                                        std::string_view server,
                                        uint16_t port) {
  if (Acquire(my_id_)) {
    auto conn = established_.find(peer_id);
    if (conn != established_.end()) {
      Release();
      return conn->second.get();
    }

    auto port_str = std::to_string(htons(port));
    rdma_cm_id* id = nullptr;
    rdma_addrinfo hints, *resolved = nullptr;

    std::memset(&hints, 0, sizeof(hints));
    hints.ai_port_space = RDMA_PS_TCP;
    hints.ai_qp_type = IBV_QPT_RC;
    hints.ai_family = AF_IB;

    struct sockaddr_in src;
    std::memset(&src, 0, sizeof(src));
    src.sin_family = AF_INET;
    auto src_addr_str = broker_->address();
    inet_aton(src_addr_str.data(), &src.sin_addr);

    hints.ai_src_addr = reinterpret_cast<sockaddr*>(&src);
    hints.ai_src_len = sizeof(src);

    // Resolve the server's address. If this connection request is for the
    // loopback connection, then we are going to
    int gai_ret =
        rdma_getaddrinfo(server.data(), port_str.data(), &hints, &resolved);
    ROME_CHECK_QUIET(
        ROME_RETURN(InternalErrorBuilder()
                    << "rdma_getaddrinfo(): " << gai_strerror(gai_ret)),
        gai_ret == 0);

    ibv_qp_init_attr init_attr = DefaultQpInitAttr();
    auto err = rdma_create_ep(&id, resolved, pd(), &init_attr);
    rdma_freeaddrinfo(resolved);
    if (err) {
      Release();
      return util::InternalErrorBuilder()
             << "rdma_create_ep(): " << strerror(errno) << " (" << errno << ")";
    }
    ROME_DEBUG("[Connect] (Node {}) Trying to connect to: {} (id={})", my_id_,
               peer_id, fmt::ptr(id));

    if (peer_id == my_id_) return ConnectLoopback(id);

    auto* event_channel = rdma_create_event_channel();
    RDMA_CM_CHECK(fcntl, event_channel->fd, F_SETFL,
                  fcntl(event_channel->fd, F_GETFL) | O_NONBLOCK);
    RDMA_CM_CHECK(rdma_migrate_id, id, event_channel);

    rdma_conn_param conn_param;
    std::memset(&conn_param, 0, sizeof(conn_param));
    conn_param.private_data = &my_id_;
    conn_param.private_data_len = sizeof(my_id_);
    conn_param.retry_count = 7;
    conn_param.rnr_retry_count = 1;
    conn_param.responder_resources = 8;
    conn_param.initiator_depth = 8;

    RDMA_CM_CHECK(rdma_connect, id, &conn_param);

    // Handle events.
    while (true) {
      rdma_cm_event* event;
      auto result = rdma_get_cm_event(id->channel, &event);
      while (result < 0 && errno == EAGAIN) {
        result = rdma_get_cm_event(id->channel, &event);
      }
      ROME_DEBUG("[Connect] (Node {}) Got event: {} (id={})", my_id_,
                 rdma_event_str(event->event), fmt::ptr(id));

      absl::StatusOr<ChannelType*> conn_or;
      switch (event->event) {
        case RDMA_CM_EVENT_ESTABLISHED: {
          RDMA_CM_CHECK(rdma_ack_cm_event, event);
          auto conn = established_.find(peer_id);
          if (bool is_established = (conn != established_.end());
              is_established && peer_id != my_id_) {
            Release();

            // Since we are initiating the disconnection, we must get and ack
            // the event.
            ROME_DEBUG("[Connect] (Node {}) Disconnecting: (id={})", my_id_,
                       fmt::ptr(id));
            RDMA_CM_CHECK(rdma_disconnect, id);
            rdma_cm_event* event;
            auto result = rdma_get_cm_event(id->channel, &event);
            while (result < 0 && errno == EAGAIN) {
              result = rdma_get_cm_event(id->channel, &event);
            }
            RDMA_CM_CHECK(rdma_ack_cm_event, event);

            rdma_destroy_ep(id);
            rdma_destroy_event_channel(event_channel);

            if (is_established) {
              ROME_DEBUG("[Connect] Already connected: {}", peer_id);
              return conn->second.get();
            } else {
              return util::UnavailableErrorBuilder()
                     << "[Connect] (Node " << my_id_
                     << ") Connection is already requested: " << peer_id;
            }
          }

          // If this code block is reached, then the connection established by
          // this call is the first successful connection to be established and
          // therefore we must add it to the set of established connections.
          ROME_DEBUG(
              "Connected: dev={}, addr={}, port={}", id->verbs->device->name,
              inet_ntoa(reinterpret_cast<sockaddr_in*>(rdma_get_local_addr(id))
                            ->sin_addr),
              rdma_get_src_port(id));

          RDMA_CM_CHECK(fcntl, event_channel->fd, F_SETFL,
                        fcntl(event_channel->fd, F_GETFL) | O_SYNC);
          RDMA_CM_CHECK(fcntl, id->recv_cq->channel->fd, F_SETFL,
                        fcntl(id->recv_cq->channel->fd, F_GETFL) | O_NONBLOCK);
          RDMA_CM_CHECK(fcntl, id->send_cq->channel->fd, F_SETFL,
                        fcntl(id->send_cq->channel->fd, F_GETFL) | O_NONBLOCK);

          // Allocate a new control channel to be used with this connection
          auto channel = std::make_unique<ChannelType>(id);
          auto iter = established_.emplace(
              peer_id, new Connection{my_id_, peer_id, std::move(channel)});
          ROME_ASSERT(iter.second, "Unexepected error");
          auto* new_conn = established_[peer_id].get();
          Release();
          return new_conn;
        }
        case RDMA_CM_EVENT_ADDR_RESOLVED:
          ROME_WARN("Got addr resolved...");
          RDMA_CM_CHECK(rdma_ack_cm_event, event);
          break;
        default: {
          auto cm_event = event->event;
          RDMA_CM_CHECK(rdma_ack_cm_event, event);
          backoff_us_ =
              backoff_us_ > 0
                  ? std::min((backoff_us_ + (100 * my_id_)) * 2, kMaxBackoffUs)
                  : kMinBackoffUs;
          Release();
          rdma_destroy_ep(id);
          rdma_destroy_event_channel(event_channel);
          if (cm_event == RDMA_CM_EVENT_REJECTED) {
            std::this_thread::sleep_for(std::chrono::microseconds(backoff_us_));
            return util::UnavailableErrorBuilder()
                   << "Connection request rejected";
          }
          return InternalErrorBuilder()
                 << "Got unexpected event: " << rdma_event_str(cm_event);
        }
      }
    }
  } else {
    return util::UnavailableErrorBuilder() << "Lock acquisition failed";
  }
}

template <typename ChannelType>
absl::StatusOr<typename ConnectionManager<ChannelType>::conn_type*>
ConnectionManager<ChannelType>::GetConnection(uint32_t peer_id) {
  while (!Acquire(my_id_)) {
    std::this_thread::yield();
  }
  auto conn = established_.find(peer_id);
  if (conn != established_.end()) {
    auto result = conn->second.get();
    Release();
    return result;
  } else {
    Release();
    return util::NotFoundErrorBuilder() << "Connection not found: " << peer_id;
  }
}

}  // namespace rome::rdma