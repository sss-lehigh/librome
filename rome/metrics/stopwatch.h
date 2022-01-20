#include <chrono>
#include <cstdint>
#include <memory>

#include "metric.h"

namespace rome::metrics {

class Stopwatch : public Metric {
 public:
  // A split is used to measure a period of time. The period starts when the
  // `Split` is first created and completes whenever a call to `Stop` is made.
  // The purpose of having a specific object to measure each period is to enable
  // monitoring more complex control flows. A `Split` object can be moved
  //
  class Split {
   public:
    explicit Split(uint32_t tsc_freq_khz);

    // No copy
    Split(const Split&) = delete;

    // But can move
    Split(Split&& other)
        : start_(std::move(other.start_)),
          end_(std::move(other.end_)),
          tsc_freq_khz_(std::move(other.tsc_freq_khz_)) {}

    // Stops the split and sets its endpoint. After a call to stop, `end_` is
    // immutable.
    void Stop();

    // Returns the length of the split in nanoseconds.
    std::chrono::nanoseconds GetRuntimeNanoseconds();

   private:
    bool running_;
    uint64_t start_;
    uint64_t end_;
    uint32_t tsc_freq_khz_;
  };

  // Initializes the tsc frequency from either a known file or a compile time
  // variable. Then, starts the stopwatch running by initializing the internal
  // `split_`, which tracks total runtime.
  static std::unique_ptr<Stopwatch> Create(std::string_view name);

  // Returns a new `Split` object that is used to measure a portion of the
  // overall runtime.
  std::unique_ptr<Stopwatch::Split> NewSplit();

  // Stops the stopwatch. Future calls to `Split` will still generate new splits
  // that can be used for timing, but the internal split maintained by the
  // stopwatch is fixed.
  void Stop();

  // Returns the total runtime measured by the stopwatch. The return value is
  // only valid after `Stop` has been called.
  std::chrono::nanoseconds GetRuntimeNanoseconds();

  std::string ToString() override;

 private:
  Stopwatch(std::string_view name, uint64_t tsc_freq_khz)
      : Metric(name), tsc_freq_khz_(tsc_freq_khz), split_(tsc_freq_khz_) {}

  uint64_t tsc_freq_khz_;
  Split split_;
};

}  // namespace rome::metrics