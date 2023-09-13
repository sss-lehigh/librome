#pragma once
#include <cstdint>

#include <absl/status/statusor.h>

#include "metric.h"

namespace rome::metrics {

template <typename T>
class Counter : public Metric, Accumulator<Counter<T>> {
 public:
  ~Counter() = default;
  explicit Counter(std::string_view name) : Metric(name), counter_(0) {}
  Counter(std::string_view name, T counter) : Metric(name), counter_(counter) {}

  T GetCounter() const { return counter_; }

  // Assignment.
  Counter& operator=(T c);

  // Addition and subtraction.
  Counter& operator+=(T rhs);
  Counter& operator-=(T rhs);

  // Pre- and postfix increment.
  Counter& operator++();
  Counter operator++(int);

  // Pre- and postfix decrement.
  Counter& operator--();
  Counter operator--(int);

  // Equlity operator.
  bool operator==(T c) const;
  bool operator==(const Counter& c) const;

  // For printing.
  std::string ToString() override;

  MetricProto ToProto() override;

  absl::Status Accumulate(const absl::StatusOr<Counter<T>>& other) override;

 private:
  T counter_;
};

// ---------------|
// IMPLEMENTATION |
// ---------------|

template <typename T>
Counter<T>& Counter<T>::operator=(T c) {
  counter_ = c;
  return *this;
}

template <typename T>
Counter<T>& Counter<T>::operator+=(T rhs) {
  counter_ += rhs;
  return *this;
}

template <typename T>
Counter<T>& Counter<T>::operator-=(T rhs) {
  counter_ -= rhs;
  return *this;
}

template <typename T>
Counter<T>& Counter<T>::operator++() {
  ++counter_;
  return *this;
}

template <typename T>
Counter<T> Counter<T>::operator++(int) {
  Counter old = *this;
  operator++();
  return old;
}

template <typename T>
Counter<T>& Counter<T>::operator--() {
  --counter_;
  return *this;
}

template <typename T>
Counter<T> Counter<T>::operator--(int) {
  Counter old = *this;
  operator--();
  return old;
}

template <typename T>
bool Counter<T>::operator==(T c) const {
  return counter_ == c;
}

template <typename T>
bool Counter<T>::operator==(const Counter<T>& c) const {
  return name_ == c.name_ && operator==(c.counter_);
}

template <typename T>
std::string Counter<T>::ToString() {
  return "count: " + std::to_string(counter_) + "";
}

template <typename T>
MetricProto Counter<T>::ToProto() {
  MetricProto proto;
  proto.set_name(name_);
  proto.mutable_counter()->set_count(counter_);
  return proto;
}

template <typename T>
absl::Status Counter<T>::Accumulate(const absl::StatusOr<Counter<T>>& other) {
  if (!other.ok()) return other.status();
  if (!(name_ == other.value().name_)) {
    return absl::FailedPreconditionError("Counter name does not match: " +
                                         other.value().name_);
  }
  operator+=(other.value().counter_);
  return absl::OkStatus();
}

}  // namespace rome::metrics
