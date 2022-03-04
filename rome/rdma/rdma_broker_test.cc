#include "rdma_broker.h"

#include <rdma/rdma_cma.h>

#include <barrier>
#include <chrono>
#include <random>
#include <thread>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "rdma_client.h"
#include "rdma_receiver.h"
#include "rdma_util.h"
#include "rome/testutil/status_matcher.h"
#include "rome/util/status_util.h"

namespace rome {
namespace {

using ::util::InternalErrorBuilder;

constexpr char kServer[] = "10.0.2.5";

class FakeRdmaReceiver : public RdmaReceiverInterface {
 public:
  absl::Status OnConnectRequest(rdma_cm_id* id) override {
    ibv_qp_init_attr init_attr;
    std::memset(&init_attr, 0, sizeof(init_attr));
    init_attr.cap.max_send_wr = init_attr.cap.max_recv_wr = 1;
    init_attr.cap.max_send_sge = init_attr.cap.max_recv_sge = 1;
    init_attr.cap.max_inline_data = 0;
    init_attr.qp_type = id->qp_type;
    RDMA_CM_ASSERT(rdma_create_qp, id, nullptr, &init_attr);
    return absl::OkStatus();
  }
  void OnEstablished(rdma_cm_id* id) override {}
  void OnDisconnect(rdma_cm_id* id) override {}
};

class FakeRdmaClient : public RdmaClientInterface {
 public:
  absl::Status Connect(std::string_view server, uint32_t port) override {
    rdma_cm_id* id = nullptr;
    rdma_addrinfo hints, *resolved;
    ibv_qp_init_attr init_attr;

    std::memset(&hints, 0, sizeof(hints));
    hints.ai_port_space = RDMA_PS_TCP;
    hints.ai_flags = AI_NUMERICSERV;

    int gai_ret = rdma_getaddrinfo(server.data(), nullptr, &hints, &resolved);
    ROME_CHECK_QUIET(
        ROME_RETURN(InternalErrorBuilder()
                    << "rdma_getaddrinfo(): " << gai_strerror(gai_ret)),
        gai_ret == 0);
    reinterpret_cast<sockaddr_in*>(resolved->ai_dst_addr)->sin_port = port;

    std::memset(&init_attr, 0, sizeof(init_attr));
    init_attr.cap.max_send_wr = init_attr.cap.max_recv_wr = 1;
    init_attr.cap.max_send_sge = init_attr.cap.max_recv_sge = 1;
    init_attr.cap.max_inline_data = 0;
    init_attr.qp_type = ibv_qp_type(resolved->ai_qp_type);
    RDMA_CM_CHECK(rdma_create_ep, &id, resolved, nullptr, &init_attr);

    RDMA_CM_CHECK(rdma_connect, id, nullptr);
    ROME_INFO(
        "Connected to {} (port={})",
        inet_ntoa(
            reinterpret_cast<sockaddr_in*>(rdma_get_peer_addr(id))->sin_addr),
        rdma_get_dst_port(id));

    return absl::OkStatus();
  }
};

TEST(RdmaBrokerTest, FakeTest) {
  // Test plan: test_plan
  ROME_INIT_LOG();

  std::barrier init(2);
  std::barrier done(2);

  std::thread thread([&]() {
    FakeRdmaReceiver receiver;
    RdmaBroker server("server", "0.0.0.0", 18018, &receiver);
    init.arrive_and_wait();
    done.arrive_and_wait();
    EXPECT_OK(server.Stop());
  });

  init.arrive_and_wait();

  // Do work
  FakeRdmaClient client;
  EXPECT_OK(client.Connect(kServer, 18018));
  EXPECT_OK(client.Connect(kServer, 18018));
  EXPECT_OK(client.Connect(kServer, 18018));

  done.arrive_and_wait();
  thread.join();
}

// TODO: Validate failure cases

}  // namespace
}  // namespace rome