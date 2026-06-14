#pragma once

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

#include "primitives.h"

class BranchPredictorBase {
public:
    virtual bool predict(uint32_t pc) = 0;
    virtual void update(uint32_t pc, BranchResult branch) = 0;
    virtual std::string predictor_name() const = 0;
    virtual ~BranchPredictorBase() = default;

    friend std::ostream& operator<<(std::ostream& os, const BranchPredictorBase& p) {
        return os << p.predictor_name();
    }
};

class BimodalPredictor : public BranchPredictorBase {
    std::vector<SatCounter<2>> pred_table;
    size_t table_size;
    public:
        BimodalPredictor(size_t table_size) :
                table_size(table_size) {
            pred_table.resize(table_size);
        }

        void update(uint32_t pc, BranchResult branch) override {
            pred_table[pc % table_size].update(branch);
        }

        bool predict(uint32_t pc) override {
            return pred_table[pc % table_size].predict();
        }

        std::string predictor_name() const override { return "BimodalPredictor"; }
};

class GSharePredictor : public BranchPredictorBase {
    size_t table_size;
    size_t history_length;
    size_t pc_mask;
    size_t history_mask;

    std::vector<SatCounter<2>> pred_table;
    HistoryBuffer<64> history_buffer;

    public:
        GSharePredictor(size_t table_size, size_t history_length) :
                table_size(table_size), pc_mask((1ull << table_size) - 1),
                history_length(history_length), history_mask((1ull << history_length) - 1),
                pred_table(table_size, SatCounter<2>()),
                history_buffer() {}

        uint64_t get_idx(uint32_t pc) {
            size_t history_idx = history_buffer.as_idx() & history_mask; // select lower *history_length* bits
            return (pc ^ history_idx) & pc_mask; // canonical gshare idx construction, select lower table_size bits
        }

        void update(uint32_t pc, BranchResult branch) override {
            pred_table[get_idx(pc)].update(branch);
            history_buffer.update(branch);
        }

        bool predict(uint32_t pc) override {
            return pred_table[get_idx(pc)].predict();
        }

        std::string predictor_name() const override { return "GSharePredictor"; }
};

class GSelectPredictor : public BranchPredictorBase {
    public:
        GSelectPredictor(size_t table_size, size_t history_length) {}

        bool predict(uint32_t pc) override { return false; }
        void update(uint32_t pc, BranchResult branch) override {}
        std::string predictor_name() const override { return "GSelectPredictor"; }
};

class TwoLevelPredictor : public BranchPredictorBase {
    size_t table_size;
    size_t history_length;
    size_t pc_mask;
    size_t history_mask;

    std::vector<std::vector<SatCounter<2>>> pred_table;
    std::vector<HistoryBuffer<64>> branch_history_table;

    public:
        TwoLevelPredictor(size_t table_size, size_t history_length) :
        table_size(table_size), pc_mask((1ull << table_size) - 1),
        history_length(history_length), history_mask((1ull << history_length) - 1),
        pred_table(table_size, std::vector<SatCounter<2>>(1ull << history_length)),
        branch_history_table(table_size, HistoryBuffer<64>()) {}

        uint64_t get_pc_idx(uint32_t pc) {
            return pc & pc_mask;
        }

        uint64_t get_history_idx(size_t history) {
            return branch_history_table[history].as_idx() & history_mask;
        }

        void update(uint32_t pc, BranchResult branch) override {
            size_t pc_idx = get_pc_idx(pc);
            size_t history_idx = get_history_idx(pc_idx);

            branch_history_table[pc_idx].update(branch); // update branch history
            pred_table[pc_idx][history_idx].update(branch); // update counter
        }

        bool predict(uint32_t pc) override {
            size_t pc_idx = get_pc_idx(pc);
            size_t history_idx = get_history_idx(pc_idx);

            return pred_table[pc_idx][history_idx].predict();
        }

        std::string predictor_name() const override { return "TwoLevelPredictor"; }
};

class TournamentPredictor : public BranchPredictorBase {
    size_t table_size;
    size_t history_length;
    // GShare-specific
    std::vector<SatCounter<2>> pred_table;
    HistoryBuffer<64> history_buffer;
    // two-level-specific
    std::vector<HistoryBuffer<64>> branch_history_table;
    // tournament selector
    std::vector<SatCounter<2>> meta_pred_table;
    public:
        TournamentPredictor(size_t table_size, size_t history_length) :
            table_size(table_size), history_length(history_length) {}

        uint64_t get_idx(uint32_t pc) { return 0; }

        void update(uint32_t pc, BranchResult branch) override {
            size_t pc_idx = get_idx(pc);
            branch_history_table[pc_idx].update(branch);
        }

        bool predict(uint32_t pc) override { return false; }

        std::string predictor_name() const override { return "TournamentPredictor"; }
};
