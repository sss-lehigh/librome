#pragma once

#include <chrono>
#include <functional>
#include <future>
#include <memory>
#include <thread>

#include "rome/colosseum/client_adaptor.h"
#include "rome/colosseum/qps_controller.h"
#include "rome/colosseum/stream.h"
#include "rome/metrics/counter.h"
#include "rome/metrics/stopwatch.h"
#include "rome/metrics/summary.h"
#include "rome/util/duration_util.h"

namespace rome {

// For reference, a `WorkloadDriver` with a simple `MappedStream` (i.e.,
// consisting of one sub-stream) can achieve roughly 1M QPS. This was measured
// using a client adaptor that does nothing. As the number of constituent
// streams increases, we expect the maximum throughput to decrease but it is not
// likely to be the limiting factor in performance.
template <typename OpType>
class WorkloadDriver {
 public:
  ~WorkloadDriver();

  // Creates a new `WorkloadDriver` from the constiuent client adaptor and
  // stream. If not `nullptr`, the QPS controller is used to limit the
  // throughput of operations fed to the client.
  static std::unique_ptr<WorkloadDriver> Create(
      std::unique_ptr<ClientAdaptor<OpType>> client,
      std::unique_ptr<Stream<OpType>> stream, QpsController* qps_controller,
      std::optional<std::chrono::milliseconds> qps_sampling_rate) {
    return std::unique_ptr<WorkloadDriver>(new WorkloadDriver(
        std::move(client), std::move(stream), qps_controller,
        qps_sampling_rate.value_or(std::chrono::milliseconds(0))));
  }

  // Calls the client's `Start` method before starting the workload driver,
  // returning its error if there is one. Operations are then pulled from
  // `stream_` and passed to `client_`'s `Apply` method. The client will handle
  // operations until either the given stream is exhausted or `Stop` is called.
  absl::Status Start();

  // Stops the workload driver so no new requests are passed to the client.
  // Then, the client's `Stop` method is called so that any pending operations
  // can be finalized.
  absl::Status Stop();

  metrics::Stopwatch* GetStopwatch() { return stopwatch_.get(); }

  std::string ToString() {
    std::stringstream ss;
    ss << ops_ << std::endl;
    ss << qps_summary_ << std::endl;
    ss << *stopwatch_;
    return ss.str();
  }

 private:
  absl::Status Run();

  WorkloadDriver(std::unique_ptr<ClientAdaptor<OpType>> client,
                 std::unique_ptr<Stream<OpType>> stream,
                 QpsController* qps_controller,
                 std::chrono::milliseconds qps_sampling_rate)
      : terminated_(false),
        client_(std::move(client)),
        stream_(std::move(stream)),
        qps_controller_(qps_controller),
        ops_("total_ops"),
        stopwatch_(nullptr),
        qps_sampling_rate_(qps_sampling_rate),
        qps_summary_("actual_qps", "ops/ms", 1000) {}

  std::atomic<bool> terminated_;

  std::unique_ptr<ClientAdaptor<OpType>> client_;
  std::unique_ptr<Stream<OpType>> stream_;

  //! Not owned.
  QpsController* qps_controller_;

  metrics::Counter<uint64_t> ops_;
  std::unique_ptr<metrics::Stopwatch> stopwatch_;

  uint64_t prev_ops_;
  std::chrono::milliseconds qps_sampling_rate_;
  metrics::Summary<double> qps_summary_;

  std::future<absl::Status> run_status_;
  std::unique_ptr<std::thread> run_thread_;
};

// ---------------|
// Implementation |
// ---------------|

template <typename OpType>
WorkloadDriver<OpType>::~WorkloadDriver() {
  terminated_ = true;
  run_thread_->join();
}

template <typename OpType>
absl::Status WorkloadDriver<OpType>::Start() {
  if (terminated_) {
    return absl::UnavailableError(
        "Cannot restart a terminated workload driver.");
  }
  stopwatch_ = metrics::Stopwatch::Create("driver_stopwatch");
  auto client_status = client_->Start();
  if (!client_status.ok()) return client_status;

  auto task =
      std::packaged_task<absl::Status()>(std::bind(&WorkloadDriver::Run, this));
  run_status_ = task.get_future();
  run_thread_ = std::make_unique<std::thread>(std::move(task));
  return absl::OkStatus();
}

template <typename OpType>
absl::Status WorkloadDriver<OpType>::Stop() {
  if (terminated_) {
    return absl::UnavailableError("Workload driver was already terminated");
  }

  // Signal `run_thread_` to stop. After joining the thread, it is guaranteed
  // that no new operations are passed to the client for handling.
  terminated_ = true;
  run_status_.wait();

  // The client's `Stop` may block while there are any outstanding operations.
  // After this call, it is assumed that the client is no longer active.
  auto client_status = client_->Stop();
  stopwatch_->Stop();

  if (!run_status_.get().ok()) return run_status_.get();
  if (!client_status.ok()) return client_status;
  return absl::OkStatus();
}

template <typename OpType>
absl::Status WorkloadDriver<OpType>::Run() {
  absl::Status status = absl::OkStatus();
  while (!terminated_) {
    if (qps_controller_ != nullptr) {
      qps_controller_->Wait();
    }

    auto next_op = stream_->Next();
    if (!next_op.ok()) {
      if (!IsStreamTerminated(next_op.status())) {
        status = next_op.status();
      }
      break;
    }

    auto client_status = client_->Apply(next_op.value());
    if (!client_status.ok()) {
      status = client_status;
      break;
    }

    ++ops_;
    if (auto curr_lap = stopwatch_->GetLapSplit();
        std::chrono::duration_cast<std::chrono::milliseconds>(
            curr_lap.GetRuntimeNanoseconds()) > qps_sampling_rate_) {
      auto sample = (ops_.GetCounter() - prev_ops_) /
                    util::ToDoubleMilliseconds(
                        stopwatch_->GetLap().GetRuntimeNanoseconds());
      qps_summary_ << sample;
      prev_ops_ = ops_.GetCounter();
    }
  }
  stopwatch_->Stop();
  return status;
}

}  // namespace rome