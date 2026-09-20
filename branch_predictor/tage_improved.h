#pragma once

#include "predictors.h"
#include "primitives.h"
#include <cstdint>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <memory>
#include <algorithm>
#undef TAGE_STATS
// #define TAGE_STATS

#ifdef TAGE_STATS
    #ifndef SC_STATS
    #define SC_STATS
    #endif
#endif

#ifndef SC_STATS
// #define SC_STATS
#endif

#ifdef USE_PHIST_
    inline constexpr bool USE_PHIST = true;
#else
    inline constexpr bool USE_PHIST = false;
#endif

#define PHISTWIDTH 27
#define PMASK (1ull << PHISTWIDTH) - 1

// ---------------------------------------------------------------------------
// TAGEImproved: iteration target for the advanced techniques (interleaved
// banks, statistical correction, loop predictor, ...).
// ---------------------------------------------------------------------------


struct conf {
    bool low_conf, high_conf;
};

static constexpr u16 bim2tage[4] = {0,3,4,7};

class StatCorrector : BranchPredictorBase {
    static constexpr u8 CS = 6; // GEHL counter size

    static constexpr u8 NUM_BIAS = 2;
    static constexpr u8 NUM_GLOBAL_H = 3;
    static constexpr u8 NUM_PATH_H = 3;

    static constexpr u8 GLOB_H_LEN[NUM_GLOBAL_H] = {10, 40, 64};
    static constexpr u8 PATH_H_LEN[NUM_PATH_H] = {24, 16, 9};

    static constexpr u64 tage_weight = 8;

    static constexpr u64 num_tables = NUM_BIAS + NUM_GLOBAL_H + USE_PHIST*NUM_PATH_H;

    u64 _lg_bias_ctr_size; // log_2 bias counter table size
    u64 _lg_gh_ctr_size;
    u64 _lg_ph_ctr_size; 
    u64 _lg_lh1_ctr_size;
    u64 _lg_lh2_ctr_size;
    u64 _lg_lh3_ctr_size;

    u64 _bias_mask;
    u64 _global_hist_mask;

    HistoryBuffer<64> _global_history;
    u64 _path_hist = 0;
    std::vector<std::vector<HistoryBuffer<32>>> _local_histories;

    std::vector<SatCounter<CS>> _bias;
    std::vector<SatCounter<CS>> _bias_sk;

    std::array<std::vector<SatCounter<CS>>, NUM_GLOBAL_H> _w_global;
    std::array<std::vector<SatCounter<CS>>,NUM_PATH_H> _w_path_hist;
    std::vector<SatCounter<CS>> _w_local_1;
    std::vector<SatCounter<CS>> _w_local_2;
    std::vector<SatCounter<CS>> _w_local_3;

    std::array<csr, NUM_GLOBAL_H> _gh_hashes = {};

    conf _tage_conf {} ;
    u8 _last_tage_ctr = SatCounter<3>::midpoint();
    bool _use_bias = false;
    bool _tage_pred = false;
    bool _sc_pred = false;
    
    bool _overrode = false;

    u64 _last_sum = 0;
    bool _last_pred = false;

    SatCounter<10> _global_threshold_ctr;
    std::vector<SatCounter<8>> _local_threshold_ctrs;
    u64 _ltc_mask = (1ull << 6) - 1;

    SatCounter<7> _lc_chooser = {};

    u64 _get_bias_idx(u64 pc) {
        return ( (
            (((pc^(pc >> 2)) << 1) ^ 
            (_tage_conf.low_conf & _use_bias)) << 1 ) + _tage_pred) 
        & _bias_mask;
    }

    u64 _get_bias_sk_idx(u64 pc) {
        return ( (
            (((pc^(pc >> 8)) << 1) ^ 
            (_tage_conf.high_conf)) << 1 ) + _tage_pred) 
        & _bias_mask;
    }

