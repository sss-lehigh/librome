#pragma once

#include <memory>

#define TRACE 0
#define DEBUG 1
#define INFO 2
#define WARN 3
#define ERROR 4
#define CRITICAL 5
#define OFF 6

#ifndef ROME_LOG_LEVEL
#define ROME_LOG_LEVEL DEBUG
#endif

//! Must be set before including `spdlog/spdlog.h`
#define SPDLOG_ACTIVE_LEVEL ROME_LOG_LEVEL

#include "spdlog/async.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

inline void __rome_init_log__() {
  ::spdlog::init_thread_pool(8192, 1);
  static_assert(ROME_LOG_LEVEL < spdlog::level::level_enum::n_levels,
                "Invalid logging level.");
  std::shared_ptr<spdlog::logger> __rome_log__ =
      ::spdlog::create_async<::spdlog::sinks::stdout_color_sink_mt>("rome");
  __rome_log__->set_level(
      static_cast<spdlog::level::level_enum>(ROME_LOG_LEVEL));
  __rome_log__->set_pattern("[%Y-%m-%d %H:%M%S thread:%t] [%^%l%$] [%@] %v");
  ::spdlog::set_default_logger(std::move(__rome_log__));
}

#if ROME_LOG_LEVEL == OFF
#define ROME_INIT_LOG []() {}
#else
#define ROME_INIT_LOG __rome_init_log__
#endif

#if ROME_LOG_LEVEL == OFF
#define ROME_DEINIT_LOG() ((void)0)
#else
#define ROME_DEINIT_LOG [&]() { ::spdlog::drop("rome"); }
#endif

#if ROME_LOG_LEVEL <= TRACE
#define ROME_TRACE(...) SPDLOG_TRACE(__VA_ARGS__)
#else
#define ROME_TRACE(...) ((void)0)
#endif

#if ROME_LOG_LEVEL <= DEBUG
#define ROME_DEBUG(...) SPDLOG_DEBUG(__VA_ARGS__)
#else
#define ROME_DEBUG(...) ((void)0)
#endif

#if ROME_LOG_LEVEL <= INFO
#define ROME_INFO(...) SPDLOG_INFO(__VA_ARGS__)
#else
#define ROME_INFO(...) ((void)0)
#endif

#if ROME_LOG_LEVEL <= WARN
#define ROME_WARN(...) SPDLOG_WARN(__VA_ARGS__)
#else
#define ROME_WARN(...) ((void)0)
#endif

#if ROME_LOG_LEVEL <= ERROR
#define ROME_ERROR(...) SPDLOG_ERROR(__VA_ARGS__)
#else
#define ROME_ERROR(...) ((void)0)
#endif

#if ROME_LOG_LEVEL <= CRITICAL
#define ROME_CRITICAL(...) SPDLOG_CRITICAL(__VA_ARGS__)
#else
#define ROME_CRITICAL(...) ((void)0)
#endif

#if ROME_LOG_LEVEL != OFF
#define ROME_FATAL(...)          \
  SPDLOG_CRITICAL(__VA_ARGS__);  \
  SPDLOG_CRITICAL("Exiting..."); \
  exit(1);
#endif

#define ROME_RETURN(x) [=]() { return x; }
#define ROME_RETURN_FALSE []() { return false; }
#define ROME_RETURN_TRUE []() { return true; }
#define ROME_NOOP []() {}
#define ROME_ABORT []() { std::exit(1); }

// General checks
#define ROME_CHECK_QUIET(ret_func, check) \
  if (!(check)) {                         \
    return ret_func();                    \
  }
#define ROME_CHECK(ret_func, check, ...) \
  if (!(check)) {                        \
    SPDLOG_ERROR(__VA_ARGS__);           \
    return ret_func();                   \
  }
#define ROME_CHECK_OK(ret_func, status, ...) \
  if (!(status.ok())) {                      \
    SPDLOG_ERROR(__VA_ARGS__);               \
    return ret_funct();                      \
  }
#define ROME_ASSERT_OK(status)      \
  if (!(status.ok())) {             \
    SPDLOG_ERROR(status.message()); \
    exit(1);                        \
  }

// Specific checks for debugging. Can be turned off by commenting out
// `ROME_DO_DEBUG_CHECKS` in config.h
#ifndef ROME_NDEBUG
#define ROME_CHECK_DEBUG(ret_func, check, ...) \
  if (!(check)) {                              \
    SPDLOG_ERROR(__VA_ARGS__);                 \
    return ret_func();                         \
  }
#define ROME_ASSERT_DEBUG(func, ...) \
  if (!(func)) {                     \
    SPDLOG_ERROR(__VA_ARGS__);       \
    exit(1);                         \
  }
#else
#define ROME_CHECK_DEBUG(...) ((void)0)
#endif
