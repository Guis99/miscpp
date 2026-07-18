#include "traces.h"
#include "primitives.h"

std::vector<BranchInstr> repeating_pattern(
    size_t num_taken,
    size_t num_not_taken,
    size_t reps,
    uint32_t pc,
    uint32_t id
) {
    size_t seq_len = reps * (num_taken + num_not_taken);
    std::vector<BranchInstr> out; out.reserve(seq_len);
    for (size_t i = 0; i < reps; i++) {
        for (size_t j = 0; j < num_taken; j++) {
            out.emplace_back(BranchResult::TAKEN, pc, id);
        }
        for (size_t j = 0; j < num_not_taken; j++) {
            out.emplace_back(BranchResult::NOT_TAKEN, pc, id);
        }
    }

    assert(out.size() == seq_len);
    return out;
}

std::vector<BranchInstr> correlated_branch(
    size_t reps,
    uint32_t pc1,
    uint32_t id1,
    uint32_t pc2,
    uint32_t id2,
    bool anti
) {
    std::vector<BranchInstr> out; out.reserve(2 * reps);
    uint64_t state = 0x9e3779b97f4a7c15ull;
    for (size_t i = 0; i < reps; i++) {
        state = state * 6364136223846793005ull + 1442695040888963407ull;
        BranchResult direction = static_cast<BranchResult>(state >> 63);
        out.emplace_back(direction, pc1, id1);
        out.emplace_back(static_cast<BranchResult>(static_cast<bool>(direction) ^ anti), pc2, id2);
    }

    assert(out.size() == 2 * reps);
    return out;
}

std::vector<BranchInstr> xor_correlated_branch(
    size_t reps,
    uint32_t pc1,
    uint32_t id1,
    uint32_t pc2,
    uint32_t id2,
    uint32_t pc3,
    uint32_t id3
) {
    std::vector<BranchInstr> out; out.reserve(3 * reps);
    uint64_t state = 0x9e3779b97f4a7c15ULL;
    for (size_t i = 0; i < reps; i++) {
        state ^= state >> 12;
        state ^= state << 25;
        state ^= state >> 27;
        uint64_t bits = state * 0x2545f4914f6cdd1dULL;

        bool first = bits & 1ULL;
        bool second = bits & 2ULL;
        out.emplace_back(first ? BranchResult::TAKEN : BranchResult::NOT_TAKEN, pc1, id1);
        out.emplace_back(second ? BranchResult::TAKEN : BranchResult::NOT_TAKEN, pc2, id2);
        out.emplace_back((first != second) ? BranchResult::TAKEN : BranchResult::NOT_TAKEN, pc3, id3);
    }
    assert(out.size() == 3 * reps);
    return out;
}

std::vector<BranchInstr> imitate_branch(
    size_t reps,
    uint32_t pc1,
    uint32_t id1
) {
    std::vector<BranchInstr> out; out.reserve(7 * 2 * reps);
    uint64_t state = 0x9e3779b97f4a7c15ULL;
    for (size_t i = 0; i < reps; i++) {
        state ^= state >> 12;
        state ^= state << 25;
        state ^= state >> 27;
        uint64_t bits = state * 0x2545f4914f6cdd1dULL;

        uint64_t first = bits & 7ULL; // mask to max 7 (111)
        for (size_t i = 0; i < first; i++) {
            out.emplace_back(BranchResult::TAKEN, pc1, id1);
        } 

        for (size_t i = 0; i < first; i++) {
            out.emplace_back(BranchResult::NOT_TAKEN, pc1, id1);
        } 
    }
    return out;
}
