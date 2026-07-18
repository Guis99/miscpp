#include "primitives.h"

#include <cmath>
#include <cstddef>
#include <iostream>
#include <string>

#define LOUD 0


// helpers
std::string statuses[2] = {"TEST PASSED", "TEST_FAILED"};

template<typename T1, typename T2>
bool compare(T1 expected, T2 actual) {
    if (expected == actual) {
        if (LOUD > 0) {
            std::cout << "expected " << expected << ", got " << actual << std::endl;
        }
        return true;
    } else {
        std::cout << "expected " << expected << ", got " << actual << std::endl;
        return false;
    }
}

/*
Tests for saturation counter
*/

template<unsigned N>
bool update_and_verify(SatCounter<N>& counter, BranchResult branch, u8 expected_val, bool expected_pred) {
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


template bool compare<size_t>(size_t expected, size_t actual);
template bool compare<u64>(u64 expected, u64 actual);

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
    passed &= compare<size_t>(0xF0, res);

    res = hb.get_bit_range(start+8, 8);
    passed &= compare<size_t>(0xBD, res);

    res = hb.get_bit_range(start+16, 8);
    passed &= compare<size_t>(0xEA, res);

    res = hb.get_bit_range(start+32, 8);
    passed &= compare<size_t>(0x78, res);

    res = hb.get_bit_range(start+32, 32);
    passed &= compare<size_t>(0xFF'0F'CA'78, res);

    res = hb.get_bit_range(start+28, 24);
    passed &= compare<size_t>(0xFCA780, res);

    std::cout << "==========" << statuses[!passed] << "========" << std::endl;
    std::cout << "=========END Testing history buffer indexing========" << std::endl << std::endl;
    return passed;
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

    passed &= compare(exp_tag, tag);
    passed &= compare(exp_idx_12, b_idx_12);

    std::cout << "----testing with hist_len = " << 8 << std::endl;

    tag = hb.as_folded_idx(pc, tag_mask, tag_width, 8);
    b_idx_12 = hb.as_folded_idx(pc, idx_mask, idx_width, 8);

    exp_tag = (pc & tag_mask) ^ 0xF3;
    exp_idx_12 = (pc & idx_mask) ^ 0xF3;

    passed &= compare(exp_tag, tag);
    passed &= compare(exp_idx_12, b_idx_12);

    std::cout << "----testing with hist_len = " << 16 << std::endl;

    tag = hb.as_folded_idx(pc, tag_mask, tag_width, 16);
    b_idx_12 = hb.as_folded_idx(pc, idx_mask, idx_width, 16);

    exp_tag = (pc & tag_mask) ^ 0xF3 ^ 0xBD;
    exp_idx_12 = (pc & idx_mask) ^ 0xDF3 ^ 0x00B;

    passed &= compare(exp_tag, tag);
    passed &= compare(exp_idx_12, b_idx_12);
    
    std::cout << "----testing with hist_len = " << 32 << std::endl;

    tag = hb.as_folded_idx(pc, tag_mask, tag_width, 32);
    b_idx_12 = hb.as_folded_idx(pc, idx_mask, idx_width, 32);

    exp_tag = (pc & tag_mask) ^ 0xF3 ^ 0xBD ^ 0xEA ^ 0x09;
    exp_idx_12 = (pc & idx_mask) ^ 0xDF3 ^ 0xEAB ^ 0x009;

    passed &= compare(exp_tag, tag);
    passed &= compare(exp_idx_12, b_idx_12);

    std::cout << "----testing with hist_len = " << 64 << std::endl;

    tag = hb.as_folded_idx(pc, tag_mask, tag_width, 64);
    b_idx_12 = hb.as_folded_idx(pc, idx_mask, idx_width, 64);

    exp_tag = (pc & tag_mask) ^ 0xF3 ^ 0xBD ^ 0xEA ^ 0x09 ^ 0x78 ^ 0xCA ^ 0x0F ^ 0xFF;
    exp_idx_12 = (pc & idx_mask) ^ 0xDF3 ^ 0xEAB ^ 0x809 ^ 0xCA7 ^ 0xF0F ^ 0x00F;

    passed &= compare(exp_tag, tag);
    passed &= compare(exp_idx_12, b_idx_12);

    std::cout << "----testing with hist_len = " << 128 << std::endl;

    tag = hb.as_folded_idx(pc, tag_mask, tag_width, 128);
    b_idx_12 = hb.as_folded_idx(pc, idx_mask, idx_width, 128);

    exp_tag = (pc & tag_mask) ^ 0xF3 ^ 0xBD ^ 0xEA ^ 0x09 ^ 0x78 ^ 0xCA ^ 0x0F ^ 0xFF;
    exp_idx_12 = (pc & idx_mask) ^ 0xDF3 ^ 0xEAB ^ 0x809 ^ 0xCA7 ^ 0xF0F ^ 0x00F;

    passed &= compare(exp_tag, tag);
    passed &= compare(exp_idx_12, b_idx_12);

    std::cout << "----testing with hist_len = " << 256 << std::endl;

    tag = hb.as_folded_idx(pc, tag_mask, tag_width, 256);
    b_idx_12 = hb.as_folded_idx(pc, idx_mask, idx_width, 256);

    exp_tag = (pc & tag_mask) ^ 0xF3 ^ 0xBD ^ 0xEA ^ 0x09 ^ 0x78 ^ 0xCA ^ 0x0F ^ 0xFF;
    exp_idx_12 = (pc & idx_mask) ^ 0xDF3 ^ 0xEAB ^ 0x809 ^ 0xCA7 ^ 0xF0F ^ 0x00F;

    passed &= compare(exp_tag, tag);
    passed &= compare(exp_idx_12, b_idx_12);

    std::cout << "==========" << statuses[!passed] << "========" << std::endl;
    std::cout << "=========END Testing history buffer folded hashing========" << std::endl << std::endl;

    return passed;   
}

bool test_folded_tracker_1() {
    bool passed = true;

    u16 L1 = 4;
    HistoryBuffer<64> hb {};

    auto tag_width = 5;
    auto tag_mask = (1ull << tag_width) - 1;

    auto idx_width = 6;
    auto idx_mask = (1ull << idx_width) - 1;

    std::vector<u16> h = {4, 8, 16, 32, 64};
    std::vector<csr> trackers_idx;
    std::vector<csr> trackers_tag;
    for (auto i : h) {
        trackers_idx.emplace_back(idx_mask, idx_width - 1, i % idx_width - 1);
        trackers_tag.emplace_back(tag_mask, tag_width - 1, i % tag_width - 1);
    }

    std::cout << "FIND ME" << 4%5-1 << std::endl;

    for (int i = 0; i < h.size(); i++) {        
        passed &= compare(0, trackers_idx[i].hash);
        passed &= compare(0, trackers_tag[i].hash);
    }


    // 1: 1
     /*
        idx: 000001

        tag: 00001
    */
    std::vector<u32> expected_idx;
    std::vector<u32> expected_tag;
    BranchResult new_branch = BranchResult::TAKEN;
    
    expected_idx = {1,1,1,1,1};
    expected_tag = {1,1,1,1,1};
    for (int i = 0; i < h.size(); i++) {
        auto hl = h[i];
        bool oldest = hb.get_bit_at(hl-1);
        trackers_idx[i].shift_and_fold(new_branch, oldest);
        trackers_tag[i].shift_and_fold(new_branch, oldest);

        passed &= compare(expected_idx[i], trackers_idx[i].hash);
        passed &= compare(expected_tag[i], trackers_tag[i].hash);
        if (!passed) {
            std::cout << "History length: " << hl << std::endl;
        }
    }
    hb.update(new_branch);
    std::cout << "=======UPDATE 1=======" << std::endl;
    hb.print();
    std::cout << "=======UPDATE 1=======" << std::endl;

    // 2: 11
    /*
        idx: 000011

        tag: 00011
    */
    new_branch = BranchResult::TAKEN;
    expected_idx = {3,3,3,3,3};
    expected_tag = {3,3,3,3,3};
    for (int i = 0; i < h.size(); i++) {
        auto hl = h[i];
        bool oldest = hb.get_bit_at(hl-1);
        trackers_idx[i].shift_and_fold(new_branch, oldest);
        trackers_tag[i].shift_and_fold(new_branch, oldest);

        passed &= compare(expected_idx[i], trackers_idx[i].hash);
        passed &= compare(expected_tag[i], trackers_tag[i].hash);
        if (!passed) {
            std::cout << "History length: " << hl << std::endl;
        }
    }
    hb.update(new_branch);
    std::cout << "=======UPDATE 2=======" << std::endl;
    hb.print();
    std::cout << "=======UPDATE 2=======" << std::endl;

    // 3: 110
    /*
        idx: 000110

        tag: 00110
    */
    new_branch = BranchResult::NOT_TAKEN;
    expected_idx = {6,6,6,6,6};
    expected_tag = {6,6,6,6,6};
    for (int i = 0; i < h.size(); i++) {
        auto hl = h[i];
        bool oldest = hb.get_bit_at(hl-1);
        trackers_idx[i].shift_and_fold(new_branch, oldest);
        trackers_tag[i].shift_and_fold(new_branch, oldest);

        passed &= compare(expected_idx[i], trackers_idx[i].hash);
        passed &= compare(expected_tag[i], trackers_tag[i].hash);
        if (!passed) {
            std::cout << "History length: " << hl << std::endl;
        }
    }
    hb.update(new_branch);
    std::cout << "=======UPDATE 3=======" << std::endl;
    hb.print();
    std::cout << "=======UPDATE 3=======" << std::endl;

    // 4: 1100
    /*
        idx: 001100

        tag: 01100
    */
    new_branch = BranchResult::NOT_TAKEN;
    expected_idx = {12,12,12,12,12};
    expected_tag = {12,12,12,12,12};
    for (int i = 0; i < h.size(); i++) {
        auto hl = h[i];
        bool oldest = hb.get_bit_at(hl-1);
        trackers_idx[i].shift_and_fold(new_branch, oldest);
        trackers_tag[i].shift_and_fold(new_branch, oldest);

        passed &= compare(expected_idx[i], trackers_idx[i].hash);
        passed &= compare(expected_tag[i], trackers_tag[i].hash);
        if (!passed) {
            std::cout << "History length: " << hl << std::endl;
        }
    }
    hb.update(new_branch);
    std::cout << "=======UPDATE 4=======" << std::endl;
    hb.print();
    std::cout << "=======UPDATE 4=======" << std::endl;

     // 5: 11001
     /*
        idx: 011001

        tag: 11001
    */
    new_branch = BranchResult::TAKEN;
    expected_idx = {9,25,25,25,25};
    expected_tag = {9,25,25,25,25};
    for (int i = 0; i < h.size(); i++) {
        auto hl = h[i];
        bool oldest = hb.get_bit_at(hl-1);
        trackers_idx[i].shift_and_fold(new_branch, oldest);
        trackers_tag[i].shift_and_fold(new_branch, oldest);

        passed &= compare(expected_idx[i], trackers_idx[i].hash);
        passed &= compare(expected_tag[i], trackers_tag[i].hash);
        if (!passed) {
            std::cout << "History length: " << hl << std::endl;
        }
    }
    hb.update(new_branch);
    std::cout << "=======UPDATE 5=======" << std::endl;
    hb.print();
    std::cout << "=======UPDATE 5=======" << std::endl;

    // 6: 110011
    /*
        idx: 110011

        tag: 00001
             10011 -> 10010
    */

    new_branch = BranchResult::TAKEN;
    expected_idx = {3,51,51,51,51};
    expected_tag = {3,18,18,18,18};
    for (int i = 0; i < h.size(); i++) {
        auto hl = h[i];
        bool oldest = hb.get_bit_at(hl-1);
        trackers_idx[i].shift_and_fold(new_branch, oldest);
        trackers_tag[i].shift_and_fold(new_branch, oldest);

        passed &= compare(expected_idx[i], trackers_idx[i].hash);
        passed &= compare(expected_tag[i], trackers_tag[i].hash);
        if (!passed) {
            std::cout << "History length: " << hl << std::endl;
        }
    }
    hb.update(new_branch);
    std::cout << "=======UPDATE 6=======" << std::endl;
    hb.print();
    std::cout << "=======UPDATE 6=======" << std::endl;

    // 7: 1100111
    /*
        idx: 000001 
             100111 -> 100110

        tag: 00011
             00111 -> 00100
    */

    new_branch = BranchResult::TAKEN;
    expected_idx = {7,38,38,38,38};
    expected_tag = {7,4,4,4,4};
    for (int i = 0; i < h.size(); i++) {
        auto hl = h[i];
        bool oldest = hb.get_bit_at(hl-1);
        trackers_idx[i].shift_and_fold(new_branch, oldest);
        trackers_tag[i].shift_and_fold(new_branch, oldest);

        passed &= compare(expected_idx[i], trackers_idx[i].hash);
        passed &= compare(expected_tag[i], trackers_tag[i].hash);
        if (!passed) {
            std::cout << "History length: " << hl << std::endl;
        }
    }
    hb.update(new_branch);
    std::cout << "=======UPDATE 7=======" << std::endl;
    hb.print();
    std::cout << "=======UPDATE 7=======" << std::endl;

    // 8: 11001110
    /*
        idx: 000011 
             001110 -> 001101

        tag: 00110
             01110 -> 01000
    */
    std::cout << "=======UPDATE 8=======" << std::endl;
    new_branch = BranchResult::NOT_TAKEN;
    expected_idx = {14,13,13,13,13};
    expected_tag = {14,8,8,8,8};
    for (int i = 0; i < h.size(); i++) {
        auto hl = h[i];
        bool oldest = hb.get_bit_at(hl-1);
        trackers_idx[i].shift_and_fold(new_branch, oldest);
        trackers_tag[i].shift_and_fold(new_branch, oldest);

        passed &= compare(expected_idx[i], trackers_idx[i].hash);
        passed &= compare(expected_tag[i], trackers_tag[i].hash);
        if (!passed) {
            std::cout << "History length: " << hl << std::endl;
        }
    }
    hb.update(new_branch);
    
    hb.print();
    std::cout << "=======UPDATE 8=======" << std::endl;

    // 9: 110011101
    /*
        idx: 000110 
             011101 -> 011011

        tag: 01100
             11101 -> 10001
    */
    std::cout << "=======UPDATE 9=======" << std::endl;
    new_branch = BranchResult::TAKEN;
    expected_idx = {13,31,27,27,27};
    expected_tag = {13,25,17,17,17};
    for (int i = 0; i < h.size(); i++) {
        auto hl = h[i];
        bool oldest = hb.get_bit_at(hl-1);
        trackers_idx[i].shift_and_fold(new_branch, oldest);
        trackers_tag[i].shift_and_fold(new_branch, oldest);

        passed &= compare(expected_idx[i], trackers_idx[i].hash);
        passed &= compare(expected_tag[i], trackers_tag[i].hash);
        if (!passed) {
            std::cout << "History length: " << hl << std::endl;
        }
    }
    hb.update(new_branch);

    hb.print();
    std::cout << "=======UPDATE 9=======" << std::endl;

    std::cout << "==========" << statuses[!passed] << "========" << std::endl;
    std::cout << "=========END Testing incremental hashing 1========" << std::endl << std::endl;
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

    test_folded_tracker_1();
    
    test_perceptron_1();
    test_perceptron_1();
    test_perceptron_1();
}
