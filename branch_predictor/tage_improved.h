#pragma once

#include "predictors.h"
#include <cstdlib>
#include <cmath>
// #undef TAGE_STATS

// ---------------------------------------------------------------------------
// TAGEImproved: iteration target for the advanced techniques (interleaved
// banks, statistical correction, loop predictor, ...).
// ---------------------------------------------------------------------------

template <int N_L, int N_U, int NC>
// N_L: number of lower banks
// N_U: number of upper banks
// NC: number of history lengths
class TAGEImproved : public BranchPredictorBase {
    constexpr static int nLookups = 2 * NC;

    size_t _base_idx_width;
    size_t _tage_idx_width;

    size_t _short_tag_width;
    size_t _long_tag_width;
    size_t _table_size;
    
    size_t _tag_mask_1;
    size_t _tag_mask_2;

    size_t _base_idx_mask;
    size_t _tage_idx_mask;          

    u64 _firstLongTagTable;

    HistoryBuffer<3000> _history_buffer;
    std::vector<SatCounter<2>> _base_table;
    std::array<std::vector<TagEntry>, 2> _banks = {};

    // maps banks 1..firstLongTag-1 to 1, firstLongTag..nLookups to firstLongTag
    std::array<int, nLookups+1> _bank_map = {}; 

    // self-explanatory
    std::array<int, nLookups+1> _tagWidths = {};
    std::array<u64, nLookups+1> _tag_masks = {};

    // circular shift registers; keep folded precursor hashes for computing idx, tag
    std::array<csr, nLookups+1> _idx_hashes = {};
    std::array<csr, nLookups+1> _tag_hashes = {};
    std::array<csr, nLookups+1> _tag_hashes_2 = {};

    std::array<u16, nLookups+1> _history_lengths = {};

    // store indices and tags from predict() stage to use in update()
    std::array<u16, nLookups+1> _idx_cache = {};
    std::array<u16, nLookups+1> _tag_cache = {};
    // std::array<u16, nLookups+1> _tag_cache_2 = {};

    // turn off subset of predictors 
    std::array<bool, nLookups+1> _noSkip = false;

    // aon = alloc_on_new, metacounter for deciding to use aon
    SatCounter<4> _aon_tracker = {};

    size_t _branches_seen = 0;
    int8_t _top_idx = 0;

    const u8 _shift_amt = 2;

    bool _alt_on_new = false;
    bool _top_pred = false;
    bool _alt_pred = false;

    #ifdef TAGE_STATS
        // temporary diagnostics, slot 0 = bimodal base, slot i+1 = bank i
        std::array<u64, nLookups+1> _stat_prov_cnt = {};
        std::array<u64, nLookups+1> _stat_prov_miss = {};
        u64 _stat_total = 0;
        u64 _stat_alt_used = 0, _stat_alt_used_miss = 0, _stat_alt_prov_miss = 0;
        u64 _stat_alt_accepted = 0, _stat_alt_rejected = 0;
        u64 _stat_alloc_attempt = 0, _stat_alloc_success = 0, _stat_resets = 0;
    #endif

    size_t _get_idx(size_t pc) const {
        return pc & _base_idx_mask;
    }

    void _reset_all_banks() {
        #ifdef TAGE_STATS
                _stat_resets++;
        #endif
        for (auto& bank : _banks) {
            for (auto& entry : bank) {
                entry.clear();
            }
        }
    }

    void _allocate_new(bool should_flip) {
        bool allocated = false;
        #ifdef TAGE_STATS
                _stat_alloc_attempt++;
        #endif
            for (u8 i = _top_idx+1; i <= nLookups; i++) {
                auto b_idx = _idx_cache[i];
                auto b_useful = _banks[_get_mapped_idx(i)][b_idx].get_u();
                if (b_useful == 0 && !allocated) {
                    auto b_tag = _tag_cache[i];
                    _banks[_get_mapped_idx(i)][b_idx].allocate(b_tag, should_flip);
                    allocated = true;
        #ifdef TAGE_STATS
                        _stat_alloc_success++;
        #endif
                    break;
                }
            }

        if (!allocated) {
            for (u8 i = _top_idx+1; i <= nLookups; i++) {
                auto b_idx = _idx_cache[i];
                _banks[_get_mapped_idx(i)][b_idx].update_u(BranchResult::NOT_TAKEN);
            }
        }
    }

    bool _is_entry_new(u16 top_idx) const {
        bool u_zeroed = _banks[_get_mapped_idx(_top_idx)][top_idx].get_u() == 0;
        bool ctr_weak = _banks[_get_mapped_idx(_top_idx)][top_idx].get_ctr() == 3 || _banks[_get_mapped_idx(_top_idx)][top_idx].get_ctr() == 4;
        return u_zeroed && ctr_weak;
    }

    int _get_mapped_idx(int bank_idx) const {
        return _bank_map[bank_idx];
    }

