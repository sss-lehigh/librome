#include "stopwatch.h"

#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>
#include <ostream>
#include <sstream>
#include <streambuf>

#include "rome/logging/logging.h"

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

}  // namespace

/*static*/ std::unique_ptr<Stopwatch> Stopwatch::Create(std::string_view name) {
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

Stopwatch::Split::Split(uint32_t tsc_freq_khz)
    : running_(true), tsc_freq_khz_(tsc_freq_khz) {
  start_ = RdtscpAcquire();
}

void Stopwatch::Split::Stop() {
  if (!running_) return;
  end_ = RdtscpRelease();
  running_ = false;
}

namespace {
constexpr double KhzToGhz(int khz) { return double(khz) / 1e6; }
}  // namespace

// # cycles / Ghz = # cycles / (cycles / nanosecond) = nanoseconds
std::chrono::nanoseconds Stopwatch::Split::GetRuntimeNanoseconds() {
  if (running_) return std::chrono::nanoseconds(0);
  // std::cerr << "ticks: " << end_ - start_ << ", freq: " << tsc_freq_khz_
  //           << std::endl;
  return std::chrono::nanoseconds(
      uint64_t((end_ - start_) / KhzToGhz(tsc_freq_khz_)));
}

std::unique_ptr<Stopwatch::Split> Stopwatch::NewSplit() {
  return std::make_unique<Split>(tsc_freq_khz_);
}

void Stopwatch::Stop() { split_.Stop(); }

std::chrono::nanoseconds Stopwatch::GetRuntimeNanoseconds() {
  return split_.GetRuntimeNanoseconds();
}

std::string Stopwatch::ToString() {
  std::stringstream ss;
  ss << "runtime: " << GetRuntimeNanoseconds().count() << " ns";
  return ss.str();
}

}  // namespace rome::metrics