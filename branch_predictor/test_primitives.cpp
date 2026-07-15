#include "primitives.h"

#include <cmath>
#include <cstddef>
#include <iostream>
#include <string>

std::string statuses[2] = {"TEST PASSED", "TEST_FAILED"};

/*
Tests for saturation counter
*/

template<unsigned N>
bool update_and_verify(SatCounter<N>& counter, BranchResult branch, uint8_t expected_val, bool expected_pred) {
    counter.update(branch);
    unsigned actual = counter.get_val();
    if (actual == expected_val) {
        // std::cout << "PASSED: got " << actual << " as expected" << std::endl;
    } else {
        std::cout << "Value check ";
        std::cout << "FAILED: got " << actual << ", expected " << expected_val << std::endl;
        return false;
    }

    bool actual_pred = counter.predict();
    if (actual_pred == expected_pred) {
        // std::cout << "PASSED: got " << actual_pred << " as expected" << std::endl;
    } else {
        std::cout << "Prediction check ";
        std::cout << "FAILED: got " << actual_pred << ", expected " << expected_pred << std::endl;
        std::cout << "-----------------------" << std::endl;
        return false;
    }

    return true;
}

bool test_counter_2() {
    std::cout << "=========Testing 2-bit counter========" << std::endl;
    SatCounter<2> counter = SatCounter<2>();

    bool passed = true;

    passed &= update_and_verify(counter, BranchResult::TAKEN, 2, true);
    passed &= update_and_verify(counter, BranchResult::TAKEN, 3, true);
    passed &= update_and_verify(counter, BranchResult::TAKEN, 3, true);
    passed &= update_and_verify(counter, BranchResult::TAKEN, 3, true);
    passed &= update_and_verify(counter, BranchResult::NOT_TAKEN, 2, true);
    passed &= update_and_verify(counter, BranchResult::NOT_TAKEN, 1, false);
    passed &= update_and_verify(counter, BranchResult::NOT_TAKEN, 0, false);
    passed &= update_and_verify(counter, BranchResult::NOT_TAKEN, 0, false);

    std::cout << "==========" << statuses[!passed] << "========" << std::endl;
    std::cout << "=========END Testing 2-bit counter========" << std::endl << std::endl;

    return passed;
}

bool test_counter_3() {
    std::cout << "=========Testing 3-bit counter========" << std::endl;
    SatCounter<3> counter = SatCounter<3>();

    bool passed = true;

    passed &= update_and_verify(counter, BranchResult::TAKEN, 4, true);
    passed &= update_and_verify(counter, BranchResult::TAKEN, 5, true);
    passed &= update_and_verify(counter, BranchResult::TAKEN, 6, true);
    passed &= update_and_verify(counter, BranchResult::TAKEN, 7, true);
    passed &= update_and_verify(counter, BranchResult::TAKEN, 7, true);
    passed &= update_and_verify(counter, BranchResult::TAKEN, 7, true);
    passed &= update_and_verify(counter, BranchResult::TAKEN, 7, true);
    passed &= update_and_verify(counter, BranchResult::NOT_TAKEN, 6, true);
    passed &= update_and_verify(counter, BranchResult::NOT_TAKEN, 5, true);
    passed &= update_and_verify(counter, BranchResult::NOT_TAKEN, 4, true);
    passed &= update_and_verify(counter, BranchResult::NOT_TAKEN, 3, false);
    passed &= update_and_verify(counter, BranchResult::NOT_TAKEN, 2, false);
    passed &= update_and_verify(counter, BranchResult::NOT_TAKEN, 1, false);
    passed &= update_and_verify(counter, BranchResult::NOT_TAKEN, 0, false);
    passed &= update_and_verify(counter, BranchResult::NOT_TAKEN, 0, false);
    passed &= update_and_verify(counter, BranchResult::TAKEN, 1, false);

    std::cout << "==========" << statuses[!passed] << "========" << std::endl;
    std::cout << "=========END Testing 3-bit counter========" << std::endl << std::endl;

    return passed;
}

