#include "predictors.h"
#include "traces.h"

#include <cstddef>
#include <vector>

#define FACTOR 10
#define IW 9
#define TW 7
// magic number: 1024 << 15 ~= 3e7 - size of average CBP benchmark 

// ---- Helpers ----
void print_score(BranchPredictorBase* predictor, std::string title, size_t score, size_t seq_len) {
    double perc = 100 * (double)score / (double)seq_len;
    std::cout << "[" << *predictor << "] " << title << ": Predicted " << score << " out of " << seq_len << " correctly (" << perc << "%)" << std::endl;
}

size_t num_reps = 1024 << FACTOR;

// ---- Generic trace functions ----
std::vector<BranchInstr> v_trace_1_0 = repeating_pattern(1, 1, num_reps);
std::vector<BranchInstr> v_trace_taken_then_not_taken = repeating_pattern(num_reps, num_reps, 1);
std::vector<BranchInstr> v_trace_3_3 = repeating_pattern(3, 3, num_reps);
std::vector<BranchInstr> v_trace_6_2 = repeating_pattern(6, 2, num_reps);
std::vector<BranchInstr> v_trace_8_8 = repeating_pattern(8, 8, num_reps);
std::vector<BranchInstr> v_trace_corr = correlated_branch(num_reps, 0, 0, 1, 1, false);
std::vector<BranchInstr> v_trace_anti_corr = correlated_branch(num_reps, 0, 0, 1, 1, true);
std::vector<BranchInstr> v_trace_xor = xor_correlated_branch(num_reps, 0, 0, 1, 1, 2, 2);
std::vector<BranchInstr> v_trace_imitate = imitate_branch(num_reps, 0, 0);

void trace_1_0(BranchPredictorBase* predictor) {
    // alternating taken-not taken
    size_t num_reps = 1024 << FACTOR;
    size_t seq_len = 2 * num_reps;
    size_t score = 0;
    // std::vector<BranchInstr> trace = repeating_pattern(1, 1, num_reps);

    for (const auto& branch : v_trace_1_0) {
        bool pred = predictor->predict(branch.pc);
        score += pred == branch.direction;
        predictor->update(branch.pc, branch.direction);
    }

    print_score(predictor, "trace_1_0", score, seq_len);
}

void trace_taken_then_not_taken(BranchPredictorBase* predictor) { 
    // many taken followed by not takens
    size_t num_dir = 1024 << FACTOR;
    size_t seq_len = 2 * num_dir;
    size_t score = 0;
    // std::vector<BranchInstr> trace = repeating_pattern(num_dir, num_dir, 1);

    for (const auto& branch : v_trace_taken_then_not_taken) {
        bool pred = predictor->predict(branch.pc);
        score += pred == branch.direction;
        predictor->update(branch.pc, branch.direction);
    }

    print_score(predictor, "trace_taken_then_not_taken", score, seq_len);
}

void trace_3_3(BranchPredictorBase* predictor) { 
    // 3 taken followed by 3 not taken
    size_t num_reps = 1024 << FACTOR;
    size_t seq_len = 6 * num_reps;
    size_t score = 0;
    std::vector<BranchInstr> trace = repeating_pattern(3, 3, num_reps);

    for (const auto& branch : trace) {
        bool pred = predictor->predict(branch.pc);
        score += pred == branch.direction;
        predictor->update(branch.pc, branch.direction);
    }

    print_score(predictor, "trace_3_3", score, seq_len);
}

void trace_6_2(BranchPredictorBase* predictor) { 
    // 6 taken followed by 2 not taken
    size_t num_reps = 1024 << FACTOR;
    // size_t num_reps = 256;
    size_t seq_len = 8 * num_reps;
    size_t score = 0;
    // std::vector<BranchInstr> trace = repeating_pattern(6, 2, num_reps);

    for (const auto& branch : v_trace_6_2) {
        bool pred = predictor->predict(branch.pc);
        score += pred == branch.direction;
        predictor->update(branch.pc, branch.direction);
    }

    print_score(predictor, "trace_6_2", score, seq_len);
}

