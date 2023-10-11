#include "twosided_messenger.h"

#include <random>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "rdma_accessor.h"
#include "rdma_channel.h"
#include "protos/testutil.pb.h"
#include "rome/rdma/rdma_broker.h"

namespace rome::rdma {
namespace {

using ::util::InternalErrorBuilder;

constexpr char kServer[] = "10.0.0.1";
constexpr uint32_t kPort = 18018;
constexpr uint32_t kCapacity = 1UL << 12;
constexpr int32_t kRecvMaxBytes = 64;
constexpr uint32_t kNumWr = kCapacity / kRecvMaxBytes;
const std::string kMessage = "Shhh, it's a message!";

using ChannelType = RdmaChannel<TwoSidedRdmaMessenger<kCapacity, kRecvMaxBytes>,
                                EmptyRdmaAccessor>;

class FakeRdmaReceiver : public RdmaReceiverInterface {
 public:
  void OnConnectRequest(rdma_cm_id* id, rdma_cm_event* event) override {
    ibv_qp_init_attr init_attr;
    std::memset(&init_attr, 0, sizeof(init_attr));
    init_attr.cap.max_send_wr = init_attr.cap.max_recv_wr = kNumWr;
    init_attr.cap.max_send_sge = init_attr.cap.max_recv_sge = 1;
    init_attr.cap.max_inline_data = 0;
    init_attr.qp_type = id->qp_type;
    RDMA_CM_ASSERT(rdma_create_qp, id, nullptr, &init_attr);

    id_ = id;
    channel_ = std::make_unique<ChannelType>(id_);

    RDMA_CM_ASSERT(rdma_accept, id, nullptr);
    rdma_ack_cm_event(event);
  }

  void OnEstablished(rdma_cm_id* id, rdma_cm_event* event) override {
    rdma_ack_cm_event(event);
  }

  void OnDisconnect(rdma_cm_id* id) override { rdma_disconnect(id); }

  absl::StatusOr<testutil::RdmaChannelTestProto> Deliver() {
    return channel_->TryDeliver<testutil::RdmaChannelTestProto>();
  }

 private:
  rdma_cm_id* id_;
  std::unique_ptr<ChannelType> channel_;
};

class FakeRdmaClient {
 public:
  ~FakeRdmaClient() { rdma_destroy_ep(id_); }

  absl::Status Connect(std::string_view server, uint16_t port) {
    rdma_cm_id* id = nullptr;
    rdma_addrinfo hints, *resolved;
    ibv_qp_init_attr init_attr;

    std::memset(&hints, 0, sizeof(hints));
    hints.ai_port_space = RDMA_PS_TCP;
    hints.ai_flags = AI_NUMERICSERV;
    int gai_ret = rdma_getaddrinfo(
        server.data(), std::to_string(htons(port)).data(), &hints, &resolved);
    ROME_CHECK_QUIET(
        ROME_RETURN(InternalErrorBuilder()
                    << "rdma_getaddrinfo(): " << gai_strerror(gai_ret)),
        gai_ret == 0);
    ROME_ASSERT(
        reinterpret_cast<sockaddr_in*>(resolved->ai_dst_addr)->sin_port == port,
        "Port does not match: expected={}, actual={}", port,
        reinterpret_cast<sockaddr_in*>(resolved->ai_dst_addr)->sin_port);

    std::memset(&init_attr, 0, sizeof(init_attr));
    init_attr.cap.max_send_wr = init_attr.cap.max_recv_wr = kNumWr;
    init_attr.cap.max_send_sge = init_attr.cap.max_recv_sge = 1;
    init_attr.cap.max_inline_data = 0;
    init_attr.qp_type = ibv_qp_type(resolved->ai_qp_type);
    RDMA_CM_CHECK(rdma_create_ep, &id, resolved, nullptr, &init_attr);

    id_ = id;
    channel_ = std::make_unique<ChannelType>(id_);

    RDMA_CM_CHECK(rdma_connect, id, nullptr);
    ROME_INFO(
        "Connected to {} (port={})",
        inet_ntoa(
            reinterpret_cast<sockaddr_in*>(rdma_get_peer_addr(id))->sin_addr),
        rdma_get_dst_port(id));

    return absl::OkStatus();
  }

  absl::Status Send(const testutil::RdmaChannelTestProto& proto) {
    return channel_->Send<testutil::RdmaChannelTestProto>(proto);
  }

