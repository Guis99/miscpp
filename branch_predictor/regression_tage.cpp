// ===========================================================================
// Regression guard: TAGEImproved must not fall meaningfully below TAGE on the
// sanity-check traces.
//
// For every trace in the TAGE sanity set, we score both TAGEPredictor (the
// pinned baseline) and TAGEImproved on the SAME generated trace, then require
//
//     rate(TAGEImproved) >= MIN_RATIO * rate(TAGEPredictor)
//
// per trace. Any trace that regresses makes the process exit non-zero, so this
// can gate a build / CI run while you iterate on TAGEImproved.
//
// Baseline is computed live rather than hard-coded, so it tracks TAGE as its
// shared primitives evolve. If you would rather pin absolute numbers (so the
// bar can't silently drop if TAGE itself regresses), capture the printed
// "base" column into a constant table and compare against that instead.
//
// Build/run:  make run regression-tage
// ===========================================================================

#include "predictors.h"
#include "tage_improved.h"
#include "traces.h"

#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#ifdef TAGE_STATS
#undef TAGE_STATS
#define TAGE_STATS 0
#endif

// --- Predictor geometry: keep in sync with the TAGE runners in run_traces.cpp
#define IW 9        // index width
#define TW 7        // tag width
#define NUM_COMP 12 // tagged component banks
#define L1 4        // shortest history length
#define R 1.6f      // geometric history ratio
#define H_SIZE 1024 // history buffer capacity

// --- Trace size (matches run_traces.cpp's FACTOR); override at compile time.
#ifndef FACTOR
#define FACTOR 6
#endif

// --- Regression threshold: TAGEImproved must reach this fraction of TAGE.
#ifndef MIN_RATIO
#define MIN_RATIO 0.98
#endif

using TAGE_t = TAGEPredictor<H_SIZE, NUM_COMP>;
using TAGEImproved_t = TAGEImproved<H_SIZE, 8, 4>;

namespace {

constexpr int kScoreAll = -1;

struct Case {
    std::string name;
    std::vector<BranchInstr> trace;
    int score_id;  // kScoreAll, or a specific branch id to score
};

// Fresh predictor per case; identical trace fed to both predictors so any
// nondeterminism in a generator (e.g. imitate_branch) cancels in the ratio.
template <class Pred>
double score(const Case& c) {
    Pred p(IW, TW, NUM_COMP, L1, R);
    size_t correct = 0, total = 0;
    for (const auto& b : c.trace) {
        bool pred = p.predict(b.pc);
        if (c.score_id == kScoreAll || b.id == static_cast<uint32_t>(c.score_id)) {
            correct += (pred == b.direction);
            ++total;
        }
        p.update(b.pc, b.direction);
    }
    return total ? static_cast<double>(correct) / static_cast<double>(total) : 0.0;
}

std::string pct(double x) {
    std::ostringstream os;
    os << std::fixed << std::setprecision(3) << (100.0 * x) << "%";
    return os.str();
}

}  // namespace

int main() {
    const size_t N = 1024u << FACTOR;

    // Mirror exactly the traces run under "TAGE sanity checks" in run_traces.cpp.
    std::vector<Case> cases;
    cases.push_back({"trace_1_0", repeating_pattern(1, 1, N), kScoreAll});
    cases.push_back({"trace_taken_then_not_taken", repeating_pattern(N, N, 1), kScoreAll});
    cases.push_back({"trace_3_3", repeating_pattern(3, 3, N), kScoreAll});
    cases.push_back({"trace_6_2", repeating_pattern(6, 2, N), kScoreAll});
    cases.push_back({"trace_8_8", repeating_pattern(8, 8, N), kScoreAll});
    cases.push_back({"trace_xor", xor_correlated_branch(N, 0, 0, 1, 1, 2, 2), 2});
    cases.push_back({"trace_imitate", imitate_branch(N, 0, 0), kScoreAll});

    std::cout << "==== TAGEImproved regression vs TAGE (min ratio "
              << std::fixed << std::setprecision(2) << MIN_RATIO << ") ====\n";
    std::cout << std::left << std::setw(28) << "trace"
              << std::right << std::setw(11) << "base"
              << std::setw(11) << "improved"
              << std::setw(11) << "required"
              << "   verdict\n";

    int failures = 0;
    for (const auto& c : cases) {
        double base = score<TAGE_t>(c);
        double improved = score<TAGEImproved_t>(c);
        double required = MIN_RATIO * base;
        // Tiny tolerance so exact-equal baselines never flake on rounding.
        bool ok = improved >= required - 1e-9;

        std::cout << std::left << std::setw(28) << c.name
                  << std::right << std::setw(11) << pct(base)
                  << std::setw(11) << pct(improved)
                  << std::setw(11) << pct(required)
                  << "   " << (ok ? "PASS" : "*** FAIL ***") << "\n";

        if (!ok) {
            ++failures;
            std::cout << "    REGRESSION: " << c.name << " improved " << pct(improved)
                      << " < required " << pct(required)
                      << " (" << std::fixed << std::setprecision(2) << MIN_RATIO
                      << " * base " << pct(base) << ")\n";
        }
    }

    std::cout << "====\n";
    if (failures) {
        std::cout << failures << " trace(s) regressed below the baseline.\n";
        return 1;
    }
    std::cout << "All traces within " << std::fixed << std::setprecision(2)
              << MIN_RATIO << "x of TAGE baseline.\n";
    return 0;
}
