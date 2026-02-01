#pragma once

#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <cstdint>
#include <stdio.h>
#include <chess.hpp>
#include <immintrin.h>
#include "constants.hpp"
#include "nnue_misc.hpp"

using namespace chess;

struct ModifiedFeatures {
    int added = -1;
    int removed = -1;
    int captured = -1;
    int added_2 = -1;

    ModifiedFeatures() = default;

    ModifiedFeatures(int added, int removed, int captured):
        added(added),
        removed(removed),
        captured(captured) {};

    ModifiedFeatures(int added, int added_2, int removed, int removed_2):
        added(added),
        added_2(added_2),
        removed(removed),
        captured(removed_2) {};

    bool valid() const;
};

namespace NNUE {

/*****************
Feature transformer
******************/

// 2*input_size -> 2*acc_size 

// weights are flattened 2d array, to be contiguous in memory.
// weights are stored in row major
extern int16_t* ft_weights;
extern int16_t* ft_bias;

/******
Layer 1
*******/

// 2*acc_size -> 1

extern int16_t* l1_weights;
extern int32_t* l1_bias;

int32_t run_L1(Accumulators& accumulators, Color stm, int bucket);

void init();
void cleanup();

void load_model();

void compute_accumulator(Accumulator& new_acc, const std::vector<int>& active_features);

void update_accumulator(Accumulator& prev_acc, Accumulator& new_acc, const ModifiedFeatures& m_features);

int run(Accumulators& accumulators, Color stm, int piece_count);

}; // namespace NNUE