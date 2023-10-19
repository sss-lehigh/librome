#include <chrono>
#include <memory>
#include <type_traits>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/status/status.h"
#include "rome/colosseum/client_adaptor.h"
#include "rome/colosseum/qps_controller.h"
#include "rome/colosseum/streams/streams.h"
#include "rome/colosseum/workload_driver.h"
#include "rome/metrics/summary.h"
#include "rome/util/clocks.h"

ABSL_FLAG(int, max_qps, -1, "Maximum offered QPS during execution");
ABSL_FLAG(int, runtime, 5, "Nmber of seconds to run execution for");

using ::util::SystemClock;

struct SimpleOp {
  enum class Type {
    kAdd,
    kSubtract,
  } type;
  double data;
};

class SimpleClientAdaptor : public rome::ClientAdaptor<SimpleOp> {
 public:
  SimpleClientAdaptor() : sum_(0.0) {}
  absl::Status Start() override { return absl::OkStatus(); }
  absl::Status Apply(const SimpleOp& op) override {
    switch (op.type) {
      case SimpleOp::Type::kAdd:
        sum_ += op.data;
        break;
      case SimpleOp::Type::kSubtract:
        sum_ -= op.data;
    }
    return absl::OkStatus();
  }
  absl::Status Stop() override { return absl::OkStatus(); }

 private:
  double sum_;
};

int main(int argc, char* argv[]) {
  absl::ParseCommandLine(argc, argv);

  // Make client and op stream.
  auto client = std::make_unique<SimpleClientAdaptor>();
  auto op_stream = std::make_unique<rome::WeightedStream<SimpleOp::Type>>(
      std::vector<uint32_t>{1, 1});
  auto data_stream =
      std::make_unique<rome::UniformDoubleStream<double, double>>(0.0, 1000.0);
  auto mapped_stream =
      rome::MappedStream<SimpleOp, SimpleOp::Type, double>::Create(
          [](const auto& op_stream,
             const auto& data_stream) -> absl::StatusOr<SimpleOp> {
            auto op = op_stream->Next();
            if (!op.ok()) return op.status();
            auto data = data_stream->Next();
            if (!data.ok()) return data.status();
            return SimpleOp{op.value(), data.value()};
          },
          std::move(op_stream), std::move(data_stream));

  // Setup qps_controller.
  std::unique_ptr<rome::LeakyTokenBucketQpsController<SystemClock>>
      qps_controller;
  if (absl::GetFlag(FLAGS_max_qps) > 0) {
    qps_controller = rome::LeakyTokenBucketQpsController<SystemClock>::Create(
        absl::GetFlag(FLAGS_max_qps));
  }

  // Create the workload driver.
  auto driver = rome::WorkloadDriver<SimpleOp>::Create(
      std::move(client), std::move(mapped_stream), qps_controller.get());

  // Start the workload driver.
  auto start = SystemClock::now();
  auto runtime = std::chrono::seconds(absl::GetFlag(FLAGS_runtime));
  if (!driver->Start().ok()) exit(1);

  while (SystemClock::now() - start < runtime)
    ;
  if (!driver->Stop().ok()) exit(1);

  std::cout << driver->ToString() << std::endl;
  return 0;
}