    public:
        TAGEImproved(size_t idx_width_1, size_t idx_width_2, size_t tag_width_1, size_t tag_width_2, 
                    u64 firstLongTagTable,
                    u64 minHist, u64 maxHist, 
                    std::array<bool, nLookups+1> noSkip) :
                _base_idx_width(idx_width_1), _tage_idx_width(idx_width_2), 
                _short_tag_width(tag_width_1), _long_tag_width(tag_width_2), 
                _firstLongTagTable(firstLongTagTable),
                _table_size(1ull << idx_width_1), 
                _base_idx_mask((1ull << idx_width_1) - 1), _tage_idx_mask((1ull << idx_width_2) - 1), 
                _tag_mask_1((1ull << _short_tag_width) - 1), _tag_mask_2((1ull << _long_tag_width) - 1),
                _history_buffer(),
                _base_table(_table_size, SatCounter<2>()), 
                _noSkip(noSkip)
            {
                // std::cout << "initializing history lengths" << std::endl;
                _history_lengths[1] = minHist;
                _history_lengths[NC] = maxHist;

                // std::cout << "[init]history length at index " << 1 << ": " << minHist << std::endl;
                
                for (int i = 2; i <= NC; i++) {
                    
                    _history_lengths[i] = (int) (((double) minHist *
                                pow ((double) (maxHist) / (double) minHist,
                                    (double) (i - 1) / (double) ((NC - 1))))
                                + 0.5);

                    // std::cout << "[init]history length at index " << i << ": " << _history_lengths[i] << std::endl;
                }    

                // std::cout << "[init]history length at index " << NC << ": " << maxHist << std::endl;

                for (int i = nLookups; i > 1; i--) {
                        _history_lengths[i] = _history_lengths[(i + 1) / 2];
                    }    

                for (int i=0; i<=nLookups; i++) {
                    _bank_map[i] = i < _firstLongTagTable ? 0 : 1; // allocate only tables 0 and 1
                }

                for (int i = 1; i <= nLookups; i++) {
                    _tagWidths[i] = 
                        i < _firstLongTagTable ? tag_width_1 : tag_width_2;
                    _tag_masks[i] = 
                        i < _firstLongTagTable ? _tag_mask_1 : _tag_mask_2;

                    u16 hl = _history_lengths[i];
                    _idx_hashes[i] = csr(_tage_idx_mask, _tage_idx_width, (hl - 1) % _tage_idx_width);
                    _tag_hashes[i] = csr(_tag_masks[i], _tagWidths[i], (hl - 1) % _tagWidths[i]);
                    _tag_hashes_2[i] = csr(_tag_masks[i], _tagWidths[i] - 1, (hl - 1) % _tagWidths[i]);
                }

                _banks[0].resize(N_L * (1 << _tage_idx_width));
                _banks[1].resize(N_U * (1 << _tage_idx_width));
        }

        ~TAGEImproved() {}

        bool predict(u64 pc) override {
            _top_pred = _base_table[_get_idx(pc)].predict();
            _alt_pred = _top_pred;
            _top_idx = 0;
            _alt_on_new = false;

            // compute tag and index hashes
            for (int i = 1; i <= nLookups; i+=2) {
                u32 idx_hash = _idx_hashes[i].hash;
                u32 tag_hash = _tag_hashes[i].hash;
                u32 tag_hash_2 = _tag_hashes_2[i].hash;

                u16 idx = (pc ^ (pc >> (std::abs(int(_tage_idx_width - i) + 1))) ^ idx_hash) & _tage_idx_mask;
                u16 tag = (pc ^ tag_hash ^ (tag_hash_2 << 1)) & _tag_masks[i]; // change to array of tag masks?

                _idx_cache[i] = idx;
                _tag_cache[i] = tag;
                
                _idx_cache[i+1] = idx ^ 
                    (tag & _tage_idx_mask);
                _tag_cache[i+1] = tag;
            }

            const u64 shifted_pc = pc >> _shift_amt;

            u64 t = (shifted_pc ^ 
                    ((1 << _history_lengths[_firstLongTagTable]) - 1))
             % N_L;

             for (int i = _firstLongTagTable; i <= nLookups; i++) {
                if (_noSkip[i]) {
                    _idx_cache[i] += (t << _tage_idx_width);
                    t++;
                    t = t % N_L;
                }
            }
 
            t = (shifted_pc ^ 
                            ((1 << _history_lengths[1]) - 1))
                % N_L;
    
            for (int i = 1; i < _firstLongTagTable; i++) {
                if (_noSkip[i]) {
                    _idx_cache[i] += (t << _tage_idx_width);
                    t++;
                    t = t % N_L;
                }
            }

            for (int i=1; i <= nLookups; i++) {
                u16 idx = _idx_cache[i];
                u16 tag = _tag_cache[i];

                bool pred = _banks[_get_mapped_idx(i)][idx].predict();
                bool tag_match = _banks[_get_mapped_idx(i)][idx].match_tag(tag);
                if (tag_match) {
                // if (tag_match && (_history_lengths[i] != _history_lengths[_top_idx])) {
                    _alt_pred = _top_pred;
                    _top_pred = pred;
                    _top_idx = i;
                }
            }

            if (_top_idx > 0) {
                auto top_idx = _idx_cache[_top_idx];
                if (_is_entry_new(top_idx)) {
                    _alt_on_new = true;
                    return _aon_tracker.predict() ? _alt_pred : _top_pred;
                }
            }

            return _top_pred;
        }

