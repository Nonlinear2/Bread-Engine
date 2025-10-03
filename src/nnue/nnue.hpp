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
#include "nnue_misc.hpp"

constexpr int INPUT_SIZE = 768;
constexpr int ACC_SIZE = 512;

constexpr int OUTPUT_BUCKET_COUNT = 8;

constexpr int L1_INPUT_SIZE = 2 * ACC_SIZE;
constexpr int L1_OUTPUT_SIZE = 1;

constexpr int L0_WEIGHTS_SIZE = INPUT_SIZE * ACC_SIZE;
constexpr int L0_BIAS_SIZE = ACC_SIZE;

constexpr int L1_WEIGHTS_SIZE = L1_INPUT_SIZE * L1_OUTPUT_SIZE;
constexpr int L1_BIAS_SIZE = L1_OUTPUT_SIZE;

constexpr int BUCKETED_L1_WEIGHTS_SIZE = OUTPUT_BUCKET_COUNT * L1_WEIGHTS_SIZE;
constexpr int BUCKETED_L1_BIAS_SIZE = OUTPUT_BUCKET_COUNT * L1_BIAS_SIZE;

struct modified_features {
    int added;
    int removed;
    int captured;

    modified_features(int added, int removed, int captured):
        added(added),
        removed(removed),
        captured(captured) {};
};

class Accumulator {
    public:
    int16_t* operator[](bool color);

    int16_t accumulator[2*ACC_SIZE]; 
};

class NNUE {
    public:
    Accumulator accumulator; // array stored on the stack, as it will change often
    
    /*****************
    Feature transformer
    ******************/

    // 2*input_size -> 2*acc_size 

    // weights are flattened 2d array, to be contiguous in memory.
    // otherways it would be an array of pointers pointing to scattered memory locations, 
    // which would be slower and unpractical.
    // weights are stored in row major
    int16_t* ft_weights;
    int16_t* ft_bias;

    // all computations happen in int16. Scale is 127
    // to make sure that even after accumulation no overflows happen : there can be a maximum of 30 active input features,
    // so we need (sum of 30 weights) + bias < 32767
    // we need to make sure this limit is not reached before converting the model.

    // unclipped output is in accumulator

    // apply crelu16 and store
    int16_t ft_clipped_output[ACC_SIZE*2];

    /******
    Layer 1
    *******/

    // 2*acc_size -> 1

    // int8 weights with scale 64. Multiplication outputs in int16, so no overflows,
    // and sum is computed in int32. Maximum weights times maximum input with accumulation is 127*127*512 = 8258048
    // maximum bias is therefore (2,147,483,647-8,258,048)/32 = 66850799 which is totally fine.

    int16_t* l1_weights;
    int32_t* l1_bias;

    // also, output is scaled back by 64, so total scale is still only 127. as we only do integer division,
    // error caused by the division is max 1/127.
    
    // max output value is 1*(127*127) + bias, the max possible output with the max possible weight.
    // this is 16129+bias which is less than the max int16 if bias is less than (int16_max - 16129)/(127*64) = 2.04

    // output is not scaled back by 64, so scale is 64*127 times true output.
    int32_t run_output_layer(int16_t* input, int16_t* weights, int32_t* bias, int bucket);

    NNUE();
    ~NNUE();

    void load_model();

    void compute_accumulator(const std::vector<int> active_features, bool color);

    void update_accumulator(const modified_features m_features, bool color);

    int run_cropped_nn(bool color, int piece_count);
};