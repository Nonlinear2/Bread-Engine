#pragma once

#include <assert.h>
#include <cstdint>
#include <cmath>
#include <array>

/*************
NNUE constants
*************/

constexpr int NUM_INPUT_BUCKETS = 8;
constexpr int INPUT_BUCKETS[64] = {
    0, 1, 2, 3,  3, 2, 1, 0,
    4, 4, 5, 5,  5, 5, 4, 4,
    6, 6, 6, 6,  6, 6, 6, 6,
    6, 6, 6, 6,  6, 6, 6, 6,
    7, 7, 7, 7,  7, 7, 7, 7,
    7, 7, 7, 7,  7, 7, 7, 7,
    7, 7, 7, 7,  7, 7, 7, 7,
    7, 7, 7, 7,  7, 7, 7, 7,
};

constexpr int NUM_OUTPUT_BUCKETS = 8;

constexpr int INPUT_SIZE = 768 * NUM_INPUT_BUCKETS;
constexpr int ACC_SIZE = 1024;

constexpr int L1_INPUT_SIZE = ACC_SIZE;
constexpr int L1_OUTPUT_SIZE = 16;

constexpr int L2_INPUT_SIZE = L1_OUTPUT_SIZE * 2; // * 2 comes from dual activation
constexpr int L2_OUTPUT_SIZE = 1;


constexpr int L0_WEIGHTS_SIZE = INPUT_SIZE * ACC_SIZE;
constexpr int L0_BIAS_SIZE = ACC_SIZE;

constexpr int L1_WEIGHTS_SIZE = L1_INPUT_SIZE * L1_OUTPUT_SIZE;
constexpr int L1_BIAS_SIZE = L1_OUTPUT_SIZE;

constexpr int L2_WEIGHTS_SIZE = L2_INPUT_SIZE * L2_OUTPUT_SIZE;
constexpr int L2_BIAS_SIZE = L2_OUTPUT_SIZE;

constexpr int BUCKETED_L1_WEIGHTS_SIZE = NUM_OUTPUT_BUCKETS * L1_WEIGHTS_SIZE;
constexpr int BUCKETED_L1_BIAS_SIZE = NUM_OUTPUT_BUCKETS * L1_BIAS_SIZE;

constexpr int BUCKETED_L2_WEIGHTS_SIZE = NUM_OUTPUT_BUCKETS * L2_WEIGHTS_SIZE;
constexpr int BUCKETED_L2_BIAS_SIZE = NUM_OUTPUT_BUCKETS * L2_BIAS_SIZE;

using Accumulator = std::array<int16_t, ACC_SIZE>;
using Accumulators = std::array<Accumulator, 2>;
using ClampedAccumulators = std::array<std::array<int8_t, ACC_SIZE>, 2>;

constexpr int FT_PERMUTATION[ACC_SIZE / 2] = {
    122, 147, 474, 17, 491, 313, 457, 24, 66, 9, 247, 222, 466, 68, 286, 365, 312, 351, 458, 478, 38, 73, 124, 145, 256, 325, 387, 42, 88, 128, 164, 245, 369, 355, 29, 381, 492, 144, 268, 329, 335, 395, 421, 276, 334, 416, 417, 161, 410, 473, 148, 314, 460, 126, 383, 130, 477, 135, 201, 22, 347, 14, 200, 454, 422, 58, 385, 177, 209, 54, 77, 337, 102, 216, 393, 4, 255, 442, 45, 221, 234, 269, 430, 446, 212, 450, 120, 65, 211, 288, 12, 295, 82, 87, 414, 108, 81, 93, 319, 168, 56, 366, 367, 506, 509, 443, 67, 375, 411, 35, 203, 129, 143, 382, 6, 408, 503, 273, 346, 404, 285, 330, 467, 118, 472, 440, 246, 327, 510, 413, 97, 195, 197, 306, 384, 428, 198, 415, 464, 78, 113, 160, 490, 48, 305, 13, 217, 362, 418, 455, 175, 192, 193, 407, 74, 133, 436, 300, 150, 5, 425, 462, 86, 3, 264, 8, 72, 92, 157, 433, 50, 501, 321, 89, 165, 191, 349, 333, 339, 171, 293, 398, 51, 55, 132, 249, 302, 496, 153, 258, 292, 253, 298, 107, 326, 448, 476, 431, 84, 119, 237, 125, 183, 79, 156, 166, 469, 49, 265, 57, 499, 263, 278, 163, 174, 254, 370, 497, 279, 493, 28, 85, 60, 380, 290, 323, 173, 316, 136, 184, 322, 461, 225, 401, 188, 373, 424, 452, 379, 360, 95, 508, 481, 53, 445, 104, 251, 468, 206, 437, 317, 459, 356, 419, 109, 162, 187, 484, 489, 259, 112, 155, 181, 438, 377, 96, 114, 299, 218, 345, 406, 359, 432, 494, 315, 34, 70, 282, 227, 172, 106, 152, 338, 358, 123, 210, 69, 140, 396, 447, 83, 483, 439, 348, 304, 220, 449, 261, 267, 52, 25, 134, 364, 291, 303, 487, 2, 0, 39, 199, 399, 480, 146, 158, 33, 423, 465, 224, 332, 471, 244, 289, 507, 36, 41, 205, 297, 397, 16, 44, 116, 32, 486, 190, 270, 341, 142, 185, 121, 400, 427, 352, 239, 230, 27, 412, 170, 371, 229, 361, 394, 238, 11, 19, 151, 378, 80, 10, 71, 30, 110, 137, 429, 444, 90, 127, 226, 7, 281, 368, 242, 386, 213, 231, 354, 426, 101, 420, 331, 389, 223, 40, 59, 485, 296, 103, 343, 409, 311, 236, 94, 18, 47, 475, 208, 20, 91, 176, 275, 390, 284, 310, 463, 405, 266, 482, 504, 241, 344, 99, 403, 248, 274, 105, 167, 235, 141, 502, 307, 179, 115, 207, 196, 250, 178, 180, 43, 336, 324, 111, 402, 194, 98, 219, 75, 434, 350, 495, 169, 320, 63, 479, 214, 232, 287, 271, 138, 228, 511, 26, 272, 441, 204, 280, 435, 309, 318, 76, 149, 182, 202, 260, 139, 37, 240, 23, 392, 283, 31, 277, 233, 353, 154, 117, 62, 376, 159, 15, 294, 363, 215, 372, 470, 453, 186, 131, 100, 21, 388, 46, 1, 488, 505, 500, 342, 340, 252, 61, 257, 498, 308, 357, 328, 301, 456, 243, 189, 262, 451, 391, 64, 374
};

