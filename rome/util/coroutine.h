#pragma once

#if defined(__clang__)
#include <experimental/coroutine>
namespace rome::coroutine {
  using namespace std::experimental;
}
#elif defined(__GNUC__) || defined(__GNUG__)
#include <coroutine>
namespace rome::coroutine {
  using namespace std;
}
#else
#error "Unknown compiler"
#endif