#include "trie.h"
#include <chrono>
#include <iomanip>
#include <iostream>
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
    const std::vector<size_t> sizes = {100, 1'000, 10'000, 100'000, 399'000,500'000,1'000'000};

    std::cout << std::left
              << std::setw(12) << "N"
              << std::setw(22) << "Basic insert (ms)"
              << std::setw(22) << "Basic query (ms)"
              << std::setw(22) << "Vec insert (ms)"
              << std::setw(22) << "Vec query (ms)"
              << "\n"
              << std::string(88, '-') << "\n";

    for (size_t n : sizes) {
        const auto words = gen_words(n);

        double bi, bq, vi, vq;

        {
            TrieBasic trie;
            auto t0 = Clock::now();
            for (const auto& w : words) trie.insert(w);
            auto t1 = Clock::now();
            for (const auto& w : words) trie.query(w);
            auto t2 = Clock::now();
            bi = Ms(t1 - t0).count();
            bq = Ms(t2 - t1).count();
        }

        {
            // resize pre-zeroes the arena (construction cost excluded from timing);
            // n*4 covers typical node count after prefix sharing on random input
            size_t alloc = n * 10;
            TrieVec trie(alloc);
            auto t0 = Clock::now();
            for (const auto& w : words) trie.insert(w);
            auto t1 = Clock::now();
            for (const auto& w : words) trie.query(w);
            auto t2 = Clock::now();
            vi = Ms(t1 - t0).count();
            vq = Ms(t2 - t1).count();
        }

        std::cout << std::fixed << std::setprecision(3) << std::left
                  << std::setw(12) << n
                  << std::setw(22) << bi
                  << std::setw(22) << bq
                  << std::setw(22) << vi
                  << std::setw(22) << vq
                  << "\n";
    }
}