/****************
history constants
****************/

constexpr int PAWN_CORRHIST_SIZE = 16384;
constexpr int MINOR_CORRHIST_SIZE = 16384;
constexpr int MAJOR_CORRHIST_SIZE = 16384;
constexpr int NONPAWN_CORRHIST_SIZE = 16384;


/**************
chess constants
**************/

constexpr int NUM_COLORS = 2;
constexpr int NUM_PIECES = 12;
constexpr int NUM_PIECETYPES = 6;
constexpr int NUM_SQUARES = 64;

/****************
general constants
****************/

constexpr int NUM_GENFENS_RANDOM_MOVES = 10;
constexpr int GENFENS_FILTER_DEPTH = 9;
constexpr int GENFENS_MAX_VALUE = 600;

constexpr int TT_MIN_SIZE = 2;
constexpr int TT_MAX_SIZE = 4096;

constexpr int MAX_PLY = 256;
constexpr int STACK_PADDING_SIZE = 2;

constexpr int BENCHMARK_DEPTH = 12;
constexpr int LONG_BENCHMARK_DEPTH = 18;

constexpr int ENGINE_MAX_DEPTH = 255;

constexpr int DEPTH_UNSEARCHED = -1;
constexpr int DEPTH_QSEARCH = 0;

constexpr int QSEARCH_SOFT_DEPTH_LIMIT = 6;
constexpr int QSEARCH_HARD_DEPTH_LIMIT = 15;

/*****
scores
*****/

constexpr int BEST_MOVE_SCORE = 100'000;
constexpr int WORST_MOVE_SCORE = -100'000;

constexpr int MAX_MATE_PLY = 200;

constexpr int BEST_VALUE = 32'498;
constexpr int TB_VALUE = 32'499;
constexpr int MATE_VALUE = 32'700;
constexpr int INFINITE_VALUE = 32'701;
constexpr int NO_VALUE = 32'702;

constexpr int SEE_KING_VALUE = 150'000;

// |    value            |          name                    | is_valid  | is_mate   | is_decisive |
// ================================================================================================
// |       ...           |       xxx                        |   crash   | crash     | crash
// |      -32702         | -NO_VALUE                        |   false   | false     | crash
// |      -32701         | -INFINITE_VALUE                  |   false   | false     | crash
// |      -32700         | -MATE_VALUE (mate in 0)          |   true    | true      | true
// | -32699, ..., -32501 |       ...                        |   true    | true      | true
// |      -32500         | -MATE_VALUE+200 (mate in 200)    |   true    | true      | true
// |      -32499         | -TB_VALUE                        |   true    | false     | true

// |      -32498         | -BEST_VALUE                      |   true    | false     | false
// | -32498, ..., 32498  | valid eval range                 |   true    | false     | false
// |       32498         | BEST_VALUE                       |   true    | false     | false

// |       32499         | TB_VALUE                         |   true    | false     | true
// |       32500         | MATE_VALUE-200 (mate in 200)     |   true    | true      | true
// |  32501, ..., 32699  |       ...                        |   true    | true      | true
// |       32700         | MATE_VALUE (mate in 0)           |   true    | true      | true
// |       32701         | INFINITE_VALUE                   |   false   | false     | crash
// |       32702         | NO_VALUE                         |   false   | false     | crash
// |       ...           |       xxx                        |   crash   | crash     | crash

constexpr int is_valid(int value){
    assert(std::abs(value) <= NO_VALUE);
    return std::abs(value) < INFINITE_VALUE;
}

constexpr bool is_mate(int value){
    assert(std::abs(value) <= NO_VALUE);
    return (std::abs(value) >= MATE_VALUE - MAX_MATE_PLY && std::abs(value) <= MATE_VALUE);
}

constexpr bool is_win(int value){
    assert(is_valid(value));
    return value >= TB_VALUE;
}

constexpr bool is_loss(int value){
    assert(is_valid(value));
    return value <= -TB_VALUE;
}

constexpr bool is_decisive(int value){
    return is_win(value) || is_loss(value);
}

constexpr bool is_regular_eval(int value, bool zws_safe = true){
    assert(std::abs(value) <= NO_VALUE);
    return std::abs(value) <= BEST_VALUE - (zws_safe ? 1 : 0);
}