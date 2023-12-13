#pragma once
#include <cstdint>

static inline uint64_t rdtscp() {
  uint32_t lo, hi;
  __asm__ __volatile__("mfence\nrdtscp\nlfence\n"
                       : "=a"(lo), "=d"(hi)
                       :
                       : "%ecx");
  return (((uint64_t)hi) << 32) + lo;
}
