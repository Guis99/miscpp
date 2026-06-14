#include "primitives.h"
#include <atomic>
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

bool test_history_buffer_1() {
    bool passed = true;
    return false;
}

bool test_history_buffer_2() {
    bool passed = true;
    
    return false;
}

bool test_history_buffer_3() {
    bool passed = true;
    return false;   
}

int main() {
    test_counter_2();
    test_counter_4();

    test_history_buffer_1();
    test_history_buffer_2();
    test_history_buffer_3();
}
