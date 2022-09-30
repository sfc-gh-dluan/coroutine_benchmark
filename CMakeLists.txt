cmake_minimum_required(VERSION 3.10)

project(RingTest)

set(CMAKE_C_COMPILER /usr/bin/clang)
set(CMAKE_CXX_COMPILER /usr/bin/clang++)

# Set C++ standard and enable coroutine
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_CXX_FLAGS "-stdlib=libc++ -fcoroutines-ts -O3")

# Find google benchmark library
find_package(benchmark REQUIRED)

add_executable(RingBenchmark ring_benchmark.cpp)
target_link_libraries(RingBenchmark benchmark::benchmark)
