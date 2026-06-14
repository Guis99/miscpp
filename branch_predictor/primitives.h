#pragma once

#include <bitset>
#include <cstdint>
#include <iostream>

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

        bool predict() {
            return val >= TAKEN_THRESHOLD;
        }

        uint8_t get_val() {
            return val;
        }
};

// history buffer for individual branch
template <unsigned hist_len>
class HistoryBuffer {
    std::bitset<hist_len> history;

    public:
        HistoryBuffer() :
            history(0x0) {}
        
        void update(BranchResult result) {
            history <<= 1;
            history.set(0, result);
        }

        uint64_t as_idx() {
            return history.to_ullong();
        }
};