void trace_8_8(BranchPredictorBase* predictor) {
    // 8 taken followed by 8 not taken
    size_t num_reps = 1024 << FACTOR;
    size_t seq_len = 16 * num_reps;
    size_t score = 0;
    // std::vector<BranchInstr> trace = repeating_pattern(8, 8, num_reps);

    for (const auto& branch : v_trace_8_8) {
        bool pred = predictor->predict(branch.pc);
        score += pred == branch.direction;
        predictor->update(branch.pc, branch.direction);
    }

    print_score(predictor, "trace_8_8", score, seq_len);
}

void trace_corr(BranchPredictorBase* predictor) {
    size_t num_reps = 1024 << FACTOR;
    size_t score = 0;
    // std::vector<BranchInstr> trace = correlated_branch(num_reps, 0, 0, 1, 1, false);

    for (const auto& branch : v_trace_corr) {
        bool pred = predictor->predict(branch.pc);
        if (branch.id == 1) {
            score += pred == branch.direction;
        }
        predictor->update(branch.pc, branch.direction);
    }

    print_score(predictor, "trace_corr", score, num_reps);
}

void trace_anti_corr(BranchPredictorBase* predictor) {
    size_t num_reps = 1024 << FACTOR;
    size_t score = 0;
    // std::vector<BranchInstr> trace = correlated_branch(num_reps, 0, 0, 1, 1, true);

    for (const auto& branch : v_trace_anti_corr) {
        bool pred = predictor->predict(branch.pc);
        if (branch.id == 1) {
            score += pred == branch.direction;
        }
        predictor->update(branch.pc, branch.direction);
    }

    print_score(predictor, "trace_anti_corr", score, num_reps);
}

void trace_xor(BranchPredictorBase* predictor) {
    size_t num_reps = 1024 << FACTOR;
    size_t score = 0;
    // std::vector<BranchInstr> trace = xor_correlated_branch(num_reps, 0, 0, 1, 1, 2, 2);

    for (const auto& branch : v_trace_xor) {
        bool pred = predictor->predict(branch.pc);
        if (branch.id == 2) {
            score += pred == branch.direction;
        }
        predictor->update(branch.pc, branch.direction);
    }

    print_score(predictor, "trace_xor", score, num_reps);
}

void trace_imitate(BranchPredictorBase* predictor) {
    size_t num_reps = 1024 << FACTOR;
    size_t score = 0;
    // std::vector<BranchInstr> trace = imitate_branch(num_reps, 0, 0);

    for (const auto& branch : v_trace_imitate) {
        bool pred = predictor->predict(branch.pc);
        score += pred == branch.direction;
        predictor->update(branch.pc, branch.direction);
    }

    print_score(predictor, "trace_imitate", score, v_trace_imitate.size());
}

// ---- Wrapper functions (predictor + trace type) ----

void run_trace_1_0_bimodal() {
    BimodalPredictor predictor(TW);
    trace_1_0(&predictor);
}

void run_trace_taken_then_not_taken_bimodal() {
    BimodalPredictor predictor(TW);
    trace_taken_then_not_taken(&predictor);
}

void run_trace_3_3_bimodal() {
    BimodalPredictor predictor(TW);
    trace_3_3(&predictor);
}

void run_trace_6_2_bimodal() {
    BimodalPredictor predictor(TW);
    trace_6_2(&predictor);
}

void run_trace_8_8_bimodal() {
    BimodalPredictor predictor(TW);
    trace_8_8(&predictor);
}

void run_trace_corr_gshare() {
    GSharePredictor predictor(TW, 2);
    trace_corr(&predictor);
}

void run_trace_anti_corr_gshare() {
    GSharePredictor predictor(TW, 2);
    trace_anti_corr(&predictor);
}

void run_trace_xor_gshare() {
    GSharePredictor predictor(TW, 8);
    trace_xor(&predictor);
}

void run_trace_1_0_gshare() {
    GSharePredictor predictor(TW, 3);
    trace_1_0(&predictor);
}

