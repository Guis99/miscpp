// Evaluation harness for branch predictors.
//
// Produces one results.md and one results.csv per run, plus stdout summary.
//
// Build:
//   g++ -std=c++17 -O2 -Wall -Wextra eval_predictors.cpp traces.cpp -o eval_predictors
// Run:
//   ./eval_predictors

#include "predictors.h"
#include "primitives.h"
#include "traces.h"

#include <cstdint>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// ============================================================================
// Core types
// ============================================================================

using Stream = std::vector<BranchInstr>;

struct Score {
    size_t correct = 0;
    size_t total = 0;
    double accuracy() const { return total ? double(correct) / double(total) : 0.0; }
};

struct BenchResult {
    std::string bench;
    std::string predictor;
    size_t table_size;
    size_t history_length;
    std::string id_filter;  // empty for whole-stream score
    Score score;
    std::string note;       // e.g. "WIP" for skipped predictors
};

struct PredictorSpec {
    std::string name;
    std::function<std::unique_ptr<BranchPredictorBase>(size_t, size_t)> factory;
    bool wip;  // skip execution if true
};

// TournamentPredictor stores references to its two sub-predictors, which means
// the caller must keep them alive for the tournament's full lifetime. This
// wrapper owns both sub-predictors alongside the tournament so the references
// stay valid, and exposes the BranchPredictorBase interface so the harness can
// hold it via unique_ptr<BranchPredictorBase> like any other predictor.
template <class Pred1, class Pred2>
class OwningTournament : public BranchPredictorBase {
    Pred1 p1_;
    Pred2 p2_;
    TournamentPredictor<Pred1, Pred2> tournament_;
    std::string name_;
public:
    OwningTournament(Pred1 p1, Pred2 p2, size_t meta_ts, std::string name)
        : p1_(std::move(p1)),
          p2_(std::move(p2)),
          tournament_(p1_, p2_, meta_ts),
          name_(std::move(name)) {}

    // Non-movable / non-copyable: the inner tournament_ holds references to
    // p1_ and p2_, so relocating this object would dangle those references.
    OwningTournament(const OwningTournament&) = delete;
    OwningTournament(OwningTournament&&) = delete;
    OwningTournament& operator=(const OwningTournament&) = delete;
    OwningTournament& operator=(OwningTournament&&) = delete;

    bool predict(uint32_t pc) override { return tournament_.predict(pc); }
    void update(uint32_t pc, BranchResult b) override { tournament_.update(pc, b); }
    std::string predictor_name() const override { return name_; }
};

// Fixed roster used by every benchmark.
static const std::vector<PredictorSpec> kPredictors = {
    {"BimodalPredictor",
     [](size_t ts, size_t /*hl*/) { return std::make_unique<BimodalPredictor>(ts); },
     false},
    {"GSharePredictor",
     [](size_t ts, size_t hl) { return std::make_unique<GSharePredictor>(ts, hl); },
     false},
    {"GSelectPredictor",
     [](size_t ts, size_t hl) { return std::make_unique<GSelectPredictor>(ts, hl); },
     false},  // stub: always predicts NOT_TAKEN
    {"TwoLevelPredictor",
     [](size_t ts, size_t hl) { return std::make_unique<TwoLevelPredictor>(ts, hl); },
     false},
    {"PerceptronPredictor",
     [](size_t ts, size_t hl) { return std::make_unique<PerceptronPredictor>(ts, hl); },
     false},
     {"TAGEPredictor",
     [](size_t ts, size_t) { return std::make_unique<TAGEPredictor<256, 6>>(ts, ts-1, 6, 4, 2); },
     false},
    // McFarling's canonical combined predictor: bimodal + gshare with a 2-bit
    // meta-counter per PC. Meta-table sized at `ts`, same as the sub-predictors.
    {"Tournament(Bimodal,GShare)",
     [](size_t ts, size_t hl) {
         return std::make_unique<OwningTournament<BimodalPredictor, GSharePredictor>>(
             BimodalPredictor(ts), GSharePredictor(ts, hl), ts,
             "Tournament(Bimodal,GShare)");
     },
     false},
    // Pairs Bimodal with the strongest per-PC-history predictor we have; we
    // expect this to dominate the alternating-pattern benchmarks where
    // TwoLevel wins outright and Bimodal still holds the stable-bias slots.
    {"Tournament(Bimodal,TwoLevel)",
     [](size_t ts, size_t hl) {
         return std::make_unique<OwningTournament<BimodalPredictor, TwoLevelPredictor>>(
             BimodalPredictor(ts), TwoLevelPredictor(ts, hl), ts,
             "Tournament(Bimodal,TwoLevel)");
     },
     false},
};

