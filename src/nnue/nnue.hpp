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
    int added_1 = -1;
    int added_2 = -1;
    int removed_1 = -1;
    int removed_2 = -1;

    enum {
        NORMAL, CAPTURE, CASTLING
    } type;

    ModifiedFeatures() = default;

    ModifiedFeatures(int added, int removed):
        added_1(added),
        removed_1(removed) {
            type = NORMAL;
        };

    ModifiedFeatures(int added, int removed, int captured):
        added_1(added),
        removed_1(removed),
        removed_2(captured) {
            type = CAPTURE;
        };

    ModifiedFeatures(int added, int added_2, int removed, int removed_2):
        added_1(added),
        added_2(added_2),
        removed_1(removed),
        removed_2(removed_2) {
            type = CASTLING;
        };

    bool valid() const;
};

namespace NNUE {

inline int input_bucket(Square sq, Color color){
    return INPUT_BUCKETS[sq.index() ^ (color ? 56 : 0)];
}

inline int feature(Color persp, Color c, PieceType pt, Square sq, Square king_sq){
    int flip = persp ? 56 : 0; // mirror vertically by flipping bits 6, 5 and 4
    int mirror = king_sq.file() >= File::FILE_E ? 7 : 0; // mirror horizontally by flipping last 3 bits

    return 768 * INPUT_BUCKETS[king_sq.index() ^ flip]
         + 384 * (c ^ persp)
         + 64 * pt
         + (sq.index() ^ flip ^ mirror);
}

inline int feature(Color persp, Piece p, Square sq, Square king_sq){
    int flip = persp ? 56 : 0; // mirror vertically by flipping bits 6, 5 and 4
    int mirror = king_sq.file() >= File::FILE_E ? 7 : 0; // mirror horizontally by flipping last 3 bits

    return 768 * INPUT_BUCKETS[king_sq.index() ^ flip]
         + 384 * (p.color() ^ persp)
         + 64 * p.type()
         + (sq.index() ^ flip ^ mirror);
}

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

void update_accumulator(Accumulator& prev_acc, Accumulator& new_acc, const ModifiedFeatures& m_features);

int run(Accumulators& accumulators, Color stm, int piece_count);

}; // namespace NNUE