    u64 _get_gh_idx(u64 pc, int i) {
        return ( (pc >> 2) ^ _gh_hashes[i].hash ^ (_tage_pred << 9) ) & _global_hist_mask;
    }

    u64 _get_bias(u64 pc) {
        u64 bias_val = _bias[_get_bias_idx(pc)].get_val()
        + _bias_sk[_get_bias_sk_idx(pc)].get_val();

        return bias_val;
    }

    u64 _get_gh_prediction(u64 pc) {
        u64 global_h_pred = 0;
        for (int i = 0; i < NUM_GLOBAL_H; i++) {
            u64 idx = _get_gh_idx(pc, i);
            global_h_pred += _w_global[i][idx].get_val();
        }

        return global_h_pred;
    }

    u64 _local_hist_idx(u64 pc, u64 bhist, int h_idx) {
        return (((uint64_t) pc) ^ bhist ^ (bhist >> (8 - h_idx)) ^ (bhist >> (16 - 2 * h_idx)) ^ (bhist >> (24 - 3 * h_idx)) ^ (bhist >> (32 - 3 * h_idx)) ^ (bhist >> (40 - 4 * h_idx))) & ((1 << (9 - (h_idx >= (NUM_PATH_H - 2)))) - 1);
    }

    u64 _get_path_prediction(u64 pc) {
        if constexpr (!USE_PHIST) { return 0; }
        u64 path_h_prediction = 0;
        for (int i=0; i<NUM_PATH_H; i++) {
            u64 bhist = _path_hist & ((u64) ((1ULL << PATH_H_LEN[i]) - 1));
            u64 idx = _local_hist_idx(pc, bhist, i);
            path_h_prediction += _w_path_hist[i][idx].get_val();
        }

        return path_h_prediction;
    }

    u64 _get_threshold(u64 pc) {
        return (_global_threshold_ctr.get_val()>>3) + _local_threshold_ctrs[pc & _ltc_mask].get_val();
        // return 8;
    }

    #ifdef SC_STATS
        u64 _sc_num_predicts;
        u64 _sc_num_overrides;
        u64 _sc_override_on_high_conf;
        u64 _sc_override_on_low_conf;
    #endif

    public:
        StatCorrector(u64 bias_idx_width, u64 global_idx_width, u64 local_idx_width_1, u64 path_hist_idx_width) :
            _lg_bias_ctr_size(bias_idx_width), _lg_gh_ctr_size(global_idx_width), _lg_lh1_ctr_size(), _lg_ph_ctr_size(path_hist_idx_width),
            _bias_mask((1ull << bias_idx_width) - 1), _global_hist_mask((1ull << global_idx_width) - 1), 
            _bias(1ull << bias_idx_width, SatCounter<CS>()), _bias_sk(1ull << bias_idx_width, SatCounter<CS>())
        {
            for (int i = 0; i < NUM_GLOBAL_H; i++) {
                _w_global[i].resize(1ull << global_idx_width);
                _gh_hashes[i] = csr(_global_hist_mask, global_idx_width, (GLOB_H_LEN[i] - 1) % global_idx_width);
            }

            _local_threshold_ctrs.resize(1ull<<6);

            if constexpr (USE_PHIST) {
                for (int i = 0; i < NUM_PATH_H; i++) {
                    _w_path_hist[i].resize(1ull << path_hist_idx_width);
                    // _gh_hashes[i] = csr(_global_hist_mask, global_idx_width, (GLOB_H_LEN[i] - 1) % global_idx_width);
                }
                
            }
        }

