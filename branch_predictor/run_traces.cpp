#include "predictors.h"
#include "primitives.h"
#include "traces.h"

#include <cassert>
#include <cstddef>
#include <vector>


// ---- Generic trace functions ----

void trace_1_0(BranchPredictorBase* predictor) {
    // alternating taken-not taken
    size_t num_reps = 1024;
    size_t seq_len = 2 * num_reps;
    size_t score = 0;
    std::vector<BranchInstr> trace = repeating_pattern(1, 1, num_reps);

    for (const auto& branch : trace) {
        bool pred = predictor->predict(branch.pc);
        score += pred == branch.direction;
        predictor->update(branch.pc, branch.direction);
    }

    std::cout << "[" << *predictor << "] trace_1_0: Predicted " << score << " out of " << seq_len << " correctly" << std::endl;
}

void trace_taken_then_not_taken(BranchPredictorBase* predictor) { 
    // many taken followed by not takens
    size_t num_dir = 128*128;
    size_t seq_len = 2 * num_dir;
    size_t score = 0;
    std::vector<BranchInstr> trace = repeating_pattern(num_dir, num_dir, 1);

    for (const auto& branch : trace) {
        bool pred = predictor->predict(branch.pc);
        score += pred == branch.direction;
        predictor->update(branch.pc, branch.direction);
    }

    std::cout << "[" << *predictor << "] trace_taken_then_not_taken: Predicted " << score << " out of " << seq_len << " correctly" << std::endl;
}

void trace_3_3(BranchPredictorBase* predictor) { 
    // 3 taken followed by 3 not taken
    size_t num_reps = 1024;
    size_t seq_len = 6 * num_reps;
    size_t score = 0;
    std::vector<BranchInstr> trace = repeating_pattern(3, 3, num_reps);

    for (const auto& branch : trace) {
        bool pred = predictor->predict(branch.pc);
        score += pred == branch.direction;
        predictor->update(branch.pc, branch.direction);
    }

    std::cout << "[" << *predictor << "] trace_3_3: Predicted " << score << " out of " << seq_len << " correctly" << std::endl;
}

void trace_5_2(BranchPredictorBase* predictor) { 
    // 3 taken followed by 3 not taken
    size_t num_reps = 1024;
    size_t seq_len = 7 * num_reps;
    size_t score = 0;
    std::vector<BranchInstr> trace = repeating_pattern(5, 2, num_reps);

    for (const auto& branch : trace) {
        bool pred = predictor->predict(branch.pc);
        score += pred == branch.direction;
        predictor->update(branch.pc, branch.direction);
    }

    std::cout << "[" << *predictor << "] trace_5_2: Predicted " << score << " out of " << seq_len << " correctly" << std::endl;
}

void trace_8_8(BranchPredictorBase* predictor) {
    // 8 taken followed by 8 not taken
    size_t num_reps = 1024;
    size_t seq_len = 16 * num_reps;
    size_t score = 0;
    std::vector<BranchInstr> trace = repeating_pattern(8, 8, num_reps);

    for (const auto& branch : trace) {
        bool pred = predictor->predict(branch.pc);
        score += pred == branch.direction;
        predictor->update(branch.pc, branch.direction);
    }

    std::cout << "[" << *predictor << "] trace_8_8: Predicted " << score << " out of " << seq_len << " correctly" << std::endl;
}

void trace_corr(BranchPredictorBase* predictor) {
    size_t num_reps = 1024;
    size_t score = 0;
    std::vector<BranchInstr> trace = correlated_branch(num_reps, 0, 0, 1, 1, false);

    for (const auto& branch : trace) {
        bool pred = predictor->predict(branch.pc);
        if (branch.id == 1) {
            score += pred == branch.direction;
        }
        predictor->update(branch.pc, branch.direction);
    }

    std::cout << "[" << *predictor << "] trace_corr: Predicted " << score << " out of " << num_reps << " correctly" << std::endl;
}

void trace_anti_corr(BranchPredictorBase* predictor) {
    size_t num_reps = 1024;
    size_t score = 0;
    std::vector<BranchInstr> trace = correlated_branch(num_reps, 0, 0, 1, 1, true);

    for (const auto& branch : trace) {
        bool pred = predictor->predict(branch.pc);
        if (branch.id == 1) {
            score += pred == branch.direction;
        }
        predictor->update(branch.pc, branch.direction);
    }

    std::cout << "[" << *predictor << "] trace_anti_corr: Predicted " << score << " out of " << num_reps << " correctly" << std::endl;
}

