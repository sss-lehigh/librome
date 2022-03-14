#include "rdma_broker.h"

#include <linux/limits.h>
#include <rdma/rdma_cma.h>

#include <barrier>
#include <chrono>
#include <random>
#include <string>
#include <thread>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "rdma_receiver.h"
#include "rdma_util.h"
#include "rome/testutil/status_matcher.h"
#include "rome/util/status_util.h"

namespace rome {
namespace {

using ::util::InternalErrorBuilder;

constexpr uint16_t kPort = 18018;

class FakeRdmaReceiver : public RdmaReceiverInterface {
 public:
  absl::StatusOr<rdma_conn_param*> OnConnectRequest(
      rdma_cm_id* id, rdma_cm_event* event) override {
    ibv_qp_init_attr init_attr;
    std::memset(&init_attr, 0, sizeof(init_attr));
    init_attr.cap.max_send_wr = init_attr.cap.max_recv_wr = 1;
    init_attr.cap.max_send_sge = init_attr.cap.max_recv_sge = 1;
    init_attr.cap.max_inline_data = 0;
    init_attr.qp_type = id->qp_type;
    RDMA_CM_ASSERT(rdma_create_qp, id, nullptr, &init_attr);
    return nullptr;
  }
  void OnEstablished(rdma_cm_id* id, rdma_cm_event* event) override {}
  void OnDisconnect(rdma_cm_id* id, rdma_cm_event* event) override {}
};

class FakeRdmaClient {
 public:
  absl::Status Connect(std::string_view server, uint16_t port) {
    rdma_cm_id* id = nullptr;
    rdma_addrinfo hints, *resolved;
    ibv_qp_init_attr init_attr;

    std::memset(&hints, 0, sizeof(hints));
    hints.ai_port_space = RDMA_PS_TCP;
    hints.ai_flags = AI_NUMERICSERV;

    char hostname[PATH_MAX];
    gethostname(hostname, PATH_MAX);
    int gai_ret = rdma_getaddrinfo(hostname, std::to_string(htons(port)).data(),
                                   &hints, &resolved);
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

TEST(RdmaBrokerTest, InitSpecifiesPort) {
  // Test plan: Create a new `RdmaBroker` that is initialized on a given port to
  // check that it is created successfully.
  ROME_INIT_LOG();
  FakeRdmaReceiver receiver;
  auto broker = RdmaBroker::Create("", kPort, &receiver);
  EXPECT_NE(broker, nullptr);
}

TEST(RdmaBrokerTest, InitDoesNotSpecifyPort) {
  // Test plan: Create a new `RdmaBroker` that is initialized on a given port to
  // check that it is created successfully.
  ROME_INIT_LOG();
  FakeRdmaReceiver receiver;
  auto broker = RdmaBroker::Create("", std::nullopt, &receiver);
  EXPECT_NE(broker, nullptr);
}

TEST(RdmaBrokerTest, ClientConnectsToKnownPort) {
  // Test plan: Create a new `RdmaBroker` that is initialized on a given port to
  // check that its construction succeeds.
  ROME_INIT_LOG();

  std::barrier init(2);
  std::barrier done(2);

  std::thread thread([&]() {
    FakeRdmaReceiver receiver;
    auto broker = RdmaBroker::Create("", kPort, &receiver);
    init.arrive_and_wait();
    done.arrive_and_wait();
    EXPECT_OK(broker->Stop());
  });

  init.arrive_and_wait();

  // Do work
  FakeRdmaClient client;
  EXPECT_OK(client.Connect("", kPort));

  done.arrive_and_wait();
  thread.join();
}

TEST(RdmaBrokerTest, ClientConnectsToUnknownPort) {
  // Test plan: Create a new `RdmaBroker` that is initialized on a given port to
  // check that its construction succeeds.
  ROME_INIT_LOG();

  std::barrier init(2);
  std::barrier done(2);

  FakeRdmaReceiver receiver;
  auto broker = RdmaBroker::Create("", std::nullopt, &receiver);
  std::thread thread([&]() {
    init.arrive_and_wait();
    done.arrive_and_wait();
    EXPECT_OK(broker->Stop());
  });

  init.arrive_and_wait();

  // Do work
  FakeRdmaClient client;
  EXPECT_OK(client.Connect("", broker->port()));

  done.arrive_and_wait();
  thread.join();
}

TEST(RdmaBrokerTest, MultipleClientsConnect) {
  // Test plan: Create a new `RdmaBroker` that is initialized on a given port to
  // check that its construction succeeds.
  ROME_INIT_LOG();

  std::barrier init(2);
  std::barrier done(2);

  std::thread thread([&]() {
    FakeRdmaReceiver receiver;
    auto broker = RdmaBroker::Create("", kPort, &receiver);
    init.arrive_and_wait();
    done.arrive_and_wait();
    EXPECT_OK(broker->Stop());
  });

  init.arrive_and_wait();

  // Connect clients.
  static constexpr int kIters = 1000;
  for (int i = 0; i < kIters; ++i) {
    FakeRdmaClient client;
    EXPECT_OK(client.Connect("", kPort));
  }

  done.arrive_and_wait();
  thread.join();
}

// TODO: Validate failure cases

}  // namespace
}  // namespace rome