        bool predict_sc(u64 pc) {
            
            u64 tage_contrib = tage_weight * _last_tage_ctr;
            
            u64 midpoint = num_tables * SatCounter<CS>::midpoint() + tage_weight * SatCounter<3>::midpoint();
            u64 threshold = _get_threshold(pc);
            u64 sum = 0;
            //std::cout << "sc predict in1" << std::endl;
            sum += _get_bias(pc);
            //std::cout << "sc predict in2" << std::endl;
            sum += _get_gh_prediction(pc);
            sum += _get_path_prediction(pc);
            //std::cout << "sc predict in3" << std::endl;
            sum += tage_contrib;
            //std::cout << "sc predict in4" << std::endl;
            _last_sum = sum;
            //std::cout << "sc predict in5" << std::endl;
            _sc_pred = sum >= midpoint;
            int midpoint_distance = std::abs((int)_last_sum - (int)midpoint);
            bool override_tage = (_sc_pred != _tage_pred) & (midpoint_distance > threshold);
            if (_sc_pred != _tage_pred) {
                if (_tage_conf.high_conf) {
                    if ((midpoint_distance < threshold / 2) & (midpoint_distance >= threshold/4)) {
                        override_tage = !_lc_chooser.predict();
                    }
                }
            }
            _overrode = override_tage;
            #ifdef SC_STATS
                _sc_num_predicts++;
                _sc_num_overrides += override_tage;
                _sc_override_on_high_conf += override_tage & _tage_conf.high_conf;
                _sc_override_on_low_conf += override_tage & _tage_conf.low_conf;
            #endif
            //std::cout << "sc predict in6" << std::endl;
            _last_pred = override_tage ? _sc_pred : _tage_pred;
            //std::cout << "sc predict in7" << std::endl;

            return _last_pred;
        }   

        bool predict(u64 pc) override {
            return predict_sc(pc);
        }

        void update_sc(u64 pc, BranchResult branch) {
            u64 midpoint = num_tables * SatCounter<CS>::midpoint() + tage_weight * SatCounter<3>::midpoint();
            u64 threshold = _get_threshold(pc);
            int midpoint_distance = std::abs((int)_last_sum - (int)midpoint);
            
            bool pred_correct = _last_pred == branch;
            bool tage_correct = _tage_pred == branch;
            bool sc_correct = _sc_pred == branch;

            if ((!sc_correct) || (midpoint_distance < threshold)) {
                _bias[_get_bias_idx(pc)].update(branch);
                _bias_sk[_get_bias_sk_idx(pc)].update(branch);

                for (int i=0; i<NUM_GLOBAL_H; i++) {
                    u64 idx = _get_gh_idx(pc, i);
                    _w_global[i][idx].update(branch);
                }

                if constexpr (USE_PHIST) {
                    for (int i=0; i<NUM_PATH_H; i++) {
                        u64 bhist = _path_hist & ((u64) ((1ULL << PATH_H_LEN[i]) - 1));
                        u64 idx = _local_hist_idx(pc, bhist, i);
                        _w_path_hist[i][idx].update(branch);
                    }
                }

                _global_threshold_ctr.update(BranchResult(!sc_correct));
                _local_threshold_ctrs[pc & _ltc_mask].update(BranchResult(!sc_correct));
            }

            // if ()
            if ((_tage_pred != _sc_pred) & (midpoint_distance < threshold) & _tage_conf.high_conf) {
                if ((midpoint_distance < threshold/2) & (midpoint_distance >= threshold/4)) {
                    _lc_chooser.update( BranchResult(tage_correct));
                }
                
            }

        }     

        void update(u64 pc, BranchResult branch) override {
            update_sc(pc, branch);
            for (int i=0; i<NUM_GLOBAL_H; i++) {
                bool oldest = _global_history.get_bit_at(GLOB_H_LEN[i]-1);
                _gh_hashes[i].shift_and_fold(branch, oldest);
            }
            
            _global_history.update(branch);
        } 

        void load_branch_info(u8 last_ctr, conf tage_conf, bool use_bias, bool tage_pred, u64 path_hist) {
            _last_tage_ctr = last_ctr;
            _tage_conf = tage_conf;
            _use_bias = use_bias;
            _tage_pred = tage_pred;
            _path_hist = path_hist;
        }