void trace_xor(BranchPredictorBase* predictor) {
    size_t num_reps = 1024;
    size_t score = 0;
    std::vector<BranchInstr> trace = xor_correlated_branch(num_reps, 0, 0, 1, 1, 2, 2);

    for (const auto& branch : trace) {
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
    BimodalPredictor predictor(4);
    trace_1_0(&predictor);
}

void run_trace_taken_then_not_taken_bimodal() {
    BimodalPredictor predictor(4);
    trace_taken_then_not_taken(&predictor);
}

void run_trace_3_3_bimodal() {
    BimodalPredictor predictor(4);
    trace_3_3(&predictor);
}

void run_trace_5_2_bimodal() {
    BimodalPredictor predictor(4);
    trace_5_2(&predictor);
}

void run_trace_8_8_bimodal() {
    BimodalPredictor predictor(4);
    trace_8_8(&predictor);
}

void run_trace_corr_gshare() {
    GSharePredictor predictor(4, 2);
    trace_corr(&predictor);
}

void run_trace_anti_corr_gshare() {
    GSharePredictor predictor(4, 2);
    trace_anti_corr(&predictor);
}

void run_trace_xor_gshare() {
    GSharePredictor predictor(4, 2);
    trace_xor(&predictor);
}

void run_trace_1_0_gshare() {
    GSharePredictor predictor(4, 3);
    trace_1_0(&predictor);
}

void run_trace_5_2_gshare() {
    GSharePredictor predictor(4, 3);
    trace_5_2(&predictor);
}

void run_trace_8_8_gshare() {
    GSharePredictor predictor(4, 3);
    trace_8_8(&predictor);
}

// Two-level predictor runners

void run_trace_1_0_twolvl() {
    TwoLevelPredictor predictor(4, 2);
    trace_1_0(&predictor);
}

void run_trace_taken_then_not_taken_twolvl() {
    TwoLevelPredictor predictor(4, 2);
    trace_taken_then_not_taken(&predictor);
}

void run_trace_3_3_twolvl() {
    TwoLevelPredictor predictor(4, 3);
    trace_3_3(&predictor);
}

void run_trace_5_2_twolvl() {
    TwoLevelPredictor predictor(4, 3);
    trace_5_2(&predictor);
}

void run_trace_8_8_twolvl() {
    TwoLevelPredictor predictor(4, 2);
    trace_8_8(&predictor);
}

void run_trace_xor_twolvl() {
    TwoLevelPredictor predictor(4, 2);
    trace_xor(&predictor);
}

// tournament runners

void run_trace_1_0_tourn() {
    TwoLevelPredictor predictor1(4, 2);
    GSharePredictor predictor2(4, 3);
    TournamentPredictor<GSharePredictor, TwoLevelPredictor> predictor(predictor2, predictor1, 4);
    trace_1_0(&predictor);
}

void run_trace_taken_then_not_taken_tourn() {
    TwoLevelPredictor predictor1(4, 2);
    GSharePredictor predictor2(4, 3);
    TournamentPredictor<GSharePredictor, TwoLevelPredictor> predictor(predictor2, predictor1, 4);
    trace_taken_then_not_taken(&predictor);
}

void run_trace_3_3_tourn() {
    TwoLevelPredictor predictor1(4, 2);
    GSharePredictor predictor2(4, 3);
    TournamentPredictor<GSharePredictor, TwoLevelPredictor> predictor(predictor2, predictor1, 4);
    trace_3_3(&predictor);
}

void run_trace_5_2_tourn() {
    TwoLevelPredictor predictor1(4, 8);
    GSharePredictor predictor2(4, 8);
    TournamentPredictor<GSharePredictor, TwoLevelPredictor> predictor(predictor2, predictor1, 4);
    trace_5_2(&predictor);
}

void run_trace_8_8_tourn() {
    TwoLevelPredictor predictor1(4, 8);
    GSharePredictor predictor2(4, 8);
    TournamentPredictor<GSharePredictor, TwoLevelPredictor> predictor(predictor2, predictor1, 4);
    trace_8_8(&predictor);
}

void run_trace_xor_tourn() {
    TwoLevelPredictor predictor1(4, 2);
    GSharePredictor predictor2(4, 3);
    TournamentPredictor<GSharePredictor, TwoLevelPredictor> predictor(predictor2, predictor1, 4);
    trace_xor(&predictor);
}

// perceptron runners

#define PERCEPTRON_HIST 32

void run_trace_1_0_perc() {
    PerceptronPredictor predictor(4, PERCEPTRON_HIST);
    trace_1_0(&predictor);
}

void run_trace_taken_then_not_taken_perc() {
    PerceptronPredictor predictor(4, PERCEPTRON_HIST);
    trace_taken_then_not_taken(&predictor);
}

void run_trace_3_3_perc() {
    PerceptronPredictor predictor(4, PERCEPTRON_HIST);
    trace_3_3(&predictor);
}

void run_trace_5_2_perc() {
    PerceptronPredictor predictor(4, PERCEPTRON_HIST);
    trace_5_2(&predictor);
}

void run_trace_8_8_perc() {
    PerceptronPredictor predictor(4, PERCEPTRON_HIST);
    trace_8_8(&predictor);
}

void run_trace_xor_perc() {
    PerceptronPredictor predictor(4, PERCEPTRON_HIST);
    trace_xor(&predictor);
}

// TAGE runners

#define NUM_COMP 5
#define L1 4

void run_trace_1_0_tage() {
    TAGEPredictor predictor(4, 3, NUM_COMP, L1);
    trace_1_0(&predictor);
}

void run_trace_taken_then_not_taken_tage() {
    TAGEPredictor predictor(4, 3, NUM_COMP, L1);
    trace_taken_then_not_taken(&predictor);
}

void run_trace_3_3_tage() {
    TAGEPredictor predictor(4, 3, NUM_COMP, L1);
    trace_3_3(&predictor);
}

void run_trace_5_2_tage() {
    TAGEPredictor predictor(4, 3, NUM_COMP, L1);
    trace_5_2(&predictor);
}

void run_trace_8_8_tage() {
    TAGEPredictor predictor(4, 3, NUM_COMP, L1);
    trace_8_8(&predictor);
}

void run_trace_xor_tage() {
    TAGEPredictor predictor(4, 3, NUM_COMP, L1);
    trace_xor(&predictor);
}

int main() {
    std::cout << "======Bimodal sanity checks======" << std::endl;
    run_trace_1_0_bimodal();
    run_trace_taken_then_not_taken_bimodal();
    run_trace_3_3_bimodal();
    run_trace_5_2_bimodal();
    run_trace_8_8_bimodal();

    std::cout << "======GShare sanity checks======" << std::endl;
    run_trace_corr_gshare();
    run_trace_anti_corr_gshare();
    run_trace_xor_gshare();
    run_trace_1_0_gshare();
    run_trace_5_2_gshare();
    run_trace_8_8_gshare();

    std::cout << "======Two-level sanity checks======" << std::endl;
    run_trace_1_0_twolvl();
    run_trace_taken_then_not_taken_twolvl();
    run_trace_3_3_twolvl();
    run_trace_5_2_twolvl();
    run_trace_8_8_twolvl();
    run_trace_xor_twolvl();

    std::cout << "======Tournament sanity checks======" << std::endl;
    run_trace_1_0_tourn();
    run_trace_taken_then_not_taken_tourn();
    run_trace_3_3_tourn();
    run_trace_5_2_tourn();
    run_trace_8_8_tourn();
    run_trace_xor_tourn();

    std::cout << "======Perceptron sanity checks======" << std::endl;
    run_trace_1_0_perc();
    run_trace_taken_then_not_taken_perc();
    run_trace_3_3_perc();
    run_trace_5_2_perc();
    run_trace_8_8_perc();
    run_trace_xor_perc();

    std::cout << "======TAGE sanity checks======" << std::endl;
    run_trace_1_0_tage();
    run_trace_taken_then_not_taken_tage();
    run_trace_3_3_tage();
    run_trace_5_2_tage();
    run_trace_8_8_tage();
    run_trace_xor_tage();
}
