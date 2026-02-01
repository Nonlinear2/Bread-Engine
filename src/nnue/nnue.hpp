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

struct Features {
    int size;
    int features[32];

    Features() = default;
    Features(int size): size(size) {}

    int* begin() { return features; }
    int* end() { return features + size; }

    const int* begin() const { return features; }
    const int* end() const { return features + size; }

    int& operator[](int index){
        return features[index];
    }
};

struct ModifiedFeatures {
    int added;
    int removed;
    int captured;

    ModifiedFeatures():
        added(-1),
        removed(-1),
        captured(-1) {};

    ModifiedFeatures(int added, int removed, int captured):
        added(added),
        removed(removed),
        captured(captured) {};

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

void compute_accumulator(Accumulator& new_acc, const Features active_features);

void update_accumulator(Accumulator& prev_acc, Accumulator& new_acc, const ModifiedFeatures m_features);

int run(Accumulators& accumulators, Color stm, int piece_count);

}; // namespace NNUE