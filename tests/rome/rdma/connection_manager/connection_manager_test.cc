#include "connection_manager.h"

#include <algorithm>
#include <barrier>
#include <chrono>
#include <experimental/coroutine>
#include <iterator>
#include <random>
#include <thread>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "protos/rdma.pb.h"
#include "protos/testutil.pb.h"
#include "rome/rdma/channel/rdma_messenger.h"
#include "rome/testutil/status_matcher.h"
#include "rome/util/clocks.h"

namespace rome::rdma {

using ::util::SystemClock;
using TestProto = ::rome::testutil::ConnectionManagerTestProto;

static constexpr char kAddress[] = "10.0.0.1";

using Channel = RdmaChannel<TwoSidedRdmaMessenger<>, EmptyRdmaAccessor>;

class ConnectionManagerTest : public ::testing::Test {
 protected:
  ConnectionManagerTest() : rd_(), rand_(rd_()), backoff_dist_(0, 10000000) {}
  void SetUp() { ROME_INIT_LOG(); }

  absl::StatusOr<ConnectionManager<Channel>::conn_type*> Connect(
      ConnectionManager<Channel>* client, uint32_t peer_id,
      std::string_view address, uint16_t port) {
    int tries = 1;
    auto conn_or = client->Connect(peer_id, address, port);
    auto backoff = std::chrono::nanoseconds(100);
    while (!conn_or.ok() && tries < kMaxRetries) {
      ROME_DEBUG(conn_or.status().ToString());
      conn_or = client->Connect(peer_id, address, port);
      ++tries;

      std::this_thread::sleep_for(backoff);
      backoff = std::max(std::chrono::nanoseconds(10000000), backoff * 2);
    }
    if (!conn_or.ok()) ROME_ERROR("Retries exceeded: {}", tries);
    ROME_DEBUG("Number of tries: {}", tries);
    return conn_or;
  }

  static constexpr int kMaxRetries = std::numeric_limits<int>::max();