 private:
  rdma_cm_id* id_;
  std::unique_ptr<ChannelType> channel_;
};

class RdmaChannelTest : public ::testing::Test {
 protected:
  RdmaChannelTest() { ROME_INIT_LOG(); }

  void SetUp() {
    broker_ = RdmaBroker::Create(kServer, kPort, &receiver_);
    ASSERT_NE(broker_, nullptr);
    ASSERT_OK(client_.Connect(kServer, kPort));
  }

  FakeRdmaReceiver receiver_;
  std::unique_ptr<RdmaBroker> broker_;
  FakeRdmaClient client_;
};

TEST_F(RdmaChannelTest, Test) {
  // Test plan: Do something crazy
  RdmaChannel<EmptyRdmaMessenger, EmptyRdmaAccessor> channel(nullptr);
  EXPECT_TRUE(true);
}

TEST_F(RdmaChannelTest, SendOnce) {
  // Test plan: Create a channel and test that it can send messages without
  // failing. This does not actually check that the message arrives, but that
  // the send does not fail.
  testutil::RdmaChannelTestProto proto;
  *proto.mutable_message() = kMessage;
  EXPECT_OK(client_.Send(proto));
}

TEST_F(RdmaChannelTest, DeliverOnce) {
  testutil::RdmaChannelTestProto expected;
  *expected.mutable_message() = kMessage;
  ASSERT_OK(client_.Send(expected));
  auto msg_or = receiver_.Deliver();
  ASSERT_OK(msg_or.status());
  EXPECT_EQ(msg_or->message(), kMessage);
}

TEST_F(RdmaChannelTest, DeliverMultipleWithoutRepopulatingRecvBuffer) {
  testutil::RdmaChannelTestProto proto;
  *proto.mutable_message() = kMessage;

  for (int i = 0; i < (kCapacity / 2) / kRecvMaxBytes; ++i) {
    ASSERT_OK(client_.Send(proto));
    auto proto_or = receiver_.Deliver();
    ASSERT_OK(proto_or.status());
    EXPECT_EQ(proto_or->message(), kMessage);
  }
}

TEST_F(RdmaChannelTest, DeliverMultipleRepopulatingRecvBufferOnce) {
  testutil::RdmaChannelTestProto proto;
  *proto.mutable_message() = kMessage;

  for (int i = 0; i < ((kCapacity / 2) / kRecvMaxBytes) + 1; ++i) {
    ASSERT_OK(client_.Send(proto));
    auto proto_or = receiver_.Deliver();
    ASSERT_OK(proto_or.status());
    EXPECT_EQ(proto_or->message(), kMessage);
  }
}

TEST_F(RdmaChannelTest, DeliverMultipleRepopulatingRecvBufferMultipleTimes) {
  testutil::RdmaChannelTestProto proto;
  *proto.mutable_message() = kMessage;

  for (int i = 0; i < ((kCapacity / 2) / kRecvMaxBytes) * 10; ++i) {
    ASSERT_OK(client_.Send(proto));
    auto proto_or = receiver_.Deliver();
    ASSERT_OK(proto_or.status());
    EXPECT_EQ(proto_or->message(), kMessage);
  }
}

TEST_F(RdmaChannelTest, LargeProtoExhaustsBuffer) {
  testutil::RdmaChannelTestProto proto;

  static const char alphabet[] =
      "abcdefghijklmnopqrstuvwxyz"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "0123456789";

  std::random_device rd;
  std::default_random_engine eng(rd());
  std::uniform_int_distribution<> dist(
      0, sizeof(alphabet) / sizeof(*alphabet) - 2);

  // Each protobuf string includes at least one byte for its wire-format and its
  // field number. It also includes a `varint` that denotes the strings length.
  // Therefore, if we set the size of the field to be exactly the number of
  // bytes we are able to handle at the receiver, then the total bytes will be
  // larger and the `Send` operations should fail.
  // See (https://developers.google.com/protocol-buffers/docs/encoding).
  static constexpr int kSize = kRecvMaxBytes;
  std::string str;
  str.reserve(kSize);
  std::generate_n(std::back_inserter(str), kSize, [&]() { return dist(eng); });

  proto.mutable_message()->reserve(str.size());
  proto.mutable_message()->swap(str);

  EXPECT_THAT(client_.Send(proto),
              ::testutil::StatusIs(absl::StatusCode::kResourceExhausted));
}

}  // namespace
}  // namespace rome::rdma