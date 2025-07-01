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

constexpr int num_avx_registers = 16;
constexpr int int32_per_reg = 8;
constexpr int int16_per_reg = 16;
constexpr int int8_per_reg = 32;

inline const int HKP_size = 40960; // 64*64*5*2
inline const int acc_size = 256;

const std::vector<int> piece_to_index_w = {
    9, // white pawn
    3, // white knight
    5, // white bishop
    1, // white rook
    7, // white queen
    10, // white king
    8, // black pawn
    2, // black knight
    4, // black bishop
    0, // black rook
    6, // black queen
    11, // black king
};

const std::vector<int> piece_to_index_b = {
    8, // white pawn
    2, // white knight
    4, // white bishop
    0, // white rook
    6, // white queen
    11, // white king
    9, // black pawn
    3, // black knight
    5, // black bishop
    1, // black rook
    7, // black queen
    10, // black king
};

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

    int16_t accumulator[2*acc_size]; 
};

class NNUE {
    public:
    Accumulator accumulator; // array stored on the stack, as it will change often
    
    /*****************
    Feature transformer
    ******************/

    // 2*HKP_size -> 2*acc_size 

    static constexpr int ft_input_size = HKP_size;
    static constexpr int ft_output_size = acc_size;

    // weights are flattened 2d array, to be contiguous in memory.
    // otherways it would be an array of pointers pointing to scattered memory locations, 
    // which would be slower and unpractical.
    // weights are stored in row major
    int16_t* ft_weights = static_cast<int16_t*>(
        operator new[](sizeof(int16_t)*ft_input_size*ft_output_size, std::align_val_t{32})
    );
    int16_t* ft_bias = static_cast<int16_t*>(
        operator new[](sizeof(int16_t)*ft_output_size, std::align_val_t{32})
    );

    // all computations happen in int16. Scale is 127
    // to make sure that even after accumulation no overflows happen : there can be a maximum of 30 active input features,
    // so we need (sum of 30 weights) + bias < 32767
    // we need to make sure this limit is not reached before converting the model.

    // unclipped output is in accumulator

    // apply crelu16 and store
    int8_t ft_clipped_output[acc_size*2];

    /******
    Layer 2
    *******/

    // 512 -> 32

    // int8 weights with scale 64. Multiplication outputs in int16, so no overflows,
    // and sum is computed in int32. Maximum weights times maximum input with accumulation is 127*127*512 = 8258048
    // maximum bias is therefore (2,147,483,647-8,258,048)/32 = 66850799 which is totally fine.
    
    static constexpr int l2_input_size = 512;
    static constexpr int l2_output_size = 32;

    int8_t* l2_weights = static_cast<int8_t*>(
        operator new[](sizeof(int8_t)*l2_input_size*l2_output_size, std::align_val_t{32})
    );
    int32_t* l2_bias = static_cast<int32_t*>(
        operator new[](sizeof(int32_t)*l2_output_size, std::align_val_t{32})
    );

    // also, output is scaled back by 64, so total scale is still only 127. as we only do integer division,
    // error caused by the division is max 1/127.
    
    int32_t l2_unclipped_output[32];
    
    // apply crelu32 and store
    int8_t l2_clipped_output[32];

    /******
    Layer 3
    *******/
    // 32 -> 32

    static constexpr int l3_input_size = 32;
    static constexpr int l3_output_size = 32;

    int8_t* l3_weights = static_cast<int8_t*>(
        operator new[](sizeof(int8_t)*l3_input_size*l3_output_size, std::align_val_t{32})
    );
    int32_t* l3_bias = static_cast<int32_t*>(
        operator new[](sizeof(int32_t)*l3_output_size, std::align_val_t{32})
    );


    // max value is 32*(127*127) with 32 accumulations of the max possible output with the max possible weight.
    // this is 516128. Max bias is therefore (2,147,483,647 - 516,128)/32 = 67092734 which is fine.

    // still output is scaled back by 64, so scale is 127 times true output.
    int32_t l3_unclipped_output[32];
   
    // apply crelu32 and store
    int8_t l3_clipped_output[32];
   
    /******
    Layer 4
    *******/
    // 32 -> 1
    
    static constexpr int l4_input_size = 32;
    static constexpr int l4_output_size = 1;

    int8_t* l4_weights = static_cast<int8_t*>(
        operator new[](sizeof(int8_t)*l4_input_size*l4_output_size, std::align_val_t{32})
    );
    int16_t* l4_bias = static_cast<int16_t*>(
        operator new[](sizeof(int16_t)*l4_output_size, std::align_val_t{32})
    );


    // max output value is 1*(127*127) + bias, the max possible output with the max possible weight.
    // this is 16129+bias which is less than the max int16 if bias is less than (int16_max - 16129)/(127*64) = 2.04

    // for layers 2 and 3:
    void run(int8_t* input, int32_t* output, int input_size, int output_size, int8_t* weights, int32_t* bias);

    // output is not scaled back by 64, so scale is 64*127 times true output.
    int16_t run_output_layer(int8_t* input, int8_t* weights, int16_t* bias);

    void crelu16(int16_t *input, int8_t *output, int size);
    void crelu32(int32_t *input, int8_t *output, int size);

    NNUE();
    ~NNUE();

    void load_model();

    void compute_accumulator(const std::vector<int> active_features, bool color);

    void update_accumulator(const modified_features m_features, bool color);

    int run_cropped_nn(bool color);
};