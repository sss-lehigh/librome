#pragma once

#include "absl/status/status.h"
#include "request_handler.h"
#include "rome/util/clocks.h"
#include "rome/util/coroutine.h"
#include "rome/util/thread_pool.h"

using ::util::SystemClock;
using ::util::ThreadPool;

// A std::coroutine consists of three parts, the coroutine itself, an Awaitable
// object, and a Promise object. The coroutine is anything that uses the `co_*`
// unary operators to yield execution. The `co_await` operator passes a handle
// to the current coroutine to the Awaitable so that execution can be resumed at
// some future point (not necessarily by the Awaitable).

class AwaitableRequestHandler : public RequestHandler {
 public:
  static std::unique_ptr<AwaitableRequestHandler> Create(std::string_view id,
                                                         ThreadPool* pool) {
    return std::unique_ptr<AwaitableRequestHandler>(
        new AwaitableRequestHandler(id, pool));
  }

  // Called whenever the awaitable `co_await`ed to short circuit calling
  // `await_suspend`. If this returns true then the calling coroutine is resumed
  // immediately.
  bool await_ready() { return step_ == Step::kDone; }

  // This method is called whenever the awaitable is co_await'ed. A handle to
  // the calling coroutine is passed as `handle`, which allows this awaitable
  // to resume execution after some work has been done. Here, we are simply
  // rolling the dice and deciding whether the work is complete. In a real
  // scenario, we could replace the following with more complex logic, like
  // checking whether or not a result is ready.
  void await_suspend(util::coroutine_handle<> caller) {
    auto status = pool_->Enqueue(
        {id_ + ":" + ToString(step_), [this, caller]() {
           Handle();
           // Resume the caller.
           auto status = pool_->Enqueue({id_ + ":resume", caller});
         }});
  }

  // Determines the return value of `co_await`. In this case, we wait for the
  // handler to complete, which is whenever the internal step is `kDone`.
  bool await_resume() { return step_ == Step::kDone; }

 private:
  AwaitableRequestHandler(std::string_view id, ThreadPool* pool)
      : RequestHandler(id, pool) {}
};

struct SyncTask {
 public:
  // NOTE: The a boolean in the promise to indicate that the coroutine finished
  // is a broken pattern. A problem arises if there is a race between the thread
  // reading the boolean value the destruction of the `promise_type` object.
  // Instead, the caller needs to pass a reference to the variable to use via
  // the coroutine invocation.
  struct promise_type {
    // A `promise_type` object is constructed and stored locally whenever a
    // coroutine in instantiated. If a constructor with arguments matching those
    // of the coroutine exists, then that constructor is called.
    promise_type([[maybe_unused]] std::string_view id,
                 [[maybe_unused]] ThreadPool* pool, bool* done)
        : done(done) {
      *done = false;
    }
    ~promise_type() { *done = true; }

    SyncTask get_return_object() { return SyncTask(done); }

    // Since we return `suspend_never` the coroutine is immediately executed.
    // Meaning, that the first call will suspend at `co_await`. If this was
    // instead `suspend_always`, then the coroutine would not run at first but
    // execution would be returned to the caller. The latter is helpful for
    // lazily evaluated coroutines, whereas the first is for eager ones.
    util::suspend_never initial_suspend() { return {}; }
    util::suspend_never final_suspend() noexcept { return {}; }
    void return_void() {}
    void unhandled_exception() {}
    bool* done;
  };

  SyncTask(const SyncTask& other) : done_(other.done_) {}

  void Wait() const {
    while (!(*done_))
      ;
  }

  absl::Status WaitWithTimeout(std::chrono::nanoseconds durr) {
    auto start = SystemClock::now();
    while (!(*done_) && (SystemClock::now() - start < durr))
      ;
    if (!(*done_)) {
      return absl::DeadlineExceededError("Timed out waiting for task");
    }
    return absl::OkStatus();
  }

 private:
  explicit SyncTask(bool* done) : done_(done) {}
  bool* done_;
};

inline SyncTask start_coroutine(std::string_view id, ThreadPool* pool,
                                bool* done) {
  auto handler = AwaitableRequestHandler::Create(id, pool);
  while (!co_await *handler)
    ;
}

inline void Run(int num_coros, ThreadPool* pool) {
  std::vector<std::string> ids;
  for (int i = 0; i < num_coros; ++i) {
    ids.push_back("client" + std::to_string(i));
  }

  std::vector<SyncTask> tasks;
  bool done[num_coros];
  for (int i = 0; i < num_coros; ++i) {
    done[i] = false;
    tasks.emplace_back(start_coroutine(ids[i], pool, &done[i]));
  }

  for (const auto& t : tasks) {
    t.Wait();
  }
}