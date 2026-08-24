// sizing.cpp — instantiate each predictor and report get_size().
// Target budget: 32 KiB = 32768 bytes = 262144 bits per predictor.
// Tweak the params below and re-run to converge on the target.
//
//   clang++ sizing.cpp -std=c++20 -O2 -o ./bin/sizing && ./bin/sizing
//
#include "predictors.h"
#include "tage_improved.h"

#include <array>
#include <cstdio>
#include <string>

#include "predictor_params.h"

static void report(const std::string& label, u64 bits) {
    double bytes = bits / 8.0;
    double kib   = bytes / 1024.0;
    std::printf("%-34s %13llu bits %11.1f B %8.2f KiB   %+7.2f KiB\n",
                label.c_str(), (unsigned long long)bits, bytes, kib,
                (bytes - TARGET_BYTES) / 1024.0);
}

int main() {
    std::printf("%-34s %13s %13s %8s %11s\n",
                "predictor (params)", "size", "", "", "vs 32KiB");
    std::printf("target: %.0f bytes (%.0f bits) per predictor\n\n",
                TARGET_BYTES, TARGET_BYTES * 8);

    {
        BimodalPredictor p(BIM_LOG);
        report("Bimodal(log=" + std::to_string(BIM_LOG) + ")", p.get_size());
    }
    {
        GSharePredictor p(GSH_TBL, GSH_HIST);
        report("GShare(tbl=" + std::to_string(GSH_TBL) + ",h=" + std::to_string(GSH_HIST) + ")", p.get_size());
    }
    {
        TwoLevelPredictor p(TL_TBL, TL_HIST);
        report("TwoLevel(tbl=" + std::to_string(TL_TBL) + ",h=" + std::to_string(TL_HIST) + ")", p.get_size());
    }
    {
        PerceptronPredictor p(PCP_TBL, PCP_HIST);
        report("Perceptron(tbl=" + std::to_string(PCP_TBL) + ",h=" + std::to_string(PCP_HIST) + ")", p.get_size());
    }
    {
        TAGEPredictor<TP_H, TP_NC> p(TP_IDX, TP_TAG, TP_NC, TP_L1, TP_RATIO);
        report("TAGE<H=" + std::to_string(TP_H) + ",NC=" + std::to_string(TP_NC) + ">", p.get_size());
    }
    {
        std::array<bool, 2 * TI_NC + 1> no_skip;
        no_skip.fill(true);
        TAGEImproved<TI_N_L, TI_N_U, TI_NC> p(
            TI_BASE_IW, TI_TAGE_IW, TI_SHORT_TW, TI_LONG_TW,
            TI_FIRST_LONG, TI_MIN_HIST, TI_MAX_HIST, no_skip);
        report("TAGEImproved<NL=" + std::to_string(TI_N_L) + ",NU=" + std::to_string(TI_N_U) + ">", p.get_size());
    }
    return 0;
}
