cmake_minimum_required(VERSION 3.23)

project(RomeLibrary LANGUAGES CXX)

set(CXX_STANDARD 20)

find_package(fmt REQUIRED) # defines fmt::fmt
find_package(spdlog REQUIRED) #defines spdlog::spdlog
find_package(absl REQUIRED) # defines absl::absl
find_package(GTest REQUIRED) #defines GTest::gtest
find_package(benchmark REQUIRED) # google benchmark

## TODO add switch to change log level
add_compile_definitions(ROME_LOG_LEVEL=OFF)

## TODO add compiler detection for coroutine support
add_compile_options(-fcoroutines)

enable_testing()

add_subdirectory(rome)

