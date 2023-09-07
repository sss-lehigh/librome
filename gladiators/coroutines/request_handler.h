#pragma once

#include <chrono>
#include <string>
#include <thread>

#include "rome/logging/logging.h"
#include "rome/util/thread_pool.h"

using ::util::ThreadPool;

class RequestHandler {
 public:
  RequestHandler() = default;

 protected:
  RequestHandler(std::string_view id, ThreadPool* pool)
      : id_(id), pool_(pool), step_(Step::kFirst) {}

  enum class Step {
    kFirst,
    kSecond,
    kDone,
  };

  static std::string ToString(Step s) {
    switch (s) {
      case Step::kFirst:
        return "Step::kFirst";
      case Step::kSecond:
        return "Step::kSecond";
      case Step::kDone:
        return "Step::kDone";
      default:
        ROME_FATAL("Unknown Step");
    }
  }

  void Handle() {
    switch (step_) {
      case Step::kFirst:
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        step_ = Step::kSecond;
        break;
      case Step::kSecond:
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        step_ = Step::kDone;
        break;
      case Step::kDone:
        break;
    }
  }

  const std::string id_;
  ThreadPool* pool_;
  Step step_;
};