#pragma once

#include <vector>
#include <cassert>

#include "primitives.h"


/*
API for traces to be ingested by a branch predictor

Traces provided as std::vector<BranchInstr> where

BranchInstr {
    BranchResult result: Enum member indicating whether branch is taken or not taken
    size_t pc: 
}

*/

struct BranchInstr {
    BranchResult direction;
    size_t pc;
    size_t id;

    BranchInstr(BranchResult direction, size_t pc, size_t id) :
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
    size_t pc=0,
    size_t id=0); // num_taken TAKEN followed by num_not_taken NOT_TAKEN

std::vector<BranchInstr> correlated_branch(
    size_t reps, 
    size_t pc1,
    size_t id1,
    size_t pc2,
    size_t id2,
    bool anti
); // branch2 = branch1 if NOT anti, else branch2 = NOT branch1

std::vector<BranchInstr> xor_correlated_branch(
    size_t reps, 
    size_t pc1,
    size_t id1,
    size_t pc2, 
    size_t id2,
    size_t pc3,
    size_t id3
); // branch3 = branch1 XOR branch2