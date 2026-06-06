cmake_minimum_required(VERSION 3.20)
project(arena CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_TESTS     "Build test executable"      OFF)
option(BUILD_BENCHMARK "Build benchmark executable" OFF)

# export: static library
add_library(arena_lib STATIC arena.cpp)
target_include_directories(arena_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# test: executable depending on arena_lib
if(BUILD_TESTS)
    add_executable(arena_test test.cpp)
    target_link_libraries(arena_test PRIVATE arena_lib)
    target_include_directories(arena_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
endif()

# benchmark: executable depending on arena_lib
if(BUILD_BENCHMARK)
    add_executable(arena_bench benchmark.cpp)
    target_link_libraries(arena_bench PRIVATE arena_lib)
    target_include_directories(arena_bench PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
endif()