bool test_counter_4() {
    std::cout << "=========Testing 4-bit counter========" << std::endl;
    SatCounter<4> counter = SatCounter<4>(0);

    bool passed = true;

    passed &= update_and_verify(counter, BranchResult::NOT_TAKEN, 0, false);
    passed &= update_and_verify(counter, BranchResult::NOT_TAKEN, 0, false);
    passed &= update_and_verify(counter, BranchResult::NOT_TAKEN, 0, false);

    passed &= update_and_verify(counter, BranchResult::TAKEN, 1, false);
    passed &= update_and_verify(counter, BranchResult::TAKEN, 2, false);
    passed &= update_and_verify(counter, BranchResult::TAKEN, 3, false);
    passed &= update_and_verify(counter, BranchResult::TAKEN, 4, false);
    passed &= update_and_verify(counter, BranchResult::NOT_TAKEN, 3, false);

    counter.update(BranchResult::TAKEN); // 4
    counter.update(BranchResult::TAKEN); // 5
    counter.update(BranchResult::TAKEN); // 6
    counter.update(BranchResult::TAKEN); // 7
    counter.update(BranchResult::TAKEN); // 8
    
    passed &= update_and_verify(counter, BranchResult::TAKEN, 9, true);
    passed &= update_and_verify(counter, BranchResult::TAKEN, 10, true);
    passed &= update_and_verify(counter, BranchResult::TAKEN, 11, true);

    std::cout << "==========" << statuses[!passed] << "========" << std::endl;
    std::cout << "=========END Testing 4-bit counter========" << std::endl << std::endl;

    return passed;
}

/*
Tests for branch history buffer
*/

void set_bits_at(HistoryBuffer<256>& hb, std::bitset<256> bits, size_t start) {
    hb.or_other(bits);

    for (int i = 0; i < start; i++) {
        hb.update(BranchResult::NOT_TAKEN);
    }
}

bool test_history_buffer_1() { // ensure as_idx() method is inaccessible to history lengths larger than 64
    bool passed = true;
    HistoryBuffer<128> hb;
    auto p = hb.as_idx(); 

    
    return false;
}

