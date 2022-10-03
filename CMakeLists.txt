cmake_minimum_required(VERSION 3.10)

project(RingTest)

# set(CMAKE_C_COMPILER /usr/bin/clang)
# set(CMAKE_CXX_COMPILER /usr/bin/clang++)

# Set C++ standard and enable coroutine
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_CXX_FLAGS "-stdlib=libc++ -fcoroutines-ts -O3")


list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(USE_JEMALLOC ON)
include(Jemalloc)

# Find google benchmark library
# find_package(benchmark REQUIRED)


# Google Benchmark

# include the configurations from benchmark.cmake
configure_file(benchmark.cmake googlebenchmark-download/CMakeLists.txt)
# executing the configuration step
execute_process(
    COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
    RESULT_VARIABLE results
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googlebenchmark-download
)
# checking if the configuration step passed
if(results)
    message(FATAL_ERROR "Configuration step for Benchmark has Failed. ${results}")
endif()
# executing the build step
execute_process(
    COMMAND ${CMAKE_COMMAND} --build . --config Release
    RESULT_VARIABLE results
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googlebenchmark-download
)
# checking if the build step passed
if(results)
  message(FATAL_ERROR "Build step for Benchmark has Failed. ${results}")
endif()
set(BENCHMARK_ENABLE_TESTING OFF)
add_subdirectory(
  ${CMAKE_CURRENT_BINARY_DIR}/googlebenchmark-src
  ${CMAKE_CURRENT_BINARY_DIR}/googlebenchmark-build
  EXCLUDE_FROM_ALL
)

add_executable(RingBenchmark ring_benchmark.cpp)
target_link_libraries(RingBenchmark benchmark::benchmark jemalloc)
