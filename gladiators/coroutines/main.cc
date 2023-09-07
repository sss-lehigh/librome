#include <chrono>
#include <deque>
#include <iostream>
#include <mutex>
#include <random>
#include <ratio>
#include <string>
#include <thread>
#include <type_traits>
#include <variant>
#include <vector>

#include "absl/base/thread_annotations.h"
#include "absl/status/status.h"
#include "rome/util/clocks.h"
#include "rome/util/thread_pool.h"

#ifdef USE_BOOST_COROUTINES
#include "boost_coroutines.h"
#else
#include "c++20_coroutines.h"
#endif  // USE_BOOST_COROUTINES

// http://www.cs.tufts.edu/comp/250RTS/archive/roberto-ierusalimschy/revisiting-coroutines.pdf

using ::util::ThreadPool;

constexpr int kNumCoros = 1000;

int main(int argc, char* argv[]) {
  ThreadPool pool;
  Run(kNumCoros, &pool);
  pool.Drain();
}