bool test_history_buffer_2() { // test for bit range
    bool passed = true;
    HistoryBuffer<256> hb {};

    std::bitset<256> target {0xFF'0F'CA'78'09'EA'BD'F0}; // randomly-chosen hex bitpattern to index into
    size_t start = 32;
    set_bits_at(hb, target, start);

    // hb.print();
    /*
        start = 32 + 8, num_bits = 16
        0x0...0 FF0FCA7809|EABD| |F0| 0...0
                          |____| |__| |___| <- 32 bit padding
                     extract^     ^8-bit offset
    */


    auto res = hb.get_bit_range(start, 8);
    std::cout << "expected " << 0xF0 << ", got " << res << std::endl;

    res = hb.get_bit_range(start+8, 8);
    std::cout << "expected " << 0xBD << ", got " << res << std::endl;

    res = hb.get_bit_range(start+16, 8);
    std::cout << "expected " << 0xEA << ", got " << res << std::endl;

    res = hb.get_bit_range(start+32, 8);
    std::cout << "expected " << 0x78 << ", got " << res << std::endl;

    res = hb.get_bit_range(start+32, 32);
    std::cout << "expected " << 0xFF'0F'CA'78 << ", got " << res << std::endl;

    res = hb.get_bit_range(start+28, 24);
    std::cout << "expected " << 0xFCA780 << ", got " << res << std::endl;

    return true;
}

bool test_history_buffer_3() { // test folded XOR idx

    std::cout << "=====tests for folded history idx=====" << std::endl;
    bool passed = true;

    HistoryBuffer<256> hb {};
    std::bitset<256> target {0xFF'0F'CA'78'09'EA'BD'F3}; // randomly-chosen hex bitpattern to index into
    size_t start = 0;

    set_bits_at(hb, target, start);

    size_t pc = 0x32D1;

    auto tag_width = 8;
    auto tag_mask = (1ull << tag_width) - 1;

    auto idx_width = 12;
    auto idx_mask = (1ull << idx_width) - 1;

    std::cout << "----testing with hist_len = " << 4 << std::endl;

    auto tag = hb.as_folded_idx(pc, tag_mask, tag_width, 4);
    auto b_idx_12 = hb.as_folded_idx(pc, idx_mask, idx_width, 4);

    auto exp_tag = (pc & tag_mask) ^ 0x03;
    auto exp_idx_12 = (pc & idx_mask) ^ 0x03;

    std::cout << "expected " << exp_tag << ", got " << tag << std::endl;
    std::cout << "expected " << exp_idx_12 << ", got " << b_idx_12 << std::endl;

    std::cout << "----testing with hist_len = " << 8 << std::endl;

    tag = hb.as_folded_idx(pc, tag_mask, tag_width, 8);
    b_idx_12 = hb.as_folded_idx(pc, idx_mask, idx_width, 8);

    exp_tag = (pc & tag_mask) ^ 0xF3;
    exp_idx_12 = (pc & idx_mask) ^ 0xF3;

    std::cout << "expected " << exp_tag << ", got " << tag << std::endl;
    std::cout << "expected " << exp_idx_12 << ", got " << b_idx_12 << std::endl;

    std::cout << "----testing with hist_len = " << 16 << std::endl;

    tag = hb.as_folded_idx(pc, tag_mask, tag_width, 16);
    b_idx_12 = hb.as_folded_idx(pc, idx_mask, idx_width, 16);

    exp_tag = (pc & tag_mask) ^ 0xF3 ^ 0xBD;
    exp_idx_12 = (pc & idx_mask) ^ 0xDF3 ^ 0x00B;

    std::cout << "expected " << exp_tag << ", got " << tag << std::endl;
    std::cout << "expected " << exp_idx_12 << ", got " << b_idx_12 << std::endl;
    
    std::cout << "----testing with hist_len = " << 32 << std::endl;

    tag = hb.as_folded_idx(pc, tag_mask, tag_width, 32);
    b_idx_12 = hb.as_folded_idx(pc, idx_mask, idx_width, 32);

    exp_tag = (pc & tag_mask) ^ 0xF3 ^ 0xBD ^ 0xEA ^ 0x09;
    exp_idx_12 = (pc & idx_mask) ^ 0xDF3 ^ 0xEAB ^ 0x009;

    std::cout << "expected " << exp_tag << ", got " << tag << std::endl;
    std::cout << "expected " << exp_idx_12 << ", got " << b_idx_12 << std::endl;

    std::cout << "----testing with hist_len = " << 64 << std::endl;

    tag = hb.as_folded_idx(pc, tag_mask, tag_width, 64);
    b_idx_12 = hb.as_folded_idx(pc, idx_mask, idx_width, 64);

    exp_tag = (pc & tag_mask) ^ 0xF3 ^ 0xBD ^ 0xEA ^ 0x09 ^ 0x78 ^ 0xCA ^ 0x0F ^ 0xFF;
    exp_idx_12 = (pc & idx_mask) ^ 0xDF3 ^ 0xEAB ^ 0x809 ^ 0xCA7 ^ 0xF0F ^ 0x00F;

    std::cout << "expected " << exp_tag << ", got " << tag << std::endl;
    std::cout << "expected " << exp_idx_12 << ", got " << b_idx_12 << std::endl;

    std::cout << "----testing with hist_len = " << 128 << std::endl;

    tag = hb.as_folded_idx(pc, tag_mask, tag_width, 128);
    b_idx_12 = hb.as_folded_idx(pc, idx_mask, idx_width, 128);

    exp_tag = (pc & tag_mask) ^ 0xF3 ^ 0xBD ^ 0xEA ^ 0x09 ^ 0x78 ^ 0xCA ^ 0x0F ^ 0xFF;
    exp_idx_12 = (pc & idx_mask) ^ 0xDF3 ^ 0xEAB ^ 0x809 ^ 0xCA7 ^ 0xF0F ^ 0x00F;

    std::cout << "expected " << exp_tag << ", got " << tag << std::endl;
    std::cout << "expected " << exp_idx_12 << ", got " << b_idx_12 << std::endl;

    std::cout << "----testing with hist_len = " << 256 << std::endl;

    tag = hb.as_folded_idx(pc, tag_mask, tag_width, 256);
    b_idx_12 = hb.as_folded_idx(pc, idx_mask, idx_width, 256);

    exp_tag = (pc & tag_mask) ^ 0xF3 ^ 0xBD ^ 0xEA ^ 0x09 ^ 0x78 ^ 0xCA ^ 0x0F ^ 0xFF;
    exp_idx_12 = (pc & idx_mask) ^ 0xDF3 ^ 0xEAB ^ 0x809 ^ 0xCA7 ^ 0xF0F ^ 0x00F;

    std::cout << "expected " << exp_tag << ", got " << tag << std::endl;
    std::cout << "expected " << exp_idx_12 << ", got " << b_idx_12 << std::endl;

    return false;   
}

bool test_perceptron_1() {
    bool passed = true;
}

bool test_perceptron_2() {
    bool passed = true;
}

bool test_perceptron_3() {
    bool passed = true;
}

int main() {
    test_counter_2();
    test_counter_3();
    test_counter_4();

    test_history_buffer_1();
    test_history_buffer_2();
    test_history_buffer_3();
    
    test_perceptron_1();
    test_perceptron_1();
    test_perceptron_1();
}
