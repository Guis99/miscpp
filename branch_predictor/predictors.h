#pragma once

#include <array>
#include <cassert>
#include <cstdint>
#include <exception>
#include <iostream>
#include <stdexcept>
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
    size_t table_size;
    size_t pc_mask;

    std::vector<SatCounter<2>> pred_table;
    size_t get_idx(size_t pc) const {
        return pc & pc_mask;
    }
    public:
        explicit BimodalPredictor(size_t table_size) :
                table_size(1ull << table_size), pc_mask(this->table_size - 1),
                pred_table(this->table_size, SatCounter<2>()) {
        }

        void update(uint32_t pc, BranchResult branch) override {
            pred_table[get_idx(pc)].update(branch);
        }

        bool predict(uint32_t pc) override {
            return pred_table[get_idx(pc)].predict();
        }

        std::string predictor_name() const override { return "Bimodal"; }
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
                table_size(1ull << table_size), history_length(1ull << history_length),
                pc_mask(this->table_size - 1), history_mask(this->history_length - 1),
                pred_table(this->table_size, SatCounter<2>()),
                history_buffer() {}

        uint64_t get_idx(uint32_t pc) const {
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

        std::string predictor_name() const override { return "GShare"; }
};

class GSelectPredictor : public BranchPredictorBase {
    public:
        GSelectPredictor(size_t table_size, size_t history_length) {}

        bool predict(uint32_t pc) override { return false; }
        void update(uint32_t pc, BranchResult branch) override {}
        std::string predictor_name() const override { return "GSelect"; }
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
                table_size(1ull << table_size), history_length(1ull << history_length), 
                pc_mask(this->table_size - 1), history_mask(this->history_length - 1),
                pred_table(this->table_size, std::vector<SatCounter<2>>(1ull << history_length)),
                branch_history_table(this->table_size, HistoryBuffer<64>()) {}

        uint64_t get_pc_idx(uint32_t pc) const {
            return pc & pc_mask;
        }

        uint64_t get_history_idx(size_t history) const {
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

        std::string predictor_name() const override { return "TwoLevel"; }
};

template <class Pred1, class Pred2>
class TournamentPredictor : public BranchPredictorBase {
    Pred1& predictor_1;
    Pred2& predictor_2;
    size_t table_size;
    size_t pc_mask;
    std::vector<SatCounter<2>> meta_pred_table;;
    bool last_pred_1 = false;
    bool last_pred_2 = false;
    public:
        TournamentPredictor(Pred1& predictor_1, Pred2& predictor_2, size_t table_size) :
                predictor_1(predictor_1), 
                predictor_2(predictor_2),
                table_size(1ull << table_size), pc_mask(this->table_size - 1),
                meta_pred_table(this->table_size, SatCounter<2>()) {}

        size_t get_idx(size_t pc) const { return pc & pc_mask; }

        void update(uint32_t pc, BranchResult branch) override {
            size_t pc_idx = get_idx(pc);
            
            bool pred1_correct = last_pred_1 == branch;
            bool pred2_correct = last_pred_2 == branch;

            if (pred1_correct && !pred2_correct) {
                meta_pred_table[pc_idx].update(BranchResult::TAKEN);
            } else if (!pred1_correct && pred2_correct) {
                meta_pred_table[pc_idx].update(BranchResult::NOT_TAKEN);
            }

            predictor_1.update(pc, branch);
            predictor_2.update(pc, branch);
        }

        bool predict(uint32_t pc) override {
            size_t pc_idx = get_idx(pc);

            last_pred_1 = predictor_1.predict(pc);
            last_pred_2 = predictor_2.predict(pc);

            bool meta_prediction = meta_pred_table[pc_idx].predict();
            if (meta_prediction) {
                return last_pred_1;
            } else {
                return last_pred_2;
            }
        }

        std::string predictor_name() const override { return "Tournament(" + predictor_1.predictor_name() + "," + predictor_2.predictor_name() + ")"; }
};

class PerceptronPredictor : public BranchPredictorBase {
    size_t table_size;
    size_t history_length;
    size_t pc_mask;
    size_t history_mask;
    std::vector<Perceptron> pred_table;
    HistoryBuffer<64> history_buffer;

    size_t get_idx(size_t pc) {
        return pc & pc_mask;
    }

    public:
        PerceptronPredictor(size_t table_size, size_t history_length) :
                table_size(1ull << table_size), history_length(history_length),
                pc_mask(this->table_size - 1), history_mask((1ull << history_length) - 1),
                pred_table(this->table_size, Perceptron(history_length)),
                history_buffer() {}

        void update(uint32_t pc, BranchResult branch) override {
            pred_table[get_idx(pc)].update(branch, history_buffer.as_idx() & history_mask, history_length);
            history_buffer.update(branch);
        }