void run_trace_taken_then_not_taken_gshare() {
    GSharePredictor predictor(TW, 3);
    trace_taken_then_not_taken(&predictor);
}

void run_trace_6_2_gshare() {
    GSharePredictor predictor(TW, 3);
    trace_6_2(&predictor);
}

void run_trace_8_8_gshare() {
    GSharePredictor predictor(TW, 3);
    trace_8_8(&predictor);
}

// Two-level predictor runners

void run_trace_1_0_twolvl() {
    TwoLevelPredictor predictor(TW, 2);
    trace_1_0(&predictor);
}

void run_trace_taken_then_not_taken_twolvl() {
    TwoLevelPredictor predictor(TW, 2);
    trace_taken_then_not_taken(&predictor);
}

void run_trace_3_3_twolvl() {
    TwoLevelPredictor predictor(TW, 3);
    trace_3_3(&predictor);
}

void run_trace_6_2_twolvl() {
    TwoLevelPredictor predictor(TW, 3);
    trace_6_2(&predictor);
}

void run_trace_8_8_twolvl() {
    TwoLevelPredictor predictor(TW, 2);
    trace_8_8(&predictor);
}

void run_trace_xor_twolvl() {
    TwoLevelPredictor predictor(TW, 2);
    trace_xor(&predictor);
}

// tournament runners

void run_trace_1_0_tourn() {
    TwoLevelPredictor predictor1(TW, 2);
    GSharePredictor predictor2(TW, 3);
    TournamentPredictor<GSharePredictor, TwoLevelPredictor> predictor(predictor2, predictor1, 4);
    trace_1_0(&predictor);
}

void run_trace_taken_then_not_taken_tourn() {
    TwoLevelPredictor predictor1(TW, 2);
    GSharePredictor predictor2(TW, 3);
    TournamentPredictor<GSharePredictor, TwoLevelPredictor> predictor(predictor2, predictor1, 4);
    trace_taken_then_not_taken(&predictor);
}

void run_trace_3_3_tourn() {
    TwoLevelPredictor predictor1(TW, 2);
    GSharePredictor predictor2(TW, 3);
    TournamentPredictor<GSharePredictor, TwoLevelPredictor> predictor(predictor2, predictor1, 4);
    trace_3_3(&predictor);
}

void run_trace_6_2_tourn() {
    TwoLevelPredictor predictor1(TW, 8);
    GSharePredictor predictor2(TW, 8);
    TournamentPredictor<GSharePredictor, TwoLevelPredictor> predictor(predictor2, predictor1, 4);
    trace_6_2(&predictor);
}

void run_trace_8_8_tourn() {
    TwoLevelPredictor predictor1(TW, 8);
    GSharePredictor predictor2(TW, 8);
    TournamentPredictor<GSharePredictor, TwoLevelPredictor> predictor(predictor2, predictor1, 4);
    trace_8_8(&predictor);
}

void run_trace_xor_tourn() {
    TwoLevelPredictor predictor1(TW, 2);
    GSharePredictor predictor2(TW, 3);
    TournamentPredictor<GSharePredictor, TwoLevelPredictor> predictor(predictor2, predictor1, 4);
    trace_xor(&predictor);
}

void run_trace_imitate_tourn() {
    TwoLevelPredictor predictor1(TW, 2);
    GSharePredictor predictor2(TW, 3);
    TournamentPredictor<GSharePredictor, TwoLevelPredictor> predictor(predictor2, predictor1, 4);
    trace_imitate(&predictor);
}

// perceptron runners

#define PERCEPTRON_HIST 32

void run_trace_1_0_perc() {
    PerceptronPredictor predictor(TW, PERCEPTRON_HIST);
    trace_1_0(&predictor);
}

void run_trace_taken_then_not_taken_perc() {
    PerceptronPredictor predictor(TW, PERCEPTRON_HIST);
    trace_taken_then_not_taken(&predictor);
}

void run_trace_3_3_perc() {
    PerceptronPredictor predictor(TW, PERCEPTRON_HIST);
    trace_3_3(&predictor);
}

