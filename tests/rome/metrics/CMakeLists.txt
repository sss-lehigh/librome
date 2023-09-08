add_test_executable(metric_test metric_test.cc)
message(WARNING "Disable stop watch test for CI/CD")
add_test_executable(stopwatch_test stopwatch_test.cc DISABLE_TEST)