        ~StatCorrector() {}

        std::string predictor_name() const override { return "StatCorrector"; }
        u64 get_size() const override {
            u64 total_size = 0;
            // tables
            total_size += 2 * CS * (1ull << _lg_bias_ctr_size);
            total_size += NUM_GLOBAL_H * CS * (1ull << _lg_gh_ctr_size);
            total_size += USE_PHIST*NUM_PATH_H * CS * (1ull << _lg_ph_ctr_size);

            // counters
            total_size += 10; // global threshold
            total_size += 8 * (1ull << 6); // local threshold
            total_size += 7; // 

            return total_size;
        }

        void print_sc_stats() {
            #ifdef SC_STATS
            auto pct = [](u64 num, u64 den) { return den ? 100.0 * num / den : 0.0; };
            std::cout << "sc predict called " << _sc_num_predicts << " times, " 
            << "overrode " << _sc_num_overrides << " times (" << pct(_sc_num_overrides, _sc_num_predicts) << "%)" << std::endl;

            std::cout << "overrides on high TAGE confidence: " << _sc_override_on_high_conf << " (" << pct(_sc_override_on_high_conf, _sc_num_overrides) << "%)\n"
            << "overrides on low TAGE confidence: " << _sc_override_on_low_conf << " (" <<  pct(_sc_override_on_low_conf, _sc_num_overrides) << "%)" << std::endl;

            std::cout << "final global threshold value: " << _global_threshold_ctr.get_val() << " / " << 2*SatCounter<10>::midpoint()-1 
            << "\nfinal chooser value: " << _lc_chooser.get_val() << " / " << 2*SatCounter<7>::midpoint()-1 << std::endl;
            #endif
        }
};

template <int N_L, int N_U, int NC>
// N_L: number of lower banks
// N_U: number of upper banks
// NC: number of history lengths
class TAGEImproved : public BranchPredictorBase {
    constexpr static int nLookups = 2 * NC;
    constexpr static size_t U = 1;

    std::unique_ptr<StatCorrector> sc;
    bool _use_sc = false;

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
    std::array<std::vector<TagEntry<1>>, 2> _banks = {};

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

    u64 _path_hist = 0;

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

    conf _get_confidence(u64 bank_idx, u64 top_idx) {
        conf tage_conf;

        if (bank_idx > 0) {
            tage_conf.low_conf = _banks[_get_mapped_idx(bank_idx)][top_idx].get_ctr() == 3 || _banks[_get_mapped_idx(_top_idx)][top_idx].get_ctr() == 4;
        } else {
            tage_conf.low_conf = _base_table[top_idx].get_val() == 1 || _base_table[top_idx].get_val() == 2;
        }
        
        tage_conf.high_conf = !tage_conf.low_conf;

        return tage_conf;
    }

