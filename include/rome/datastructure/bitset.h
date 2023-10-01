#include <bit>
#include <cstdint>
#include <type_traits>
#include <concepts>
#include <cassert>

#include "rome/rome.h"

namespace rome::datastructure {

/// Int constant
template<int N>
using Int = std::integral_constant<int, N>;

/// Bool constant
template<bool B>
using Bool = std::integral_constant<bool, B>;

/// uint8_t based bitset with N bits
template<int N>  
class uint8_bitset {
public:


  static constexpr uint8_t all_set_bits = []() constexpr -> uint8_t {
    uint8_bitset<N> result;
    for(int i = 0; i < N; ++i) {
      result.set(i, Bool<true>{}); 
    }
    return static_cast<uint8_t>(result);
  }();

  static constexpr int bits = N;

  static_assert(N <= 8);

  ROME_HOST_DEVICE constexpr uint8_bitset(std::integral auto x) : data(x) {
    static_assert(sizeof(x) <= N);
  }

  ROME_HOST_DEVICE constexpr uint8_bitset() = default;
  ROME_HOST_DEVICE constexpr uint8_bitset(const uint8_bitset&) = default;
  ROME_HOST_DEVICE constexpr uint8_bitset(uint8_bitset&&) = default;
  ROME_HOST_DEVICE constexpr uint8_bitset& operator=(const uint8_bitset&) = default;
  ROME_HOST_DEVICE constexpr uint8_bitset& operator=(uint8_bitset&&) = default;

  /**
   * Set bit to x
   */
  ROME_HOST_DEVICE constexpr void set(uint8_t bit, bool x = true) {
    if (x == true) {
      set(bit, Bool<true>{});
    } else {
      set(bit, Bool<false>{});
    }
  }

  /**
   * Set bit to x
   */
  template<bool x>
  ROME_HOST_DEVICE constexpr void set(uint8_t bit, Bool<x>) {
    assert(bit < N);
    if constexpr (x == true) {
      data |= (0x1 << bit);
    } else {
      data &= (~(0x1 << bit));
    }
  }

  /// Get bit
  ROME_HOST_DEVICE constexpr bool get(uint8_t bit) const {
    assert(bit < N);
    return ((data >> bit) & 0x1) == 0x1;
  }

  /// Get underlying data
  ROME_HOST_DEVICE constexpr operator uint8_t() const {
    return data;
  }

  ROME_HOST_DEVICE constexpr bool operator==(const uint8_bitset<bits>& rhs) const {
    return (all_set_bits & rhs.data) == (all_set_bits & data); 
  }

  ROME_HOST_DEVICE constexpr bool operator!=(const uint8_bitset<bits>& rhs) const {
    return (all_set_bits & rhs.data) != (all_set_bits & data); 
  }

private:
  uint8_t data = 0x0;
};


template<int N>
ROME_HOST_DEVICE constexpr uint8_bitset<N> get_all_set_bits(Int<N>) {
  return uint8_bitset<N>(uint8_bitset<N>::all_set_bits);
}

}

