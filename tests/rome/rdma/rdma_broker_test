#include "rdma_broker.h"

#include <linux/limits.h>
#include <rdma/rdma_cma.h>

#include <barrier>
#include <chrono>
#include <memory>
#include <random>
#include <string>
#include <thread>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "rdma_receiver.h"
#include "rdma_util.h"
#include "rome/rdma/rdma_device.h"
#include "rome/testutil/status_matcher.h"
#include "rome/util/status_util.h"

namespace rome::rdma {
namespace {

using ::util::InternalErrorBuilder;

constexpr char kIpAddress[] = "10.0.0.1";

class FakeRdmaReceiver : public RdmaReceiverInterface {
 public:
  void OnConnectRequest(rdma_cm_id* id, rdma_cm_event* event) override {
    ibv_qp_init_attr init_attr;
    std::memset(&init_attr, 0, sizeof(init_attr));
    init_attr.cap.max_send_wr = init_attr.cap.max_recv_wr = 1;
    init_attr.cap.max_send_sge = init_attr.cap.max_recv_sge = 1;
    init_attr.cap.max_inline_data = 0;
    init_attr.qp_type = id->qp_type;
    RDMA_CM_ASSERT(rdma_create_qp, id, nullptr, &init_attr);
    RDMA_CM_ASSERT(rdma_ack_cm_event, event);

    rdma_conn_param conn_param;
    std::memset(&conn_param, 0, sizeof(conn_param));
    RDMA_CM_ASSERT(rdma_accept, id, &conn_param);
  }

  void OnEstablished(rdma_cm_id* id, rdma_cm_event* event) override {}
  void OnDisconnect(rdma_cm_id* id) override {}
};

class FakeRdmaClient {
 public:
  absl::Status Connect(std::optional<std::string_view> server, uint16_t port) {
    rdma_cm_id* id = nullptr;
    rdma_addrinfo hints, *resolved;
    ibv_qp_init_attr init_attr;

    std::memset(&hints, 0, sizeof(hints));
    hints.ai_port_space = RDMA_PS_TCP;
    hints.ai_flags = AI_NUMERICSERV;

    char hostname[PATH_MAX];
    gethostname(hostname, PATH_MAX);
    ROME_DEBUG("Connecting to: {}:{}",
               server.has_value() ? server.value() : hostname, port);
    int gai_ret =
        rdma_getaddrinfo(server.has_value() ? server.value().data() : hostname,
                         std::to_string(htons(port)).data(), &hints, &resolved);
    ROME_CHECK_QUIET(
        ROME_RETURN(InternalErrorBuilder()
                    << "rdma_getaddrinfo(): " << gai_strerror(gai_ret)),
        gai_ret == 0);

    std::memset(&init_attr, 0, sizeof(init_attr));
    init_attr.cap.max_send_wr = init_attr.cap.max_recv_wr = 1;
    init_attr.cap.max_send_sge = init_attr.cap.max_recv_sge = 1;
    init_attr.cap.max_inline_data = 0;
    init_attr.qp_type = ibv_qp_type(resolved->ai_qp_type);
    RDMA_CM_CHECK(rdma_create_ep, &id, resolved, nullptr, &init_attr);

    RDMA_CM_CHECK(rdma_connect, id, nullptr);
    ROME_DEBUG(
        "Connected to {} (port={})",
        inet_ntoa(
            reinterpret_cast<sockaddr_in*>(rdma_get_peer_addr(id))->sin_addr),
        rdma_get_dst_port(id));

    return absl::OkStatus();
  }
};

class RdmaBrokerTest : public ::testing::Test {
 protected:
  void SetUp() {
    ROME_INIT_LOG();
    auto devices = RdmaDevice::GetAvailableDevices();
    ROME_ASSERT(devices.ok() && !devices->empty(), devices.status().message());

    // Listen on all devices
    broker_ = RdmaBroker::Create(std::nullopt, std::nullopt, &receiver_);
  }

  void TearDown() { ASSERT_OK(broker_->Stop()); }

  absl::Status Connect(FakeRdmaClient* client) {
    return client->Connect(kIpAddress, broker_->port());
  }

  FakeRdmaReceiver receiver_;
  std::unique_ptr<RdmaBroker> broker_;
};

TEST_F(RdmaBrokerTest, ClientConnects) {
  // Test plan: Create a new `RdmaBroker` that is initialized on a given port to
  // check that its construction succeeds.
  FakeRdmaClient client;
  EXPECT_OK(Connect(&client));
}

TEST_F(RdmaBrokerTest, MultipleClientsConnect) {
  // Test plan: Create a new `RdmaBroker` that is initialized on a given port to
  // check that its construction succeeds.
  // Connect clients.
  static constexpr int kIters = 100;
  std::vector<FakeRdmaClient> clients;
  for (int i = 0; i < kIters; ++i) {
    clients.emplace_back();
    EXPECT_OK(Connect(&clients.back()));
  }
}

// TODO: Validate failure cases

}  // namespace
}  // namespace rome