// ============================================================================
// Extra stream generators
// ============================================================================

// Independent Bernoulli T/NT stream.
Stream random_stream(size_t len, uint32_t pc, uint32_t id, double p_taken, uint64_t seed) {
    std::mt19937_64 rng(seed);
    std::bernoulli_distribution dist(p_taken);
    Stream out;
    out.reserve(len);
    for (size_t i = 0; i < len; ++i) {
        BranchResult r = dist(rng) ? BranchResult::TAKEN : BranchResult::NOT_TAKEN;
        out.emplace_back(r, pc, id);
    }
    return out;
}

// Repeats: [noise_len random] + [pattern], all on the same (pc, id).
Stream pattern_in_noise(size_t noise_len,
                        const std::vector<BranchResult>& pattern,
                        size_t reps,
                        uint32_t pc, uint32_t id,
                        uint64_t seed) {
    std::mt19937_64 rng(seed);
    std::bernoulli_distribution dist(0.5);
    Stream out;
    out.reserve(reps * (noise_len + pattern.size()));
    for (size_t r = 0; r < reps; ++r) {
        for (size_t i = 0; i < noise_len; ++i) {
            BranchResult b = dist(rng) ? BranchResult::TAKEN : BranchResult::NOT_TAKEN;
            out.emplace_back(b, pc, id);
        }
        for (BranchResult b : pattern) {
            out.emplace_back(b, pc, id);
        }
    }
    return out;
}

// ============================================================================
// Composition: deterministic random interleave weighted by remaining length.
// ============================================================================

Stream interleave(std::vector<Stream> streams, uint64_t seed) {
    std::mt19937_64 rng(seed);
    size_t total = 0;
    for (const auto& s : streams) total += s.size();

    Stream out;
    out.reserve(total);

    std::vector<size_t> pos(streams.size(), 0);
    size_t produced = 0;
    while (produced < total) {
        size_t remaining = total - produced;
        std::uniform_int_distribution<size_t> dist(0, remaining - 1);
        size_t draw = dist(rng);
        for (size_t i = 0; i < streams.size(); ++i) {
            size_t left = streams[i].size() - pos[i];
            if (draw < left) {
                out.push_back(streams[i][pos[i]]);
                ++pos[i];
                ++produced;
                break;
            }
            draw -= left;
        }
    }
    return out;
}

// ============================================================================
// Scoring
// ============================================================================

// Global op counter — every call to score_trace adds trace.size() to this so
// we can audit total predict()+update() pairs after a run.
static size_t g_op_count = 0;

// If scoreable_ids is empty, every branch counts toward the score; otherwise
// only branches whose id is in the set count. Updates always happen — the
// filter is purely about which branches are measured.
Score score_trace(BranchPredictorBase& pred,
                  const Stream& trace,
                  size_t warmup_per_id = 0,
                  const std::unordered_set<uint32_t>& scoreable_ids = {}) {
    std::unordered_map<uint32_t, size_t> seen;
    Score s;
    const bool score_all = scoreable_ids.empty();
    for (const auto& b : trace) {
        bool prediction = pred.predict(b.pc);
        size_t& n = seen[b.id];
        const bool in_scope = score_all || scoreable_ids.count(b.id) > 0;
        if (in_scope && n >= warmup_per_id) {
            s.correct += (prediction == b.direction);
            s.total += 1;
        }
        pred.update(b.pc, b.direction);
        ++n;
    }
    g_op_count += trace.size();
    return s;
}

// ============================================================================
// Reporting: stdout + results.md + results.csv
// ============================================================================

static std::string pct(double x) {
    std::ostringstream os;
    os << std::fixed << std::setprecision(2) << (x * 100.0) << "%";
    return os.str();
}

// RFC 4180 minimal: wrap in double-quotes and double any embedded quotes if
// the field contains a comma, quote, or newline. Required now that some
// predictor names (e.g. "Tournament(Bimodal,GShare)") contain a comma.
static std::string csv_field(const std::string& s) {
    if (s.find_first_of(",\"\n") == std::string::npos) return s;
    std::string out;
    out.reserve(s.size() + 2);
    out += '"';
    for (char c : s) {
        if (c == '"') out += "\"\"";
        else out += c;
    }
    out += '"';
    return out;
}

