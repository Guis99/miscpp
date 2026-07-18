#pragma once

#include <cstdint>
#include <vector>
#include <cassert>

#include "primitives.h"


/*
API for traces to be ingested by a branch predictor

Traces provided as std::vector<BranchInstr> where

BranchInstr {
    BranchResult direction: Enum member indicating whether branch is taken or not taken
    uint32_t pc:            Program counter (matches predictor API)
    uint32_t id:            Logical branch identifier (for per-stream scoring)
}
*/

struct BranchInstr {
    BranchResult direction;
    uint32_t pc;
    uint32_t id;

    BranchInstr(BranchResult direction, uint32_t pc, uint32_t id) :
            direction(direction),
            pc(pc),
            id(id) {}
};

/*
Trace construction methods:
Standard inputs for constructing traces:
    - length of sequence
    - address(es) of branch
*/

std::vector<BranchInstr> repeating_pattern(
    size_t num_taken,
    size_t num_not_taken,
    size_t reps,
    uint32_t pc=0,
    uint32_t id=0); // num_taken TAKEN followed by num_not_taken NOT_TAKEN

std::vector<BranchInstr> correlated_branch(
    size_t reps,
    uint32_t pc1,
    uint32_t id1,
    uint32_t pc2,
    uint32_t id2,
    bool anti
); // branch2 = branch1 if NOT anti, else branch2 = NOT branch1

std::vector<BranchInstr> xor_correlated_branch(
    size_t reps,
    uint32_t pc1,
    uint32_t id1,
    uint32_t pc2,
    uint32_t id2,
    uint32_t pc3,
    uint32_t id3
); // branch3 = branch1 XOR branch2

std::vector<BranchInstr> imitate_branch(
    size_t reps,
    uint32_t pc1,
    uint32_t id1
); // randomly selects k from [0,7], k T followed by k NT