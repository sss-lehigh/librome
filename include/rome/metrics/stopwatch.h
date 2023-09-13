#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <fstream>
#include <iostream>
#include <limits>
#include <ostream>
#include <sstream>
#include <streambuf>

#include "rome/logging/logging.h"

#include "metric.h"

namespace rome::metrics {

namespace {

// All of these are KHz
constexpr char kTscFreqKhzFilePath[] =
    "/sys/devices/system/cpu/cpu0/tsc_freq_khz";
constexpr char kMaxFreqFilePath[] =
    "/sys/devices/system/cpu/cpu0/cpufreq/base_frequency";
constexpr int kDefaultCpuFreqKhz = 2300000;

inline uint64_t RdtscpAcquire() {
  uint32_t lo, hi;
  asm volatile(
      "MFENCE\n\t"
      "RDTSCP\n\t"
      : "=a"(lo), "=d"(hi)::"%ecx");
  return ((static_cast<uint64_t>(hi) << 32) + lo);
}

inline uint64_t RdtscpRelease() {
  uint32_t lo, hi;
  asm volatile(
      "RDTSCP\n\t"
      "LFENCE\n\t"
      : "=a"(lo), "=d"(hi)::"%ecx");
  return ((static_cast<uint64_t>(hi) << 32) + lo);
}

constexpr double KhzToGhz(int khz) { return double(khz) / 1e6; }

}  // namespace

class Stopwatch : public Metric {
public:
  /// A split is used to measure a period of time. The period starts when the
  /// `Split` is first created and completes whenever a call to `Stop` is made.
  /// The purpose of having a specific object to measure each period is to enable
  /// monitoring more complex control flows. A `Split` object can be moved
  ///
  class Split {
  public:
    Split(uint32_t tsc_freq_khz, uint64_t start) : Split(tsc_freq_khz, start, RdtscpAcquire()) {}
    Split(uint32_t tsc_freq_khz, uint64_t start, uint64_t end) : tsc_freq_khz_(tsc_freq_khz), start_(start), end_(end) {}

    // Default move and copy.
    Split(const Split&) = default;
    Split(Split&& other) = default;

    /// Returns the length of the split in nanoseconds.
    std::chrono::nanoseconds GetRuntimeNanoseconds() {
      return std::chrono::nanoseconds(uint64_t((end_ - start_) / KhzToGhz(tsc_freq_khz_)));
    }

  private:
    uint32_t tsc_freq_khz_;
    uint64_t start_;
    uint64_t end_;
  };

  /// Initializes the tsc frequency from either a known file or a compile time
  /// variable. Then, starts the stopwatch running by initializing the internal
  /// `split_`, which tracks total runtime.
  static std::unique_ptr<Stopwatch> Create(std::string_view name) {
    // Attempt to read the TSC frequency from the tsc_freq_khz file first, then
    // try to read the maximum cpu frequency. The latter is not perfect for modern
    // Intel machines but it is better than nothing. If there is no max frequency
    // then use whatever is provided at compile time.
    uint64_t tsc_freq_khz;
    std::ifstream file;
    file.open(kTscFreqKhzFilePath);
    if (file.is_open()) {
      ROME_INFO("Loading tsc_freq from tsc_freq_khz");
      file >> tsc_freq_khz;
    } else {
      file.clear();
      file.open(kMaxFreqFilePath);
      if (file.is_open()) {
        ROME_INFO("Loading tsc_freq from max_cpu_freq");
        file >> tsc_freq_khz;
      } else {
        ROME_WARN(
            "Could not determine CPU frequency. Using compile time value: {} KHz "
            "[RESULTS MAY BE INACCURATE]",
            kDefaultCpuFreqKhz);
        tsc_freq_khz = kDefaultCpuFreqKhz;
      }
    }
    ROME_INFO("Using tsc_freq: {}", tsc_freq_khz);
    return std::unique_ptr<Stopwatch>(new Stopwatch(name, tsc_freq_khz));
  }

  /// Returns a new `Split` object that is used to measure a portion of the
  /// overall runtime.
  Split GetSplit() { return Split(tsc_freq_khz_, start_); }

  Split GetLap() {
    auto split = Split(tsc_freq_khz_, lap_);
    lap_ = RdtscpAcquire();
    return split;
  }

  Split GetLapSplit() { return Split(tsc_freq_khz_, lap_); }

  /// Stops the stopwatch. Future calls to `Split` will still generate new splits
  /// that can be used for timing, but the internal split maintained by the
  /// stopwatch is fixed.
  void Stop() { end_ = RdtscpRelease(); }

  /// Returns the total runtime measured by the stopwatch. The return value is
  /// only valid after `Stop` has been called.
  std::chrono::nanoseconds GetRuntimeNanoseconds() {
    return Split(tsc_freq_khz_, start_, end_).GetRuntimeNanoseconds();
  }


  /// Gets a string representation
  std::string ToString() override {
    std::stringstream ss;
    ss << "runtime: " << GetRuntimeNanoseconds().count() << " ns";
    return ss.str();
  }

  /// Gets a protobuf representation
  MetricProto ToProto() override {
    MetricProto proto;
    proto.set_name(name_);
    proto.mutable_stopwatch()->set_runtime_ns(GetRuntimeNanoseconds().count());
    return proto;
  }


 private:
  Stopwatch(std::string_view name, uint64_t tsc_freq_khz) 
    : Metric(name),
      tsc_freq_khz_(tsc_freq_khz),
      start_(RdtscpAcquire()),
      end_(0),
      lap_(start_) {}


  uint64_t tsc_freq_khz_;
  uint64_t start_;
  uint64_t end_;
  uint64_t lap_;
};

}  // namespace rome::metrics