void run_trace_6_2_perc() {
    PerceptronPredictor predictor(TW, PERCEPTRON_HIST);
    trace_6_2(&predictor);
}

void run_trace_8_8_perc() {
    PerceptronPredictor predictor(TW, PERCEPTRON_HIST);
    trace_8_8(&predictor);
}

void run_trace_xor_perc() {
    PerceptronPredictor predictor(TW, PERCEPTRON_HIST);
    trace_xor(&predictor);
}

void run_trace_imitate_perc() {
    PerceptronPredictor predictor(TW, PERCEPTRON_HIST);
    trace_imitate(&predictor);
}

// TAGE runners

#define NUM_COMP 5
#define L1 4
#define R 2
#define H_SIZE 1024

void run_trace_1_0_tage() {
    TAGEPredictor<H_SIZE, NUM_COMP> predictor(IW, TW, NUM_COMP, L1, R);
    trace_1_0(&predictor);
}

void run_trace_taken_then_not_taken_tage() {
    TAGEPredictor<H_SIZE, NUM_COMP> predictor(IW, TW, NUM_COMP, L1, R);
    trace_taken_then_not_taken(&predictor);
}

void run_trace_3_3_tage() {
    TAGEPredictor<H_SIZE, NUM_COMP> predictor(IW, TW, NUM_COMP, L1, R);
    trace_3_3(&predictor);
}

void run_trace_6_2_tage() {
    TAGEPredictor<H_SIZE, NUM_COMP> predictor(IW, TW, NUM_COMP, L1, R);
    trace_6_2(&predictor);
}

void run_trace_8_8_tage() {
    TAGEPredictor<H_SIZE, NUM_COMP> predictor(IW, TW, NUM_COMP, L1, R);
    trace_8_8(&predictor);
}

void run_trace_xor_tage() {
    TAGEPredictor<H_SIZE, NUM_COMP> predictor(IW, TW, NUM_COMP, L1, R);
    trace_xor(&predictor);
}

void run_trace_imitate_tage() {
    TAGEPredictor<H_SIZE, NUM_COMP> predictor(IW, TW, NUM_COMP, L1, R);
    trace_imitate(&predictor);
}

int main() {
    std::cout << "======Bimodal sanity checks======" << std::endl;
    run_trace_1_0_bimodal();
    run_trace_taken_then_not_taken_bimodal();
    run_trace_3_3_bimodal();
    run_trace_6_2_bimodal();
    run_trace_8_8_bimodal();

    std::cout << "======GShare sanity checks======" << std::endl;
    run_trace_1_0_gshare();
    run_trace_taken_then_not_taken_gshare();
    run_trace_6_2_gshare();
    run_trace_8_8_gshare();
    run_trace_xor_gshare();
    run_trace_corr_gshare();
    run_trace_anti_corr_gshare();

    std::cout << "======Two-level sanity checks======" << std::endl;
    run_trace_1_0_twolvl();
    run_trace_taken_then_not_taken_twolvl();
    run_trace_3_3_twolvl();
    run_trace_6_2_twolvl();
    run_trace_8_8_twolvl();
    run_trace_xor_twolvl();

    std::cout << "======Tournament sanity checks======" << std::endl;
    run_trace_1_0_tourn();
    run_trace_taken_then_not_taken_tourn();
    run_trace_3_3_tourn();
    run_trace_6_2_tourn();
    run_trace_8_8_tourn();
    run_trace_xor_tourn();
    run_trace_imitate_tourn();

    std::cout << "======Perceptron sanity checks======" << std::endl;
    run_trace_1_0_perc();
    run_trace_taken_then_not_taken_perc();
    run_trace_3_3_perc();
    run_trace_6_2_perc();
    run_trace_8_8_perc();
    run_trace_xor_perc();
    run_trace_imitate_perc();

    std::cout << "======TAGE sanity checks======" << std::endl;
    run_trace_1_0_tage();
    run_trace_taken_then_not_taken_tage();
    run_trace_3_3_tage();
    run_trace_6_2_tage();
    run_trace_8_8_tage();
    run_trace_xor_tage();
    run_trace_imitate_tage();
}
