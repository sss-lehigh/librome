#include <atomic>
#include <iostream>

#include "rome/datastructure/bitset.h"

#pragma once

namespace rome::datastructure {

  template<typename T, uint8_t B>
  class atomic_marked_ptr;

  struct nullptr_m_t {};

  constexpr inline nullptr_m_t nullptr_m;

  template<typename T, uint8_t B>
  class marked_ptr {
  public:
    static constexpr uint8_t bits = B;
    static_assert(bits < 64, "Need at least 1 bit for indexing");

    ROME_HOST_DEVICE bool is_valid() const {
      return (1 << bits) <= alignof(T);
    }

    ROME_HOST_DEVICE explicit constexpr marked_ptr(T* pointer) : ptr(pointer) {}
    
    ROME_HOST_DEVICE explicit constexpr marked_ptr(T* pointer, uint8_bitset<bits> mask) : ptr(pointer) {
      ptr = std::bit_cast<T*>(std::bit_cast<size_t>(ptr) | static_cast<size_t>(mask));
    }
    
    ROME_HOST_DEVICE constexpr marked_ptr(std::nullptr_t pointer) : ptr(pointer) {}

    ROME_HOST_DEVICE marked_ptr() = default;
    ROME_HOST_DEVICE marked_ptr(const marked_ptr&) = default;
    ROME_HOST_DEVICE marked_ptr(marked_ptr&&) = default;
    
    ROME_HOST_DEVICE marked_ptr& operator=(const marked_ptr&) = default;
    ROME_HOST_DEVICE marked_ptr& operator=(marked_ptr&&) = default;

    ROME_HOST_DEVICE constexpr T* cast() const {
      auto mask = ~static_cast<size_t>(get_all_set_bits(Int<bits>{}));
      return std::bit_cast<T*>(std::bit_cast<size_t>(ptr) & mask);
    }

    ROME_HOST_DEVICE explicit constexpr operator T*() const {
      return this->cast(); 
    }

    ROME_HOST_DEVICE constexpr uint8_bitset<bits> marks() const {
      auto mask = static_cast<size_t>(get_all_set_bits(Int<bits>{}));
      return uint8_bitset<bits>(static_cast<uint8_t>(std::bit_cast<size_t>(ptr) & mask));
    }

    ROME_HOST_DEVICE constexpr bool is_marked(int i) const {
      return marks().get(i);
    }

    ROME_HOST_DEVICE constexpr marked_ptr<T, bits> set_marked(int i) const {
      auto new_bitset = marks();
      new_bitset.set(i, Bool<true>{});
      marked_ptr<T, bits> new_ptr(cast(), new_bitset);
      return new_ptr;
    }

    ROME_HOST_DEVICE constexpr marked_ptr<T, bits> set_unmarked(int i) const {
      auto new_bitset = marks();
      new_bitset.set(i, Bool<false>{});
      marked_ptr<T, bits> new_ptr(cast(), new_bitset);
      return new_ptr;
    }

    ROME_HOST_DEVICE constexpr marked_ptr<T, bits> set_marks(uint8_bitset<bits> new_bitset) const {
      marked_ptr<T, bits> new_ptr(cast(), new_bitset);
      return new_ptr;
    }

    ROME_HOST_DEVICE T& operator*() {
      return *this->cast();
    }

    ROME_HOST_DEVICE const T& operator*() const {
      return *this->cast();
    }

    ROME_HOST_DEVICE T* operator->() {
      return this->cast();
    }

    ROME_HOST_DEVICE const T* operator->() const {
      return this->cast();
    }

    ROME_HOST_DEVICE constexpr bool operator==(marked_ptr<T, bits> rhs) const {
      return this->ptr == rhs.ptr;
    }

    ROME_HOST_DEVICE constexpr bool operator!=(marked_ptr<T, bits> rhs) const {
      return this->ptr != rhs.ptr;
    }

    ROME_HOST_DEVICE constexpr bool operator==(nullptr_m_t rhs) const {
      return static_cast<T*>(*this) == nullptr;
    }

    ROME_HOST_DEVICE constexpr bool operator!=(nullptr_m_t rhs) const {
      return static_cast<T*>(*this) != nullptr;
    }

    template<typename U, uint8_t N_>
    friend class atomic_marked_ptr;
  
    template<typename U, uint8_t N_, typename int_t>
    friend class vcas_marked_ptr;

    template<typename U, uint8_t N_>
    friend std::ostream& operator<<(std::ostream&, marked_ptr<U, N_>);
  private:
    T* ptr;
  };

  template<typename T, uint8_t bits>
  std::ostream& operator<<(std::ostream& os, marked_ptr<T, bits> ptr) {
    os << std::bit_cast<void*>(ptr);
    return os;
  }

  template<typename T, uint8_t N>
  class atomic_marked_ptr {
  public:

    constexpr static uint8_t bits = N;
    static_assert(bits < 64, "Need at least 1 bit for indexing");

    explicit atomic_marked_ptr(marked_ptr<T, bits> pointer) : ptr(pointer.ptr) {}
    explicit atomic_marked_ptr(T* pointer) : ptr(pointer) {}

    atomic_marked_ptr() = default;
    atomic_marked_ptr(const atomic_marked_ptr&) = default;
    atomic_marked_ptr(atomic_marked_ptr&&) = default;
    
    atomic_marked_ptr& operator=(const atomic_marked_ptr&) = default;
    atomic_marked_ptr& operator=(atomic_marked_ptr&&) = default;

    bool is_valid() const {
      return (1 << bits) <= alignof(T);
    }

    operator marked_ptr<T, bits>() const {
      return this->load();
    }
    
    constexpr bool is_marked(int i) const {
      return static_cast<marked_ptr<T, bits>>(*this).is_marked(i);
    }

    template<typename ... Arg_t>
    void store(marked_ptr<T, bits> x, Arg_t ...args) {
      ptr.store(x.ptr, args...);
    }

    template<typename ... Arg_t>
    marked_ptr<T, bits> load(Arg_t ...args) const {
      return marked_ptr<T, bits>(ptr.load(args...));
    }

    template<typename ... arg_t>
    bool compare_exchange_strong(marked_ptr<T, bits>& expected, marked_ptr<T, bits> val, arg_t ...args) {
      return ptr.compare_exchange_strong(expected.ptr, val.ptr, args...);
    }

    template<typename ... arg_t>
    bool compare_exchange_weak(marked_ptr<T, bits>& expected, marked_ptr<T, bits> val, arg_t ...args) {
      return ptr.compare_exchange_weak(expected.ptr, val.ptr, args...);
    }

    constexpr bool is_lock_free() const noexcept {
      return ptr.is_lock_free();
    }

  private:
    std::atomic<T*> ptr;
  };

}
