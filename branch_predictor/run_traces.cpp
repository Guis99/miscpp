#include "predictors.h"
#include "primitives.h"
#include "traces.h"

#include <cassert>
#include <cstddef>
#include <vector>


// ---- Generic trace functions ----

void trace_1_0(BranchPredictorBase* predictor) {
    size_t num_reps = 128;
    size_t seq_len = 2 * num_reps;
    size_t score = 0;
    std::vector<BranchInstr> trace = repeating_pattern(1, 1, num_reps);

    for (const auto branch : trace) {
        bool pred = predictor->predict(branch.pc);
        score += pred == branch.direction;
        predictor->update(branch.pc, branch.direction);
    }

    std::cout << "[" << *predictor << "] trace_1_0: Predicted " << score << " out of " << seq_len << " correctly" << std::endl;
}

void trace_taken_then_not_taken(BranchPredictorBase* predictor) {
    size_t num_dir = 128;
    size_t seq_len = 2 * num_dir;
    size_t score = 0;
    std::vector<BranchInstr> trace = repeating_pattern(num_dir, num_dir, 1);

    for (const auto branch : trace) {
        bool pred = predictor->predict(branch.pc);
        score += pred == branch.direction;
        predictor->update(branch.pc, branch.direction);
    }

    std::cout << "[" << *predictor << "] trace_taken_then_not_taken: Predicted " << score << " out of " << seq_len << " correctly" << std::endl;
}

void trace_3_3(BranchPredictorBase* predictor) {
    size_t num_reps = 128;
    size_t seq_len = 6 * num_reps;
    size_t score = 0;
    std::vector<BranchInstr> trace = repeating_pattern(3, 3, num_reps);

    for (const auto branch : trace) {
        bool pred = predictor->predict(branch.pc);
        score += pred == branch.direction;
        predictor->update(branch.pc, branch.direction);
    }

    std::cout << "[" << *predictor << "] trace_3_3: Predicted " << score << " out of " << seq_len << " correctly" << std::endl;
}

void trace_8_8(BranchPredictorBase* predictor) {
    size_t num_reps = 128;
    size_t seq_len = 16 * num_reps;
    size_t score = 0;
    std::vector<BranchInstr> trace = repeating_pattern(8, 8, num_reps);

    for (const auto branch : trace) {
        bool pred = predictor->predict(branch.pc);
        score += pred == branch.direction;
        predictor->update(branch.pc, branch.direction);
    }

    std::cout << "[" << *predictor << "] trace_8_8: Predicted " << score << " out of " << seq_len << " correctly" << std::endl;
}

void trace_corr(BranchPredictorBase* predictor) {
    size_t num_reps = 512;
    size_t score = 0;
    std::vector<BranchInstr> trace = correlated_branch(num_reps, 0, 0, 1, 1, false);

    for (const auto branch : trace) {
        bool pred = predictor->predict(branch.pc);
        if (branch.id == 1) {
            score += pred == branch.direction;
        }
        predictor->update(branch.pc, branch.direction);
    }

    std::cout << "[" << *predictor << "] trace_corr: Predicted " << score << " out of " << num_reps << " correctly" << std::endl;
}

void trace_anti_corr(BranchPredictorBase* predictor) {
    size_t num_reps = 512;
    size_t score = 0;
    std::vector<BranchInstr> trace = correlated_branch(num_reps, 0, 0, 1, 1, true);

    for (const auto branch : trace) {
        bool pred = predictor->predict(branch.pc);
        if (branch.id == 1) {
            score += pred == branch.direction;
        }
        predictor->update(branch.pc, branch.direction);
    }

    std::cout << "[" << *predictor << "] trace_anti_corr: Predicted " << score << " out of " << num_reps << " correctly" << std::endl;
}

void trace_xor(BranchPredictorBase* predictor) {
    size_t num_reps = 512;
    size_t score = 0;
    std::vector<BranchInstr> trace = xor_correlated_branch(num_reps, 0, 0, 1, 1, 2, 2);

    for (const auto branch : trace) {
        bool pred = predictor->predict(branch.pc);
        if (branch.id == 2) {
            score += pred == branch.direction;
        }
        predictor->update(branch.pc, branch.direction);
    }

    std::cout << "[" << *predictor << "] trace_xor: Predicted " << score << " out of " << num_reps << " correctly" << std::endl;
}


// ---- Wrapper functions (predictor + trace type) ----

void run_trace_1_0_bimodal() {
    BimodalPredictor predictor(16);
    trace_1_0(&predictor);
}

void run_trace_taken_then_not_taken_bimodal() {
    BimodalPredictor predictor(16);
    trace_taken_then_not_taken(&predictor);
}

void run_trace_3_3_bimodal() {
    BimodalPredictor predictor(16);
    trace_3_3(&predictor);
}

void run_trace_8_8_bimodal() {
    BimodalPredictor predictor(16);
    trace_8_8(&predictor);
}

void run_trace_corr_gshare() {
    GSharePredictor predictor(16, 2);
    trace_corr(&predictor);
}

void run_trace_anti_corr_gshare() {
    GSharePredictor predictor(16, 2);
    trace_anti_corr(&predictor);
}

void run_trace_xor_gshare() {
    GSharePredictor predictor(16, 2);
    trace_xor(&predictor);
}

void run_trace_1_0_twolvl() {
    TwoLevelPredictor predictor(16, 2);
    trace_1_0(&predictor);
}

void run_trace_taken_then_not_taken_twolvl() {
    TwoLevelPredictor predictor(16, 2);
    trace_taken_then_not_taken(&predictor);
}

void run_trace_3_3_twolvl() {
    TwoLevelPredictor predictor(16, 3);
    trace_3_3(&predictor);
}

void run_trace_8_8_twolvl() {
    TwoLevelPredictor predictor(16, 2);
    trace_8_8(&predictor);
}


int main() {
    std::cout << "======Bimodal sanity checks======" << std::endl;
    run_trace_1_0_bimodal();
    run_trace_taken_then_not_taken_bimodal();
    run_trace_3_3_bimodal();
    run_trace_8_8_bimodal();

    std::cout << "======GShare sanity checks======" << std::endl;
    run_trace_corr_gshare();
    run_trace_anti_corr_gshare();
    run_trace_xor_gshare();

    std::cout << "======Two-level sanity checks======" << std::endl;
    run_trace_1_0_twolvl();
    run_trace_taken_then_not_taken_twolvl();
    run_trace_3_3_twolvl();
    run_trace_8_8_twolvl();
}