void emit_results(const std::vector<BenchResult>& rs,
                  const std::string& csv_path) {
    // Preserve insertion order of benchmarks.
    std::vector<std::string> order;
    std::map<std::string, std::vector<const BenchResult*>> by_bench;
    for (const auto& r : rs) {
        if (by_bench.find(r.bench) == by_bench.end()) order.push_back(r.bench);
        by_bench[r.bench].push_back(&r);
    }

    {
        std::ofstream csv(csv_path);
        csv << "bench,predictor,table_size,history_length,id_filter,correct,total,accuracy,note\n";
        for (const auto& r : rs) {
            csv << csv_field(r.bench) << ","
                << csv_field(r.predictor) << ","
                << r.table_size << ","
                << r.history_length << ","
                << csv_field(r.id_filter) << ","
                << r.score.correct << ","
                << r.score.total << ","
                << std::fixed << std::setprecision(6) << r.score.accuracy() << ","
                << csv_field(r.note) << "\n";
        }
    }
}

// ============================================================================
// Benchmark helpers
// ============================================================================

// Run every predictor (skipping WIP) on `trace` at (table_size, history_length),
// tagging each result with bench label. Optionally restrict scoring to a set
// of ids (empty = score all branches).
void run_all_predictors(std::vector<BenchResult>& out,
                        const std::string& bench,
                        const Stream& trace,
                        size_t table_size,
                        size_t history_length,
                        size_t warmup_per_id = 0,
                        const std::unordered_set<uint32_t>& scoreable_ids = {},
                        const std::string& id_filter_label = "") {
    for (const auto& spec : kPredictors) {
        BenchResult r{bench, spec.name, table_size, history_length, id_filter_label, {}, ""};
        if (spec.wip) {
            r.note = "WIP (skipped)";
            out.push_back(r);
            continue;
        }
        auto p = spec.factory(table_size, history_length);
        r.score = score_trace(*p, trace, warmup_per_id, scoreable_ids);
        out.push_back(r);
    }
}

// ============================================================================
// Benchmarks
// ============================================================================

// 1) 16 independent streams, no aliasing. Each PC gets a different simple pattern.
//
// Expected (qualitative): GShare/TwoLevel learn all per-PC patterns; Bimodal
// fails on alternating-style patterns (1/1, 3/3) because the counter ping-pongs.
std::vector<BenchResult> bench_independent_16() {
    constexpr size_t kTable = 8;       // 256-entry tables, ample for 16 PCs
    constexpr size_t kHistory = 4;
    constexpr uint64_t kSeed = 1;

    std::vector<Stream> streams;
    for (uint32_t i = 0; i < 16; ++i) {
        Stream s;
        switch (i % 8) {
            case 0: s = repeating_pattern(1, 0, 64, i, i); break;   // T-only
            case 1: s = repeating_pattern(0, 1, 64, i, i); break;   // NT-only
            case 2: s = repeating_pattern(1, 1, 32, i, i); break;   // 1/1
            case 3: s = repeating_pattern(2, 2, 24, i, i); break;   // 2/2
            case 4: s = repeating_pattern(3, 3, 16, i, i); break;   // 3/3
            case 5: s = repeating_pattern(5, 3, 12, i, i); break;   // 5/3
            case 6: s = repeating_pattern(8, 8,  8, i, i); break;   // 8/8
            case 7: s = repeating_pattern(4, 1, 24, i, i); break;   // 4/1
        }
        streams.push_back(std::move(s));
    }
    Stream trace = interleave(std::move(streams), kSeed);

    std::vector<BenchResult> rs;
    run_all_predictors(rs, "bench_independent_16", trace, kTable, kHistory);
    return rs;
}

