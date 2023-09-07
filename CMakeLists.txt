cmake_minimum_required(VERSION 3.23)

project(RomeLibrary LANGUAGES CXX)

set(CXX_STANDARD 20)

find_package(fmt 8.1...<8.2 REQUIRED) # defines fmt::fmt

message(STATUS "Using fmt version ${fmt_VERSION}")

find_package(spdlog 1.9...<1.10 REQUIRED) #defines spdlog::spdlog
message(STATUS "Using spdlog version ${spdlog_VERSION}")

find_package(absl 20210324 REQUIRED) # defines absl::absl
message(STATUS "Using absl version ${absl_VERSION}")

find_package(GTest 1.11 REQUIRED) #defines GTest::gtest
message(STATUS "Using gtest version ${GTest_VERSION}")

find_package(benchmark 1.6...<1.7 REQUIRED) # google benchmark
message(STATUS "Using benchmark version ${benchmark_VERSION}")

find_package(Protobuf REQUIRED) # protobuf
message(STATUS "Using protobuf version ${Protobuf_VERSION}")

set(LOG_LEVELS "TRACE;DEBUG;INFO;WARN;ERROR;CRITICAL;OFF")

set(LOG_LEVEL "INFO" CACHE STRING "Log level options include TRACE, DEBUG, INFO, WARN, ERROR, CRITICAL, and OFF")

foreach(L ${LOG_LEVELS})
    if("${LOG_LEVEL}" STREQUAL "${L}")
        set(CORRECT_LOG_LEVEL TRUE)
    endif()
endforeach()

if(NOT DEFINED CORRECT_LOG_LEVEL)
    message(FATAL_ERROR "${LOG_LEVEL} is not a correct log level: e.g. ${LOG_LEVELS}") 
endif()

message(STATUS "Using log level ${LOG_LEVEL}")


## TODO add compiler detection for coroutine support

enable_testing()

add_subdirectory(protos)

add_library(rome SHARED
            src/rome/rdma/rdma_broker.cc  
            src/rome/rdma/rdma_device.cc  
            src/rome/rdma/rdma_memory.cc
            src/rome/metrics/stopwatch.cc
            )
target_include_directories(rome PUBLIC include)
target_compile_options(rome PUBLIC -fcoroutines)
target_compile_definitions(rome PUBLIC ROME_LOG_LEVEL=${LOG_LEVEL})
target_link_libraries(rome PUBLIC protos ibverbs rdmacm fmt::fmt)
target_link_libraries(rome PUBLIC 
absl::atomic_hook absl::errno_saver absl::log_severity absl::raw_logging_internal absl::spinlock_wait absl::config absl::dynamic_annotations absl::core_headers absl::malloc_internal absl::base_internal absl::base absl::throw_delegate absl::pretty_function absl::endian absl::exponential_biased absl::periodic_sampler absl::scoped_set_env absl::strerror absl::fast_type_id absl::algorithm absl::algorithm_container absl::cleanup_internal absl::cleanup absl::btree absl::compressed_tuple absl::fixed_array absl::inlined_vector_internal absl::inlined_vector absl::counting_allocator absl::flat_hash_map absl::flat_hash_set absl::node_hash_map absl::node_hash_set absl::container_memory absl::hash_function_defaults absl::hash_policy_traits absl::hashtablez_sampler absl::hashtable_debug absl::hashtable_debug_hooks absl::have_sse absl::node_hash_policy absl::raw_hash_map absl::container_common absl::raw_hash_set absl::layout absl::stacktrace absl::symbolize absl::examine_stack absl::failure_signal_handler absl::debugging_internal absl::demangle_internal absl::leak_check absl::leak_check_disable absl::debugging absl::flags_path_util absl::flags_program_name absl::flags_config absl::flags_marshalling absl::flags_commandlineflag_internal absl::flags_commandlineflag absl::flags_private_handle_accessor absl::flags_reflection absl::flags_internal absl::flags absl::flags_usage_internal absl::flags_usage absl::flags_parse absl::bind_front absl::function_ref absl::hash absl::city absl::wyhash absl::memory absl::type_traits absl::meta absl::bits absl::int128 absl::numeric absl::numeric_representation absl::random_random absl::random_bit_gen_ref absl::random_internal_mock_helpers absl::random_distributions absl::random_seed_gen_exception absl::random_seed_sequences absl::random_internal_traits absl::random_internal_distribution_caller absl::random_internal_fast_uniform_bits absl::random_internal_seed_material absl::random_internal_pool_urbg absl::random_internal_salted_seed_seq absl::random_internal_iostream_state_saver absl::random_internal_generate_real absl::random_internal_wide_multiply absl::random_internal_fastmath absl::random_internal_nonsecure_base absl::random_internal_pcg_engine absl::random_internal_randen_engine absl::random_internal_platform absl::random_internal_randen absl::random_internal_randen_slow absl::random_internal_randen_hwaes absl::random_internal_randen_hwaes_impl absl::random_internal_distribution_test_util absl::random_internal_uniform_helper absl::status absl::statusor absl::strings absl::strings_internal absl::str_format absl::str_format_internal absl::cord absl::graphcycles_internal absl::kernel_timeout_internal absl::synchronization absl::time absl::civil_time absl::time_zone absl::any absl::bad_any_cast absl::bad_any_cast_impl absl::span absl::optional absl::bad_optional_access absl::bad_variant_access absl::variant absl::compare absl::utility)

function(add_test_executable TARGET SOURCE)
    set(options DISABLE_TEST)
    set(oneValueArgs "")
    set(multiValueArgs LIBS)

    cmake_parse_arguments(_ "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

    add_executable(${TARGET} ${SOURCE})
   
    foreach(NAME "${TARGET}")
        target_link_libraries(${NAME} PRIVATE rome GTest::gtest GTest::gtest_main GTest::gmock ${__LIBS})
    endforeach()
    
    if(NOT ${__DISABLE_TEST})
        add_test(NAME ${TARGET} COMMAND ${TARGET})
    endif()
endfunction()

add_subdirectory(tests)


