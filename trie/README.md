# Overview
Basic implementation of a prefix trie

Supports:
- insert
- delete (named 'remove_key' to avoid keyword clash)
- query

# Building
Basic unit tests: g++ -O2 -std=c++17 test.cpp trie.cpp -o run_trie_tests && ./run_trie_tests
Benchmark: g++ -O2 -std=c++17 trie.cpp benchmark.cpp -o bench && ./bench