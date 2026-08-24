#pragma once

// ======================= tweak these to hit 32 KiB =========================
// Shared predictor geometry: consumed by sizing.cpp and by the CBP sweep
// factory (cbp2025/my_cond_branch_predictor.h) so measured size and swept
// configuration never drift apart.
// Bimodal(log2_entries):            entries = 2^log,  2 bits each
constexpr int BIM_LOG   = 17;
// GShare(log2_entries, log2_hist)
constexpr int GSH_TBL   = 17;
constexpr int GSH_HIST  = 14;
// TwoLevel(log2_pc_entries, log2_hist)
constexpr int TL_TBL    = 9;
constexpr int TL_HIST   = 8;
// Perceptron(log2_entries, history_length)
constexpr int PCP_TBL   = 9;
constexpr int PCP_HIST  = 45;
// TAGEPredictor<H, NC>(idx_width, tag_width, num_comp, L1, ratio)  [tag != idx!]
constexpr int TP_H = 1024, TP_NC = 12;
constexpr int TP_IDX = 10, TP_TAG = 11, TP_L1 = 4;
constexpr float TP_RATIO = 1.6f;
// TAGEImproved<N_L, N_U, NC>(idx1, idx2, tag1(short), tag2(long), firstLong, minHist, maxHist, noSkip)
//   mirrors my_cond_branch_predictor.h
constexpr int TI_N_L = 8, TI_N_U = 16, TI_NC = 12;
constexpr int TI_BASE_IW = 9, TI_TAGE_IW = 9, TI_SHORT_TW = 7, TI_LONG_TW = 12;
constexpr int TI_FIRST_LONG = 9, TI_MIN_HIST = 4, TI_MAX_HIST = 2000;
// ===========================================================================

constexpr double TARGET_BYTES = 32.0 * 1024.0;   // 32 KiB
