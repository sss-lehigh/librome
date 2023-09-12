cmake_minimum_required(VERSION 3.18)

project(Rome LANGUAGES CXX VERSION 0.0.0)

#######################################################################################
# configuration
#######################################################################################

option(HAVE_RDMA_CARD OFF "Set HAVE_RDMA_CARD if you have a mlnx card to test with")
option(USE_CUDA OFF "Set USE_CUDA if you want to run tests for NVCC")
set(LOG_LEVEL "INFO" CACHE STRING "Log level options include TRACE, DEBUG, INFO, WARN, ERROR, CRITICAL, and OFF")
set(CXX_STANDARD 20 CACHE STRING "Uses C++20 or 23")

#######################################################################################
# setup module path and standard
#######################################################################################

# to include anything in cmake directory
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(utils)

set(CMAKE_CXX_STANDARD ${CXX_STANDARD})
if(${USE_CUDA})
    enable_language(CUDA)
    set(CMAKE_CUDA_STANDARD ${CXX_STANDARD})
    add_compile_options($<$<COMPILE_LANGUAGE:CUDA>:--expt-relaxed-constexpr>)
    message(STATUS "Using host compiler - ${CMAKE_CUDA_HOST_COMPILER}")
endif()

#######################################################################################
# find packages
#######################################################################################

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

find_package(Coroutines REQUIRED) # defines std::coroutines for linking

find_package(RDMA REQUIRED) # defines rdma::ibverbs and rdma::cm for linking



set(LOG_LEVELS "TRACE;DEBUG;INFO;WARN;ERROR;CRITICAL;OFF")


foreach(L ${LOG_LEVELS})
    if("${LOG_LEVEL}" STREQUAL "${L}")
        set(CORRECT_LOG_LEVEL TRUE)
    endif()
endforeach()

if(NOT DEFINED CORRECT_LOG_LEVEL)
    message(FATAL_ERROR "${LOG_LEVEL} is not a correct log level: e.g. ${LOG_LEVELS}") 
endif()

message(STATUS "Using log level ${LOG_LEVEL}")

enable_testing()

add_subdirectory(protos)

add_library(rome SHARED
            src/rome/rdma/rdma_broker.cc  
            src/rome/rdma/rdma_device.cc  
            src/rome/rdma/rdma_memory.cc
            src/rome/metrics/stopwatch.cc
            src/rome/util/thread_pool.cc)
add_library(rome::rome ALIAS rome)
target_include_directories(rome PUBLIC 
                           $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                           $<INSTALL_INTERFACE:include>)
target_compile_definitions(rome PUBLIC ROME_LOG_LEVEL=${LOG_LEVEL})
target_link_libraries(rome PUBLIC rome::protos rdma::ibverbs rdma::cm fmt::fmt std::coroutines)
target_link_libraries(rome PUBLIC absl::status absl::statusor absl::synchronization)

add_subdirectory(tests)
add_subdirectory(gladiators)

#######################################################################################
# install rome
#######################################################################################

install(TARGETS rome protos EXPORT romeTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/ DESTINATION include) 
# install the include dir
install(FILES ${PROJECT_SOURCE_DIR}/protos/colosseum.proto DESTINATION protos)
install(FILES ${PROJECT_SOURCE_DIR}/protos/testutil.proto DESTINATION protos)
install(FILES ${PROJECT_SOURCE_DIR}/protos/metrics.proto DESTINATION protos)
# install protos for reuse

install(EXPORT romeTargets
        FILE romeTargets.cmake
        NAMESPACE rome::
        DESTINATION lib/cmake/rome)

install(FILES ${PROJECT_SOURCE_DIR}/share/romeConfig.cmake DESTINATION lib/cmake/rome)

install(FILES ${PROJECT_SOURCE_DIR}/cmake/FindRDMA.cmake DESTINATION lib/cmake)
install(FILES ${PROJECT_SOURCE_DIR}/cmake/FindCoroutines.cmake DESTINATION lib/cmake)