        bool predict(uint32_t pc) override {
            return pred_table[get_idx(pc)].predict(history_buffer.as_idx() & history_mask, history_length);
        }

        std::string predictor_name() const override { return "PerceptronPredictor"; }
};

class TAGEPredictor : public BranchPredictorBase {
    size_t _idx_width;
    size_t _tag_width;
    size_t _table_size;
    size_t _pc_mask;
    size_t _tag_mask;

    HistoryBuffer<256> _history_buffer;
    std::vector<SatCounter<2>> _pred_table;
    std::vector<std::vector<TagEntry>> _banks;
    std::vector<uint16_t> _history_lengths;

    std::vector<uint16_t> _idx_cache;

    size_t _branches_seen = 0;
    int8_t _longest_match_idx = -1;

    bool _top_pred = false;
    bool _alt_pred = false;

    size_t _get_idx(size_t pc) {
        return pc & _pc_mask;
    }

    void _allocate_new(bool should_flip) {
        bool allocated = false;
        for (uint8_t i = _longest_match_idx+1; i < _banks.size(); i++) {
            auto b_idx = _idx_cache[i];
            auto b_useful = _banks[i][b_idx].get_u();
            if (b_useful == 0 && !allocated) {
                auto b_tag = _idx_cache[i+_banks.size()];
                _banks[i][b_idx].set_tag(b_tag);
                _banks[i][b_idx].set_ctr(3 + should_flip);
                _banks[i][b_idx].set_u(0);
                allocated = true;
                break;
            }
        }

        if (!allocated) {
            for (uint8_t i = _longest_match_idx+1; i < _banks.size(); i++) {
                auto b_idx = _idx_cache[i];
                _banks[i][b_idx].update_u(BranchResult::NOT_TAKEN);
            }
        }
    }

    void _reset_all_banks() {
        for (auto& bank : _banks) {
            for (auto& entry : bank) {
                entry.set_u(0);
            }
        }
    }
    
    public:
        TAGEPredictor(size_t idx_width, size_t tag_width, uint8_t num_comp, uint16_t L1) :
                _idx_width(idx_width), _tag_width(tag_width), 
                _table_size(1ull << idx_width), 
                _pc_mask(_table_size - 1), _tag_mask((1ull << tag_width) - 1),
                _history_buffer(),
                _pred_table(_table_size, SatCounter<2>()), 
                _banks(num_comp, std::vector<TagEntry>(_table_size, TagEntry()))
            {
                if (L1 << (num_comp - 1) > 256) {
                    throw(std::runtime_error(""));
                }
                _history_lengths.reserve(num_comp);
                _history_lengths.push_back(L1);
                for (int i = 1; i < num_comp; i++) {
                    L1 <<= 1;
                    _history_lengths.push_back(L1);           
                }
                
                _idx_cache.resize(2*num_comp, 0);
        }

        void update(uint32_t pc, BranchResult branch) override {
            bool provider_corr = _top_pred == branch;
            bool alt_corr = _alt_pred == branch;
            
            if (!provider_corr) {
                _allocate_new(branch);
            }

            if (_longest_match_idx > -1) {
                auto pred_idx = _idx_cache[_longest_match_idx];
                _banks[_longest_match_idx][pred_idx].update_ctr(branch);
                if (provider_corr && !alt_corr) {
                    _banks[_longest_match_idx][pred_idx].update_u(BranchResult::TAKEN);
                } else if (!provider_corr && alt_corr) {
                    _banks[_longest_match_idx][pred_idx].update_u(BranchResult::NOT_TAKEN);
                }

            } else {
                _pred_table[_get_idx(pc)].update(branch);
            }

            _history_buffer.update(branch);

            if (_branches_seen++ > 256'000) {
                _reset_all_banks();
                _branches_seen = 0;
            }
        }

        bool predict(uint32_t pc) override {
            _top_pred = _pred_table[_get_idx(pc)].predict();
            _alt_pred = _top_pred;
            _longest_match_idx = -1;
            for (int i = 0; i < _banks.size(); i++) {
                uint16_t hist_len = _history_lengths[i];

                uint16_t idx = _history_buffer.as_folded_idx(pc, _pc_mask, _idx_width, hist_len);
                uint16_t tag = _history_buffer.as_folded_idx(pc, _tag_mask, _tag_width, hist_len);

                _idx_cache[i] = idx;
                _idx_cache[i+_banks.size()] = tag;

                bool pred = _banks[i][idx].predict();
                bool tag_match = _banks[i][idx].match_tag(tag);
                if (tag_match) {
                    _alt_pred = _top_pred;
                    _top_pred = pred;
                    _longest_match_idx = i;
                }
            }

            return _top_pred;
        }

        std::string predictor_name() const override { return "TAGE"; }
};