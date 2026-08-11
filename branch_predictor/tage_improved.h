#pragma once

#include "predictors.h"

// ---------------------------------------------------------------------------
// TAGEImproved: iteration target for the advanced techniques (interleaved
// banks, statistical correction, loop predictor, ...).
// ---------------------------------------------------------------------------
// template <int H, int NC>
// class TAGEImproved : public TAGEPredictor<H, NC> {
// public:
//     using TAGEPredictor<H, NC>::TAGEPredictor;  // inherit the constructor

//     std::string predictor_name() const override { return "TAGEImproved"; }
// };

template <int H, int N_L, int N_U>
class TAGEImproved : public BranchPredictorBase {
    // requires (H > 0, N_L > 0, N_U > 0);
    static constexpr int NC = N_L+N_U;
    size_t _idx_width;
    size_t _tag_width_1;
    size_t _tag_width_2;
    size_t _table_size;
    size_t _pc_mask;

    size_t _tag_mask_1;
    size_t _tag_mask_2;

    size_t _nb_1; // num_banks
    size_t _nb_2;

    size_t _ne_1; // num_entries
    size_t _ne_2;

    HistoryBuffer<H> _history_buffer;
    std::vector<SatCounter<2>> _base_table;
    // std::vector<std::vector<TagEntry>> _banks;
    std::array<std::vector<TagEntry>, 2> _banks = {};

    std::array<csr, 2*NC> _hashes = {};
    std::array<u16, NC> _history_lengths = {};
    std::array<u16, 2*NC> _idx_cache = {};

    SatCounter<4> _aon_tracker = {};

    size_t _branches_seen = 0;
    int8_t _top_idx = -1;

    bool _alt_on_new = false;
    bool _top_pred = false;
    bool _alt_pred = false;
    public:
        TAGEImproved(size_t idx_width, size_t tag_width, u16 L1, u8, float ratio) :
                _idx_width(idx_width), _tag_width_1(tag_width), _tag_width_2(tag_width), 
                _table_size(1ull << idx_width), 
                _pc_mask(_table_size - 1), _tag_mask_1((1ull << _tag_width_1) - 1), _tag_mask_2((1ull << _tag_width_2) - 1),
                _history_buffer(),
                _base_table(_table_size, SatCounter<2>())
            {
                float factor = 1.0;
                for (int i = 0; i < NC; i++) {
                    u16 hl = L1 * factor;
                    if (hl > H) {
                        char err_msg[100];
                        snprintf(err_msg, 60, "%d is greater than the max history length of %d", hl, H);
                        throw(std::runtime_error(err_msg));
                    }
                    _history_lengths[i] = hl;    
                    _hashes[2*i] = csr(_pc_mask, _idx_width, (hl - 1) % idx_width);
                    _hashes[2*i+1] = csr(_tag_mask_1, _tag_width_1, (hl - 1) % tag_width); // come back and fix
                    factor *= ratio;    
                }           
        }
        bool predict(u64 pc) override {
            return true;
        }



        void update(u64 pc, BranchResult branch) override {

        }

        std::string predictor_name() const override { return "TAGEImproved"; }

        u64 get_size() const override {
            u64 base_tables = _table_size * 2;
            u64 upper_b = _ne_1 * _nb_1 * (3 + 2 + 1 + _tag_width_1); // 3 bit ctr, 2 u-bits, 1 allocated bit, tag bits
            u64 lower_b = _ne_2 * _nb_2 * (3 + 2 + 1 + _tag_width_2);
            u64 csrs = N_L * (_tag_width_1 + _idx_width) + N_U * (_tag_width_2 + _idx_width); 
            return base_tables + upper_b + lower_b;
        }
};

