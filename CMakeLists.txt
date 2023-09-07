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

##
#        ":trace": ["ROME_LOG_LEVEL=TRACE"],
#        ":debug": ["ROME_LOG_LEVEL=DEBUG"],
#        ":dbg": ["ROME_LOG_LEVEL=DEBUG"],
#        ":info": ["ROME_LOG_LEVEL=INFO"],
#        ":fastbuild": ["ROME_LOG_LEVEL=INFO"],
#        ":opt": ["ROME_LOG_LEVEL=INFO"],
#        ":warn": ["ROME_LOG_LEVEL=WARN"],
#        ":error": ["ROME_LOG_LEVEL=ERROR"],
#        ":critical": ["ROME_LOG_LEVEL=CRITICAL"],
#        ":off": ["ROME_LOG_LEVEL=OFF"],
#        "//conditions:default": ["ROME_LOG_LEVEL=INFO"],
#
##

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

## TODO add switch to change log level
add_compile_definitions(ROME_LOG_LEVEL=${LOG_LEVEL})

## TODO add compiler detection for coroutine support
add_compile_options(-fcoroutines)

enable_testing()

add_subdirectory(rome)

