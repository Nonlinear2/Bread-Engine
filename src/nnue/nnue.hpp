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
    
    bool large_difference;
    std::array<int, 32> added_buffer = {};
    std::array<int, 32> removed_buffer = {};
    uint8_t added_count = 0;
    uint8_t removed_count = 0;

    ModifiedFeatures() = default;

    ModifiedFeatures(int added, int removed, int captured):
        large_difference(false),
        added(added),
        removed(removed),
        captured(captured) {};

    ModifiedFeatures(const int* added, uint8_t added_count, const int* removed, uint8_t removed_count)
        : large_difference(true), added_count(added_count), removed_count(removed_count) {
        for (int i = 0; i < added_count; i++)
            added_buffer[i] = added[i];
        for (int i = 0; i < removed_count; i++)
            removed_buffer[i] = removed[i];
    }

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