        void update_history(u64 pc, BranchResult branch) {
        
        }

        void update(u64 pc, BranchResult branch) override {
            bool provider_corr = _top_pred == branch;
            bool alt_corr = _alt_pred == branch;

            #ifdef TAGE_STATS
            _stat_total++;
            const bool final_pred = _alt_on_new ? _alt_pred : _top_pred;
            // if (_top_idx == 0) { _stat_prov_cnt[] }
            _stat_prov_cnt[_top_idx]++;
            if (final_pred != branch) { _stat_prov_miss[_top_idx]++; }
            if (_alt_on_new) {
                _stat_alt_used++;
                if (_alt_pred != branch) { _stat_alt_used_miss++; }
                if (_top_pred != branch) { _stat_alt_prov_miss++; }
                if (_aon_tracker.predict()) { _stat_alt_accepted++; }
                if (!_aon_tracker.predict()) { _stat_alt_rejected++; }
            }
#endif

            // if (!provider_corr && !_alt_on_new) {
            //     _allocate_new(branch);
            // }

             if (!provider_corr) {
                _allocate_new(branch);
            }

            if (_alt_on_new) {
                _aon_tracker.update(BranchResult(alt_corr));
            }

            if (_top_idx > 0) {
                auto pred_idx = _idx_cache[_top_idx];
                _banks[_get_mapped_idx(_top_idx)][pred_idx].update_ctr(branch);
                if (provider_corr && !alt_corr && !_alt_on_new) {
                    _banks[_get_mapped_idx(_top_idx)][pred_idx].update_u(BranchResult::TAKEN);
                } else if (!provider_corr && alt_corr && !_alt_on_new) {
                    _banks[_get_mapped_idx(_top_idx)][pred_idx].update_u(BranchResult::NOT_TAKEN);
                }

            } else {
                _base_table[_get_idx(pc)].update(branch);
            }

            for (int i = 1; i <= nLookups; i++) {
                u16 hist_len = _history_lengths[i];
                bool oldest = _history_buffer.get_bit_at(hist_len-1);

                _idx_hashes[i].shift_and_fold(branch, oldest);
                _tag_hashes[i].shift_and_fold(branch, oldest);
                _tag_hashes_2[i].shift_and_fold(branch, oldest);
            }

            _history_buffer.update(branch);

            if (_branches_seen++ > 256'000) {
                _reset_all_banks();
                _branches_seen = 0;
            }
        }

        std::string predictor_name() const override { return "TAGEImproved"; }

        u64 get_size() const override {
            u64 base_tables = _table_size * 2; // bimodal table with 2-bit counter
            u64 upper_b = N_U * (1ull << _tage_idx_width) * (3 + 2 + 1 + _long_tag_width); // 3 bit ctr, 2 u-bits, 1 allocated bit, tag bits
            u64 lower_b = N_L * (1ull << _tage_idx_width) * (3 + 2 + 1 + _short_tag_width);
            u64 upper_csrs = N_L * (2*_short_tag_width - 1 + _tage_idx_width);
            u64 lower_csrs = N_U * (2*_long_tag_width - 1 + _tage_idx_width); 
            u64 history = 3000;
            return base_tables + upper_b + lower_b + upper_csrs + lower_csrs + history;
        }

        void print_stats() const {
            #ifdef TAGE_STATS
            std::cout<< "======STATS for IMPROVED TAGE=====" << std::endl;
            auto pct = [](u64 num, u64 den) { return den ? 100.0 * num / den : 0.0; };
            std::cout << "==== TAGE_STATS ====\n";
            std::cout << "total updates: " << _stat_total << "\n";
            for (int i = 0; i <= nLookups; i++) {
                if (i == 0) { std::cout << "bimodal        "; }
                else { std::cout << "bank " << i << " (h=" << _history_lengths[i] << ")"; }
                std::cout << ": provided " << _stat_prov_cnt[i]
                          << " (" << pct(_stat_prov_cnt[i], _stat_total) << "%)"
                          << ", final MR " << pct(_stat_prov_miss[i], _stat_prov_cnt[i]) << "%\n";
            }
            std::cout << "alt_on_new used: " << _stat_alt_used
                      << " (" << pct(_stat_alt_used, _stat_total) << "%)"
                      << ", MR when used " << pct(_stat_alt_used_miss, _stat_alt_used) << "%"
                      << " (provider would have had MR " << pct(_stat_alt_prov_miss, _stat_alt_used) << "%)\n"
                      << "alt_on_new overriden: " << _stat_alt_rejected
                      << ", alt_on_new success: " << _stat_alt_accepted << "\n";
            std::cout << "alloc attempts: " << _stat_alloc_attempt
                      << ", success " << pct(_stat_alloc_success, _stat_alloc_attempt) << "%"
                      << ", u-resets: " << _stat_resets << "\n";
            #endif
        }
};

