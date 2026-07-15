#pragma once

#include <bitset>
#include <climits>
#include <cstdint>
#include <iostream>
#include <array>
#include <cassert>

enum BranchResult : bool {
    NOT_TAKEN = 0,
    TAKEN = 1
};

// k-bit saturation counter
template <unsigned n_bits>
class SatCounter {
    static constexpr uint8_t MAX = (1u << n_bits) - 1;
    static constexpr uint8_t TAKEN_THRESHOLD = 1u << (n_bits - 1);
    uint8_t val;
    public:
        SatCounter(uint8_t init = TAKEN_THRESHOLD - 1) : val(init) {}
        void update(BranchResult result) {
            if (result && val < MAX) { val++; } // branch taken
            else if (!result && val > 0) { val--; } // branch not taken
        }

        bool predict() const {
            return val >= TAKEN_THRESHOLD;
        }

        uint8_t get_val() const {
            return val;
        }

        void set_val(uint8_t val) { this->val = val; }
};

// static inline 

// history buffer for individual branch
template <unsigned hist_len>
// requires (hist_len <= 64)
class HistoryBuffer {
    std::bitset<hist_len> history;

    public:
        HistoryBuffer() :
            history(0x0) {}
        
        void update(BranchResult result) {
            history <<= 1;
            history.set(0, result);
        }

        uint64_t as_idx() const requires (hist_len <= 64) {
            return history.to_ullong();
        }

        uint64_t as_idx() const requires (hist_len > 64) {
            return 0;
        }

        uint16_t as_folded_idx(size_t pc, size_t mask, size_t width, uint16_t length) const {
            auto num_iter = length / width;
            auto rem = length % width;

            uint16_t start = 0;

            uint16_t idx = pc & mask;
            for (int i = 0; i < num_iter; i++) {
                idx ^= get_bit_range(start, width);
                start += width;
            }

            idx ^= get_bit_range(start, rem);

            return idx;
        }

        size_t get_bit_range(uint16_t start, size_t num_bits) const {
            size_t mask = (1ull << num_bits) - 1;
            return ((history >> start) & std::bitset<hist_len>(mask)).to_ullong();
        }

        void print() {
            std::cout << history << std::endl;
        }

        void or_other(const std::bitset<hist_len>& other) {
            history |= other;
        }
};

class Perceptron {
    int64_t threshold;
    int64_t last_sum = 0;
    bool last_pred = false;
    std::array<int8_t, 64> weights;

    int8_t bool2sgn(bool val) {
        return 2 * val - 1;
    }

    public:
        Perceptron(size_t history_length) :
                threshold(2*history_length+14),  // per Jimenez paper
                weights({0}) 
            {
                assert(history_length <= 64);
            }

        void update(BranchResult result, size_t history, size_t history_length) {
            if (last_pred != result || std::abs(last_sum) <= threshold) {
                for (size_t i = 0; i < history_length; i++) {
                    int8_t updated_val = weights[history_length - i - 1] + bool2sgn(result == (history & 0x01));
                    if (std::abs(updated_val) <= threshold && updated_val < INT8_MAX && updated_val > INT8_MIN) {
                        weights[history_length - i - 1] = updated_val;
                    }
                    history >>= 1;
                }
            }
        }

        bool predict(size_t history, size_t history_length) {
            int64_t sum = 0;

            for (size_t i = 0; i < history_length; i++) {
                sum += weights[history_length - i - 1] * bool2sgn(history & 0x01);
                history >>= 1;
            }

            last_sum = sum;
            last_pred = sum >= 0;
            return last_pred;
        }
};

// TAGE-specific

struct BaseEntry {
    SatCounter<3> _ctr = SatCounter<3>();

    bool predict() {
        return _ctr.predict();
    }
};

class TagEntry {
    SatCounter<3> _ctr;
    SatCounter<2> _u;
    uint16_t _tag;

    public:
        TagEntry() :
                _ctr(), _u(),
                _tag(0)
            {}

        bool match_tag(uint16_t tag) {
            return tag == _tag;
        }

        void update_ctr(BranchResult result) { _ctr.update(result); }
        void update_u(BranchResult result) { _u.update(result); }

        bool predict() { return _ctr.predict(); }
        uint8_t get_u() { return _u.get_val(); }

        void set_tag(uint16_t tag) { _tag = tag; }
        void set_ctr(uint8_t val) { _ctr.set_val(val); }
        void set_u(uint8_t val) { _u.set_val(val); }
};