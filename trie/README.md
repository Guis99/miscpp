# Overview
Basic implementation of a prefix trie

Supports:
- insert
- delete (named 'remove_key' to avoid keyword clash)
    - Does not prune nodes, so memory is not recovered on delete. Potential future work
- query


# Running the code
```bash
cmake -B build # Configures three targets: static lib for export and two executables for validation
make -C build # Linux and macOS
```

Basic unit tests: ```./build/trie_test```

Benchmark: ```./build/trie_bench```

# Performance characteristics
I wanted to get an idea of how different approaches to memory management would affect the runtime of a tree-like data structure, so I implemented four variants:

- Naive: New heap allocation for every node
- Naive with shared_ptr: Same as above but with automatic memory management
- Bump allocator: Pre-allocate memory pool and construct new nodes inside it
- Vector: Pre-allocate std::vector as my memory pool

My predicted ranking, from fastest to slowest:

| Ranking | Implementation | Reason |
|---|---|---|
| 1 | Vector | Pre-allocated memory. Mature code written by experts, so will likely have optimizations I didn't think of |
| 2 | Bump allocator | Pre-allocated memory avoids overhead of new allocation |
| 3 | Naive | Non-trivial overhead of new heap allocations |
| 4 | Naive with shared_ptr | Additional overhead from shared_ptr |

## Data

The below data are generated from runs on a Macbook M4 Pro:

### Insert (ms)

| N | Raw pointer | SharedPtr | Vec | Arena |
|---|---|---|---|---|
| 100 | 0.032 | 0.056 | 0.003 | 0.026 |
| 1,000 | 0.287 | 0.414 | 0.024 | 0.159 |
| 10,000 | 2.230 | 3.315 | 0.184 | 1.211 |
| 100,000 | 19.054 | 27.937 | 1.441 | 13.617 |
| 500,000 | 112.458 | 178.480 | 13.742 | 54.196 |
| 1,000,000 | 174.305 | 339.130 | 30.521 | 111.922 |

### Query (ms)

| N | Raw pointer | SharedPtr | Vec | Arena |
|---|---|---|---|---|
| 100 | 0.002 | 0.005 | 0.002 | 0.002 |
| 1,000 | 0.024 | 0.049 | 0.014 | 0.017 |
| 10,000 | 0.254 | 0.648 | 0.124 | 0.190 |
| 100,000 | 4.633 | 9.850 | 1.097 | 4.136 |
| 500,000 | 25.187 | 63.786 | 6.867 | 26.298 |
| 1,000,000 | 47.376 | 131.003 | 16.018 | 39.581 |

## Note

For full transparency, the std::vector implementation contains another optimization that slightly skews results. The default code path both pre-allocates memory for each node AND pre-constructs each node. 

You can turn it off by running
```bash
rm -rf build
cmake -B build -DOPTIMIZE_VEC=OFF
make -C build
```

The performance of the vector-based trie will drop by a factor of ~2-3x. Still outperforms the other implementations but nowhere near as dominant.

# Challenges

- Coming up with code architecture for TrieNode and Trie classes was more difficult than anticipated and gave me a healthy respect for C++ classes.
- Template programming: learned of and used [CRTP](https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern) for the first time
- Implementing the bump allocator was full of footguns. Mismatched pointer strides, void*, object alignment, etc. all required careful handling. [This blog post](https://medium.com/@sgn00/high-performance-memory-management-arena-allocators-c685c81ee338) was a great reference for this exercise.