cmake_minimum_required(VERSION 3.20)
project(trie CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -O2")

option(BUILD_TESTS     "Build test executable"      ON)
option(BUILD_BENCHMARK "Build benchmark executable" ON)

set(ARENA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../arena)

# export: static library
add_library(trie_lib STATIC
    trie.cpp
    ${ARENA_DIR}/arena.cpp
)
target_include_directories(trie_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# test: executable depending on trie_lib
if(BUILD_TESTS)
    add_executable(trie_test test.cpp)
    target_link_libraries(trie_test PRIVATE trie_lib)
    target_include_directories(trie_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
endif()

# benchmark: executable depending on trie_lib
if(BUILD_BENCHMARK)
    add_executable(trie_bench benchmark.cpp)
    target_link_libraries(trie_bench PRIVATE trie_lib)
    target_include_directories(trie_bench PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
endif()
