#include "trie.h"
#include <chrono>
#include <iomanip>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <vector>

using Clock = std::chrono::high_resolution_clock;
using Ms    = std::chrono::duration<double, std::milli>;

static std::vector<std::string> gen_words(size_t n, unsigned seed = 42) {
    std::mt19937 rng(seed);
    std::uniform_int_distribution<> len_dist(4, 10);
    std::uniform_int_distribution<> char_dist(0, 25);

    std::vector<std::string> words(n);
    for (auto& w : words) {
        size_t len = len_dist(rng);
        w.resize(len);
        for (auto& c : w) c = 'a' + char_dist(rng);
    }
    return words;
}

int main() {
    const std::vector<size_t> sizes = {100, 1'000, 10'000, 100'000, 500'000, 1'000'000};

    bool query_sink = false;

    struct Row { double raw_i, sp_i, vec_i, arena_i; double raw_q, sp_q, vec_q, arena_q; };
    std::vector<Row> rows;
    rows.reserve(sizes.size());

    for (size_t n : sizes) {
        const auto words = gen_words(n);
        Row r{};

        { // TrieBasic<TrieNodeRaw*>
            TrieBasic<TrieNodeRaw*> trie;
            auto t0 = Clock::now();
            for (const auto& w : words) trie.insert(w);
            auto t1 = Clock::now();
            for (const auto& w : words) query_sink ^= trie.query(w);
            auto t2 = Clock::now();
            r.raw_i = Ms(t1 - t0).count();
            r.raw_q = Ms(t2 - t1).count();
        }

        { // TrieBasic<shared_ptr<TrieNodeBasic>>
            TrieBasic<std::shared_ptr<TrieNodeBasic>> trie;
            auto t0 = Clock::now();
            for (const auto& w : words) trie.insert(w);
            auto t1 = Clock::now();
            for (const auto& w : words) query_sink ^= trie.query(w);
            auto t2 = Clock::now();
            r.sp_i = Ms(t1 - t0).count();
            r.sp_q = Ms(t2 - t1).count();
        }

        { // TrieVec
            TrieVec trie(n * 10);
            auto t0 = Clock::now();
            for (const auto& w : words) trie.insert(w);
            auto t1 = Clock::now();
            for (const auto& w : words) query_sink ^= trie.query(w);
            auto t2 = Clock::now();
            r.vec_i = Ms(t1 - t0).count();
            r.vec_q = Ms(t2 - t1).count();
        }

        { // TrieArena
            size_t block = sizeof(TrieNodeRaw) * n * 10;
            TrieArena trie(block, 1);
            auto t0 = Clock::now();
            for (const auto& w : words) trie.insert(w);
            auto t1 = Clock::now();
            for (const auto& w : words) query_sink ^= trie.query(w);
            auto t2 = Clock::now();
            r.arena_i = Ms(t1 - t0).count();
            r.arena_q = Ms(t2 - t1).count();
        }

        rows.push_back(r);
    }

    constexpr int W = 18;
    auto print_header = [&](const char* c1, const char* c2, const char* c3, const char* c4) {
        std::cout << std::left
                  << std::setw(12) << "N"
                  << std::setw(W) << c1
                  << std::setw(W) << c2
                  << std::setw(W) << c3
                  << std::setw(W) << c4
                  << "\n" << std::string(12 + W*4, '-') << "\n";
    };

    std::cout << "\n=== INSERT (ms) ===\n";
    print_header("Raw* insert", "SharedPtr insert", "Vec insert", "Arena insert");
    for (size_t i = 0; i < sizes.size(); ++i) {
        const auto& r = rows[i];
        std::cout << std::fixed << std::setprecision(3) << std::left
                  << std::setw(12) << sizes[i]
                  << std::setw(W) << r.raw_i
                  << std::setw(W) << r.sp_i
                  << std::setw(W) << r.vec_i
                  << std::setw(W) << r.arena_i
                  << "\n";
    }

    std::cout << "\n=== QUERY (ms) ===\n";
    print_header("Raw* query", "SharedPtr query", "Vec query", "Arena query");
    for (size_t i = 0; i < sizes.size(); ++i) {
        const auto& r = rows[i];
        std::cout << std::fixed << std::setprecision(3) << std::left
                  << std::setw(12) << sizes[i]
                  << std::setw(W) << r.raw_q
                  << std::setw(W) << r.sp_q
                  << std::setw(W) << r.vec_q
                  << std::setw(W) << r.arena_q
                  << "\n";
    }

    if (query_sink) std::cout << "\n(query verification: some words not found)\n";
}
