add_test_executable(coroutine_test coroutine_test.cc)
add_test_executable(duration_util_test duration_util_test.cc)
add_test_executable(status_util_test status_util_test.cc)
add_test_executable(variant_util_test variant_util_test.cc)

if(${USE_CUDA})
    add_test_executable(coroutine_test_cuda coroutine_test_cuda.cu)
endif()