    void _allocate_new(bool should_flip, [[maybe_unused]] uint64_t pc) {
        bool allocated = false;
        #ifdef TAGE_STATS
                _stat_alloc_attempt++;
        #endif
            for (u8 i = _top_idx+1; i <= nLookups; i++) {
                auto b_idx = _idx_cache[i];
                auto b_useful = _banks[_get_mapped_idx(i)][b_idx].get_u();
                // if (b_useful == 0 && !allocated) {
                if (_is_entry_new(i, b_idx) && !allocated) {
                    auto b_tag = _tag_cache[i];
                    _banks[_get_mapped_idx(i)][b_idx].allocate(b_tag, should_flip);
                    allocated = true;
                    COLLECT_ALLOC(true);
                    COLLECT_FP_STORE(_get_mapped_idx(i), b_idx, _collect_fp(pc, _history_lengths[i]));
        #ifdef TAGE_STATS
                        _stat_alloc_success++;
        #endif
                    break;
                }
            }

        if (!allocated) {
            COLLECT_ALLOC(false);
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

#ifdef COLLECT_DATA
    // Wide, tag-independent fingerprint of (pc, real history[0:hl]) for the
    // COLLECT_DATA tag-false-match instrument. Not part of prediction.
    uint64_t _collect_fp(uint64_t pc, int hl) const {
        uint64_t h = collect_mix(pc * 2 + 1);
        for (int s = 0; s < hl; s += 63) {
            int n = (hl - s < 63) ? (hl - s) : 63;
            h ^= collect_mix(_history_buffer.get_bit_range((u16)s, (size_t)n) ^ (uint64_t)(s + 1));
        }
        return collect_mix(h);
    }
#endif

    bool _is_entry_new(u8 bank_idx, u16 top_idx) const {
        bool u_zeroed = _banks[_get_mapped_idx(bank_idx)][top_idx].get_u() == 0;
        bool ctr_weak = _banks[_get_mapped_idx(bank_idx)][top_idx].get_ctr() == 3 || _banks[_get_mapped_idx(_top_idx)][top_idx].get_ctr() == 4;
        return u_zeroed && ctr_weak;
    }

    int _get_mapped_idx(int bank_idx) const {
        return _bank_map[bank_idx];
    }

    void _update_path_hist(u64 pc, BranchResult branch, int brtype, u64 next_pc) {
        if constexpr (!USE_PHIST) { return ;}
        int T = ((pc ^ (pc >> 2))) ^ branch;
		int PATH = pc ^ (pc >> 2) ^ (pc >> 4);
        if ((brtype == 3) & branch) {
            T = (T ^ (next_pc >> 2));
            PATH = PATH ^ (next_pc >> 2) ^ (next_pc >> 4);
        }
        bool DIR = (T & 1);
        T >>= 1;
        int PATHBIT = (PATH & 127); // 0b 0111 1111, 0x7f
        PATH >>= 1;
        _path_hist = (_path_hist << 1) ^ PATHBIT; //phist
        _path_hist = (_path_hist & ((1<<PHISTWIDTH)-1));
    }

    public:
        TAGEImproved(size_t idx_width_1, size_t idx_width_2, size_t tag_width_1, size_t tag_width_2, 
                    u64 firstLongTagTable,
                    u64 minHist, u64 maxHist, 
                    std::array<bool, nLookups+1> noSkip,
                    bool use_sc = false) :
                _use_sc(use_sc),
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
                _history_lengths[1] = minHist;
                _history_lengths[NC] = maxHist;
                
                for (int i = 2; i <= NC; i++) {
                    
                    _history_lengths[i] = (int) (((double) minHist *
                                pow ((double) (maxHist) / (double) minHist,
                                    (double) (i - 1) / (double) ((NC - 1))))
                                + 0.5);
                }    

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
                    _tag_hashes_2[i] = csr( (1ull << (_tagWidths[i] - 1)) - 1, _tagWidths[i] - 1, (hl - 1) % (_tagWidths[i] - 1));
                }

                _banks[0].resize(N_L * (1 << _tage_idx_width));
                _banks[1].resize(N_U * (1 << _tage_idx_width));
                COLLECT_FP_REGISTER(std::vector<size_t>{_banks[0].size(), _banks[1].size()});

                if (use_sc) {
                    sc = std::make_unique<StatCorrector>(10,10,10,9);
                }
        }

