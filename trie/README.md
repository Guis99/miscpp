# Overview
Basic implementation of a prefix trie

Supports:
- insert
- delete (named 'remove_key' to avoid keyword clash)
- query

# Building
```bash
cmake -B build
make -C build
```

Basic unit tests: ```./build/trie_test```

Benchmark: ```./build/trie_bench```