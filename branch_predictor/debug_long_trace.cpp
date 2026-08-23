#include "tage_improved.h"
#include "traces.h"

// --- TAGEImproved geometry (new interleaved-bank interface).
//     Placeholder values; tune as the design firms up. nLookups = 2 * TI_NC.
#define TI_NC          12   // number of history lengths
#define TI_N_L         12   // lower (short-tag) banks   -- sizing only
#define TI_N_U         12   // upper (long-tag)  banks   -- sizing only
#define TI_BASE_IW     9    // base table index width
#define TI_TAGE_IW     9    // tagged bank index width
#define TI_SHORT_TW    7    // short tag width
#define TI_LONG_TW     9    // long tag width
#define TI_FIRST_LONG  9   // first long-tag table (1-based lookup index)
#define TI_SHORT_FACT  8    // shortTagsTageFactor
#define TI_LONG_FACT   16    // longTagsTageFactor
#define TI_MIN_HIST    4    // shortest history length
#define TI_MAX_HIST    400  // longest history length

void print_score(BranchPredictorBase* predictor, std::string title, size_t score, size_t seq_len) {
    double perc = 100 * (double)score / (double)seq_len;
    std::cout << "[" << *predictor << "] " << title << ": Predicted " << score << " out of " << seq_len << " correctly (" << perc << "%)" << std::endl;
}

#define FACTOR 4
size_t num_reps = 1024 << FACTOR;

std::vector<BranchInstr> v_trace_8_8 = repeating_pattern(8, 8, num_reps);

void trace_8_8(BranchPredictorBase* predictor) {
    // 8 taken followed by 8 not taken
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

constexpr int TI_NLOOKUPS = 2 * TI_NC;
using TAGEImproved_t = TAGEImproved<TI_N_L, TI_N_U, TI_NC>;

int main() {
    std::array<bool, TI_NLOOKUPS+1> no_skip;
    no_skip.fill(true);
    TAGEImproved_t predictor = TAGEImproved_t(TI_BASE_IW, TI_TAGE_IW, TI_SHORT_TW, TI_LONG_TW,
                          TI_FIRST_LONG, 
                          TI_MIN_HIST, TI_MAX_HIST, no_skip);

    trace_8_8(&predictor);
}