        ~TAGEImproved() { 
            print_stats(); 
        }

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
                    ((1 << std::min(27, (int)_history_lengths[_firstLongTagTable])) - 1))
             % N_L;

             for (int i = _firstLongTagTable; i <= nLookups; i++) {
                if (_noSkip[i]) {
                    _idx_cache[i] += (t << _tage_idx_width);
                    t++;
                    t = t % N_L;
                }
            }
 
            t = (shifted_pc ^ 
                            ((1 << std::min(27, (int)_history_lengths[1])) - 1))
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
                COLLECT_IDX_CHECK(_get_mapped_idx(i), idx,
                                  _noSkip[i] && _banks[_get_mapped_idx(i)][idx].is_allocated(),
                                  _collect_fp(pc, _history_lengths[i]));
                if (tag_match) {
                    _alt_pred = _top_pred;
                    _top_pred = pred;
                    _top_idx = i;
                }
            }

            if (_top_idx > 0)
                COLLECT_FP_CHECK(_get_mapped_idx(_top_idx), _idx_cache[_top_idx],
                                 _collect_fp(pc, _history_lengths[_top_idx]));

            bool final_pred = _top_pred;
            if (_top_idx > 0) {
                u16 top_idx = _idx_cache[_top_idx];
                if (_is_entry_new(top_idx)) {
                    _alt_on_new = true;
                    final_pred = _aon_tracker.predict() ? _alt_pred : _top_pred;
                } 
            }

            if (_use_sc) {
                bool used_bank = _top_idx > 0;
                u16 top_idx = used_bank ? _idx_cache[_top_idx] : _get_idx(pc);
                u8 tage_ctr = used_bank ? _banks[_get_mapped_idx(_top_idx)][top_idx].get_ctr() : bim2tage[_base_table[_get_idx(pc)].get_val()];
                conf tage_conf = _get_confidence(_top_idx, top_idx);
                
                bool use_bias = _alt_pred != _top_pred;
                sc->load_branch_info(tage_ctr, tage_conf, use_bias, _top_pred, _path_hist); // needs: tage counter, tage conf, use_bias, tage_pred, path_hist
                final_pred = sc->predict(pc);
            }

            return final_pred;
        }

        void update_history(u64 pc, BranchResult branch) {
        
        }

        void update(u64 pc, BranchResult branch) override {
            update(pc, branch, 0, 0);
        }

        void update(u64 pc, BranchResult branch, u64 brtype, u64 next_pc) override {
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
                _allocate_new(branch, pc);
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

            COLLECT_PROVIDER(_top_idx > 0 ? _history_lengths[_top_idx] : 0,
                             (_alt_on_new ? _alt_pred : _top_pred) != branch);
            COLLECT_SNAPSHOT(_banks);

            for (int i = 1; i <= nLookups; i++) {
                u16 hist_len = _history_lengths[i];
                bool oldest = _history_buffer.get_bit_at(hist_len-1);

                _idx_hashes[i].shift_and_fold(branch, oldest);
                _tag_hashes[i].shift_and_fold(branch, oldest);
                _tag_hashes_2[i].shift_and_fold(branch, oldest);
            }

            _history_buffer.update(branch);
            _update_path_hist(pc, branch, brtype, next_pc);

            if (_branches_seen++ > 256'000) {
                _reset_all_banks();
                _branches_seen = 0;
            }

            if (_use_sc) {
                //std::cout << "sc update" << std::endl;
                sc->update(pc, branch);
            }
        }

        std::string predictor_name() const override { return "TAGEImproved"; }

        u64 get_size() const override {
            u64 base_tables = _table_size * 2; // bimodal table with 2-bit counter
            u64 upper_b = N_U * (1ull << _tage_idx_width) * (3 + U + 1 + _long_tag_width); // 3 bit ctr, 2 u-bits, 1 allocated bit, tag bits
            u64 lower_b = N_L * (1ull << _tage_idx_width) * (3 + U + 1 + _short_tag_width);
            u64 upper_csrs = N_L * (2*_short_tag_width - 1 + _tage_idx_width);
            u64 lower_csrs = N_U * (2*_long_tag_width - 1 + _tage_idx_width); 
            u64 history = 3000;
            u64 sc_size = sc ? sc-> get_size() : 0;
            return base_tables + upper_b + lower_b + upper_csrs + lower_csrs + history + sc_size;
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
            #ifdef SC_STATS
            if (_use_sc) { sc->print_sc_stats(); }
            #endif
        }
};

