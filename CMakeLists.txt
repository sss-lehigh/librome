cmake_minimum_required(VERSION 3.23)

project(Rome LANGUAGES CXX)

set(CXX_STANDARD 20 CACHE STRING "Uses C++20 or 23")

set(CMAKE_CXX_STANDARD ${CXX_STANDARD})

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
            src/rome/metrics/stopwatch.cc)
add_library(rome::rome ALIAS rome)
target_include_directories(rome PUBLIC include)
target_compile_options(rome PUBLIC -fcoroutines)
target_compile_definitions(rome PUBLIC ROME_LOG_LEVEL=${LOG_LEVEL})
target_link_libraries(rome PUBLIC rome::protos ibverbs rdmacm fmt::fmt)
target_link_libraries(rome PUBLIC absl::status absl::statusor absl::synchronization)

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