  std::random_device rd_;
  std::default_random_engine rand_;
  std::uniform_int_distribution<uint32_t> backoff_dist_;
};

template <typename T>
std::string ToString(const std::vector<T>& v) {
  std::stringstream ss;
  for (auto iter = v.begin(); iter < v.end(); ++iter) {
    ss << *iter;
    if (&(*iter) != &v.back()) {
      ss << ", ";
    }
  }
  return ss.str();
}

TEST_F(ConnectionManagerTest, ConstructAndDestroy) {
  // Test plan: Ensure that a `ConnectionManager` that is created then
  // immediately destroyed does not crash.
  static constexpr int kServerId = 1;
  ConnectionManager<Channel> server(kServerId);
}

TEST_F(ConnectionManagerTest, SingleConnection) {
  // Test plan: Something...
  static constexpr int kServerId = 1;
  static constexpr int kClientId = 42;

  ConnectionManager<Channel> server(kServerId);
  ASSERT_OK(server.Start(kAddress, std::nullopt));

  ConnectionManager<Channel> client(kClientId);
  ASSERT_OK(client.Start(kAddress, std::nullopt));

  auto conn_or = Connect(&client, kServerId, server.address(), server.port());
  EXPECT_OK(conn_or);

  conn_or = client.GetConnection(kServerId);
  EXPECT_OK(conn_or);

  client.Shutdown();
  server.Shutdown();
}

TEST_F(ConnectionManagerTest, ConnectionOnOtherIp) {
  // Test plan: Something...
  static constexpr int kServerId = 1;
  static constexpr int kClientId = 42;

  ConnectionManager<Channel> server(kServerId);
  ASSERT_OK(server.Start("10.0.0.1", std::nullopt));

  ConnectionManager<Channel> client(kClientId);
  ASSERT_OK(client.Start("10.0.0.2", std::nullopt));

  auto conn_or = Connect(&client, kServerId, server.address(), server.port());
  EXPECT_OK(conn_or);
  conn_or = client.GetConnection(kServerId);
  EXPECT_OK(conn_or);

  client.Shutdown();
  server.Shutdown();
}

TEST_F(ConnectionManagerTest, LoopbackTest) {
  // Test plan: Something...
  static constexpr int kId = 1;
  ConnectionManager<Channel> node(kId);
  ASSERT_OK(node.Start(kAddress, std::nullopt));
  auto conn_or = Connect(&node, kId, node.address(), node.port());
  EXPECT_OK(conn_or);
  conn_or = node.GetConnection(kId);
  EXPECT_OK(conn_or);
  node.Shutdown();
}

// TEST_F(ConnectionManagerTest, MultipleConnections) {
//   // Test plan: Something...

//   static constexpr uint32_t kServerId = 100;
//   NodeProto server_proto;
//   server_proto.set_node_id(kServerId);
//   ConnectionManager server(kAddress, std::nullopt, server_proto, &handler_);

//   static constexpr int kNumNodes = 10;
//   std::vector<std::unique_ptr<ConnectionManager>> nodes;
//   for (int i = 0; i < kNumNodes; ++i) {
//     NodeProto node;
//     node.set_node_id(i);
//     nodes.emplace_back(std::make_unique<ConnectionManager>(
//         kAddress, std::nullopt, node, &handler_));

//     auto conn_or = Connect(&(*nodes[i]), kServerId, kAddress, server.port());
//     ASSERT_OK(conn_or);

//     auto conn = conn_or.value();

//     TestProto p;
//     *p.mutable_message() = std::to_string(i);
//     EXPECT_OK(conn->channel()->Send(p));

//     absl::StatusOr<Message> m = conn->channel()->TryDeliver();
//     for (; !m.ok() && m.status().code() == absl::StatusCode::kUnavailable;
//          m = conn->channel()->TryDeliver())
//       ;
//     EXPECT_OK(m);
//   }

//   EXPECT_EQ(server.GetNumConnections(), kNumNodes);
// }

TEST_F(ConnectionManagerTest, FullyConnected) {
  // Test plan: Something...

  static constexpr int kNumNodes = 15;
  std::vector<std::unique_ptr<ConnectionManager<Channel>>> conns;
  std::vector<std::pair<uint32_t, uint16_t>> node_info;
  for (int i = 0; i < kNumNodes; ++i) {
    conns.emplace_back(std::make_unique<ConnectionManager<Channel>>(i));
    ASSERT_OK(conns.back()->Start(kAddress, std::nullopt));
    node_info.push_back({i, conns.back()->port()});
  }

  std::random_device rd;
  std::default_random_engine eng(rd());
  std::vector<std::thread> threads;
  std::barrier sync(kNumNodes);
  for (int i = 0; i < kNumNodes; ++i) {
    threads.emplace_back([&conns, &node_info, &eng, &sync, i, this]() {
      // Randomize connection order.
      std::vector<std::pair<uint32_t, uint16_t>> rand;
      std::copy(node_info.begin(), node_info.end(), std::back_inserter(rand));
      std::shuffle(rand.begin(), rand.end(), eng);

      for (auto n : rand) {
        auto conn_or = Connect(&(*conns[i]), n.first, kAddress, n.second);
        if (!conn_or.ok()) {
          ROME_FATAL(conn_or.status().ToString());
        }
      }

      sync.arrive_and_wait();
      ASSERT_EQ(conns[i]->GetNumConnections(), kNumNodes);

      for (auto n : rand) {
        TestProto p;
        *p.mutable_message() = std::to_string(i);
        auto conn_or = conns[i]->GetConnection(n.first);
        EXPECT_OK(VALUE_OR_DIE(conn_or)->channel()->Send(p));
      }

      for (auto n : rand) {
        auto* conn = VALUE_OR_DIE(conns[i]->GetConnection(n.first));
        auto m = conn->channel()->TryDeliver<TestProto>();
        while (!m.ok() && m.status().code() == absl::StatusCode::kUnavailable) {
          m = conn->channel()->TryDeliver<TestProto>();
        }
        EXPECT_OK(m);
        if (m.ok()) {
          ROME_DEBUG("[FullyConnected] (Node {}) Got: {}", conn->src_id(),
                     m->DebugString());
        }
      }

      sync.arrive_and_wait();
      ROME_DEBUG("Shutting down: {}", i);
      conns[i]->Shutdown();
    });
  }

  // Join threads.
  for (auto& t : threads) {
    t.join();
  }
}

}  // namespace rome::rdma