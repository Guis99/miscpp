// WASM driver for the interactive demo in web/index.html.
//
// Runs the real BimodalPredictor and GSharePredictor from predictors.h. Each
// demo owns its own predictor instance; the two never share state. The page
// owns the list of branches (one PC each) and feeds one outcome at a time as
// *_predict(pc) then *_update(pc, t), reading counter state back to draw it.
//
// Both demos export the same shape of API, so the page can drive them with
// one piece of code:
//   *_entries()       number of counters in the table
//   *_index(pc)       entry *_predict(pc) will read
//   *_counter(i)      raw counter value at entry i (-1 if out of range)
//   *_predict(pc)     1 = taken
//   *_update(pc, t)   train on outcome t (1 = taken)
// plus a reset that rebuilds the predictor from the page's knobs:
//   bim_reset(index_bits)
//   gsh_reset(index_bits, history_len)
// Knob values are clamped to the limits below; the page reads the limits back
// to set up its sliders.
//
// Build: run `make` in this directory (see Makefile).

#include <algorithm>
#include <optional>
#include <vector>

#include <emscripten/emscripten.h>

#include "predictors.h"

namespace {

// ---------------------------------------------------------------------------
// predictors.h keeps the counter tables, gshare's history register, and
// bimodal's index function private, with no read accessors. Explicit template
// instantiation is exempt from access checking ([temp.explicit]), so naming
// those members there gives us member pointers into the real state without
// touching the header.
template <typename Tag, typename Tag::type Member>
struct Expose {
    friend typename Tag::type member(Tag) { return Member; }
};

using Counter = SatCounter<2>;

struct BimodalTable {
    using type = std::vector<Counter> BimodalPredictor::*;
    friend type member(BimodalTable);
};
struct BimodalIndex {
    using type = size_t (BimodalPredictor::*)(size_t) const;
    friend type member(BimodalIndex);
};
struct GShareTable {
    using type = std::vector<Counter> GSharePredictor::*;
    friend type member(GShareTable);
};
struct GShareHistory {
    using type = HistoryBuffer<64> GSharePredictor::*;
    friend type member(GShareHistory);
};

template struct Expose<BimodalTable, &BimodalPredictor::pred_table>;
template struct Expose<BimodalIndex, &BimodalPredictor::get_idx>;
template struct Expose<GShareTable, &GSharePredictor::pred_table>;
template struct Expose<GShareHistory, &GSharePredictor::history_buffer>;
// ---------------------------------------------------------------------------

template <typename> struct CounterBits;
template <unsigned N> struct CounterBits<SatCounter<N>> { static constexpr int value = N; };

constexpr int kMinIndexBits = 1, kMaxIndexBits = 4;  // 2..16 counters
constexpr int kMinHistory = 2, kMaxHistory = 8;

std::optional<BimodalPredictor> bimodal{std::in_place, kMaxIndexBits};
std::optional<GSharePredictor> gshare{std::in_place, kMaxIndexBits, kMaxIndexBits};
int gshare_history_len = kMaxIndexBits;

const std::vector<Counter>& table(const BimodalPredictor& p) { return p.*member(BimodalTable{}); }
const std::vector<Counter>& table(const GSharePredictor& p) { return p.*member(GShareTable{}); }

int counter_at(const std::vector<Counter>& t, int i) {
    return (i >= 0 && static_cast<size_t>(i) < t.size()) ? t[i].get_val() : -1;
}

}  // namespace

extern "C" {

EMSCRIPTEN_KEEPALIVE int counter_bits() { return CounterBits<Counter>::value; }
EMSCRIPTEN_KEEPALIVE int min_index_bits() { return kMinIndexBits; }
EMSCRIPTEN_KEEPALIVE int max_index_bits() { return kMaxIndexBits; }
EMSCRIPTEN_KEEPALIVE int min_history() { return kMinHistory; }
EMSCRIPTEN_KEEPALIVE int max_history() { return kMaxHistory; }

// --- bimodal ---------------------------------------------------------------
EMSCRIPTEN_KEEPALIVE void bim_reset(int index_bits) {
    bimodal.emplace(std::clamp(index_bits, kMinIndexBits, kMaxIndexBits));
}
EMSCRIPTEN_KEEPALIVE int bim_entries() { return table(*bimodal).size(); }
EMSCRIPTEN_KEEPALIVE int bim_index(unsigned pc) { return ((*bimodal).*member(BimodalIndex{}))(pc); }
EMSCRIPTEN_KEEPALIVE int bim_counter(int i) { return counter_at(table(*bimodal), i); }
EMSCRIPTEN_KEEPALIVE int bim_predict(unsigned pc) { return bimodal->predict(pc); }
EMSCRIPTEN_KEEPALIVE void bim_update(unsigned pc, int taken) { bimodal->update(pc, BranchResult(taken != 0)); }

// --- gshare ----------------------------------------------------------------
EMSCRIPTEN_KEEPALIVE void gsh_reset(int index_bits, int history_len) {
    gshare_history_len = std::clamp(history_len, kMinHistory, kMaxHistory);
    gshare.emplace(std::clamp(index_bits, kMinIndexBits, kMaxIndexBits), gshare_history_len);
}
EMSCRIPTEN_KEEPALIVE int gsh_entries() { return table(*gshare).size(); }
EMSCRIPTEN_KEEPALIVE int gsh_index(unsigned pc) { return gshare->get_idx(pc); }
EMSCRIPTEN_KEEPALIVE int gsh_counter(int i) { return counter_at(table(*gshare), i); }
EMSCRIPTEN_KEEPALIVE int gsh_predict(unsigned pc) { return gshare->predict(pc); }
EMSCRIPTEN_KEEPALIVE void gsh_update(unsigned pc, int taken) { gshare->update(pc, BranchResult(taken != 0)); }

// The last history_len outcomes, newest in bit 0: the history get_idx() XORs in.
EMSCRIPTEN_KEEPALIVE int gsh_history() {
    return ((*gshare).*member(GShareHistory{})).as_idx() & ((1ull << gshare_history_len) - 1);
}

}  // extern "C"