// 2) 4096 streams, swept over both table_size (4..14) and history_length
// ({1,2,4,8}). Tests how each predictor scales as the table approaches the
// stream count and as history widens.
//
// Expected: TwoLevel jumps once table_size >= 12 (fits all 4096 streams). Wider
// history then helps further on the 3/3 and 4/4 streams whose periods exceed
// the history window. GShare stays near 50% regardless — global history is
// dominated by inter-stream noise. Bimodal ignores history (rows duplicate
// across history_length values for it).
std::vector<BenchResult> bench_aliasing_sweep() {
    constexpr size_t kNumStreams = 4096;
    constexpr uint64_t kSeed = 2;

    std::vector<Stream> streams;
    streams.reserve(kNumStreams);
    for (uint32_t i = 0; i < kNumStreams; ++i) {
        // Mix of simple patterns; choice depends on id so different streams have
        // different behaviour to maximise collision pressure.
        switch (i & 0x3) {
            case 0: streams.push_back(repeating_pattern(2, 2, 8, i, i)); break;
            case 1: streams.push_back(repeating_pattern(3, 3, 6, i, i)); break;
            case 2: streams.push_back(repeating_pattern(1, 1, 12, i, i)); break;
            case 3: streams.push_back(repeating_pattern(4, 4, 4, i, i)); break;
        }
    }
    Stream trace = interleave(std::move(streams), kSeed);

    std::vector<BenchResult> rs;
    const std::vector<size_t> history_lengths = {1, 2, 4, 8};
    for (size_t ts = 4; ts <= 14; ++ts) {
        for (size_t hl : history_lengths) {
            run_all_predictors(rs, "bench_aliasing_sweep", trace, ts, hl);
        }
    }
    return rs;
}

// 3) Single PC. Noise interleaved with a fixed pattern.
//
// Expected: this is hard. Bimodal is dominated by the noise. GShare/TwoLevel
// can only win if history_length >= pattern.size() and the noise *before* the
// pattern produces a recognisable suffix in the history. Reality: all of them
// will be in the high 50s / low 60s on accuracy on this configuration.
std::vector<BenchResult> bench_pattern_in_noise() {
    constexpr size_t kTable = 8;
    constexpr size_t kHistory = 4;
    constexpr uint64_t kSeed = 3;

    // ~32 random branches, then TTNTTT, repeated.
    std::vector<BranchResult> pattern = {
        BranchResult::TAKEN, BranchResult::TAKEN, BranchResult::NOT_TAKEN,
        BranchResult::TAKEN, BranchResult::TAKEN, BranchResult::TAKEN,
    };
    Stream trace = pattern_in_noise(32, pattern, 128, /*pc=*/0, /*id=*/0, kSeed);

    std::vector<BenchResult> rs;
    run_all_predictors(rs, "bench_pattern_in_noise", trace, kTable, kHistory);
    return rs;
}

// 4) Two streams with TTTTTTTTNNNNNNNN pattern at PCs that alias the bimodal
// index. Tests whether per-history disambiguation helps under heavy aliasing.
//
// Caveat from design discussion: TwoLevel aliases pc_idx the same way as
// bimodal, so both streams share the same branch_history_table[pc_idx] row.
// The expected story is therefore that GShare wins (history XOR scatters
// collisions); TwoLevel may not beat bimodal here. Bimodal's counter
// ping-pongs because the streams contribute opposite outcomes during
// out-of-phase windows.
std::vector<BenchResult> bench_constructive_aliasing() {
    constexpr size_t kTable = 4;       // 16-entry tables, force aliasing
    constexpr size_t kHistory = 3;
    constexpr uint64_t kSeed = 4;

    // pc=0 and pc=16 both index entry 0 with pc_mask=0xF.
    Stream a = repeating_pattern(8, 8, 32, /*pc=*/0,  /*id=*/100);   // 8T 8N starting T
    Stream b = repeating_pattern(0, 0,  0, /*pc=*/16, /*id=*/101);
    // Stream B: 8N 8T (out of phase) — build manually so the first half is NT.
    b.reserve(32 * 16);
    for (size_t r = 0; r < 32; ++r) {
        for (size_t i = 0; i < 8; ++i) b.emplace_back(BranchResult::NOT_TAKEN, 16, 101);
        for (size_t i = 0; i < 8; ++i) b.emplace_back(BranchResult::TAKEN,     16, 101);
    }

    Stream trace = interleave({std::move(a), std::move(b)}, kSeed);

    std::vector<BenchResult> rs;
    // Whole-stream score.
    run_all_predictors(rs, "bench_constructive_aliasing", trace, kTable, kHistory);
    // Per-id breakdown so we can see if any predictor handles A or B better.
    run_all_predictors(rs, "bench_constructive_aliasing", trace, kTable, kHistory,
                       /*warmup=*/0, /*scoreable_ids=*/{100}, "100");
    run_all_predictors(rs, "bench_constructive_aliasing", trace, kTable, kHistory,
                       /*warmup=*/0, /*scoreable_ids=*/{101}, "101");
    return rs;
}

// 5) All seven baseline trace generators replicated N times on distinct PCs,
// interleaved.
//
// Expected: GShare and TwoLevel competitive; Bimodal noticeably worse on
// alternating / correlated streams. GSelect stub always predicts NT, so it
// scores around the trace's NT fraction. The Tournament pairings should beat
// either sub-predictor alone, since the meta-counter learns per-PC which
// sub-predictor is more accurate for each branch.
std::vector<BenchResult> bench_combined(size_t copies_per_baseline = 4) {
    constexpr size_t kTable = 8;
    constexpr size_t kHistory = 4;
    constexpr uint64_t kSeed = 5;

    std::vector<Stream> streams;
    std::unordered_set<uint32_t> scoreable_ids;
    uint32_t next_id = 0;
    auto pc_of = [](uint32_t id) -> uint32_t { return id * 16; };

    // For correlated/xor traces we score only the dependent branch
    // (matching run_traces.cpp methodology — the source branches are random
    // by construction so they cannot be predicted above chance).
    for (size_t copy = 0; copy < copies_per_baseline; ++copy) {
        // 1/1 — score the whole stream
        streams.push_back(repeating_pattern(1, 1, 128, pc_of(next_id), next_id));
        scoreable_ids.insert(next_id++);
        // taken-then-not-taken
        streams.push_back(repeating_pattern(128, 128, 1, pc_of(next_id), next_id));
        scoreable_ids.insert(next_id++);
        // 3/3
        streams.push_back(repeating_pattern(3, 3, 64, pc_of(next_id), next_id));
        scoreable_ids.insert(next_id++);
        // 8/8
        streams.push_back(repeating_pattern(8, 8, 32, pc_of(next_id), next_id));
        scoreable_ids.insert(next_id++);
        // correlated pair: branch1 random, branch2 = branch1 — score only branch2
        {
            uint32_t id1 = next_id++;
            uint32_t id2 = next_id++;
            streams.push_back(correlated_branch(256, pc_of(id1), id1, pc_of(id2), id2, false));
            scoreable_ids.insert(id2);
        }
        // anti-correlated pair: score only branch2
        {
            uint32_t id1 = next_id++;
            uint32_t id2 = next_id++;
            streams.push_back(correlated_branch(256, pc_of(id1), id1, pc_of(id2), id2, true));
            scoreable_ids.insert(id2);
        }
        // xor triple: branch1, branch2 random; branch3 = branch1 XOR branch2 — score only branch3
        {
            uint32_t id1 = next_id++;
            uint32_t id2 = next_id++;
            uint32_t id3 = next_id++;
            streams.push_back(xor_correlated_branch(256, pc_of(id1), id1,
                                                    pc_of(id2), id2,
                                                    pc_of(id3), id3));
            scoreable_ids.insert(id3);
        }
    }
    Stream trace = interleave(std::move(streams), kSeed);

    std::vector<BenchResult> rs;
    run_all_predictors(rs, "bench_combined", trace, kTable, kHistory,
                       /*warmup=*/0, scoreable_ids, "scoreable-only");
    return rs;
}

// ============================================================================
// Main
// ============================================================================

int main() {
    std::vector<BenchResult> all;

    auto append = [&](const std::vector<BenchResult>& rs) {
        all.insert(all.end(), rs.begin(), rs.end());
    };

    auto report_work = [](const char* name, size_t before) {
        std::cerr << "[work] " << name << ": "
                  << (g_op_count - before) << " predict+update pairs\n";
    };

    size_t mark = 0;
    append(bench_independent_16());        report_work("bench_independent_16", mark);        mark = g_op_count;
    append(bench_aliasing_sweep());        report_work("bench_aliasing_sweep", mark);        mark = g_op_count;
    append(bench_pattern_in_noise());      report_work("bench_pattern_in_noise", mark);      mark = g_op_count;
    append(bench_constructive_aliasing()); report_work("bench_constructive_aliasing", mark); mark = g_op_count;
    append(bench_combined());              report_work("bench_combined", mark);
    std::cerr << "[work] TOTAL: " << g_op_count << " predict+update pairs\n";

    

    emit_results(all, "results.csv");
    return 0;
}
