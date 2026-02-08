#pragma once

#include <assert.h>
#include <cstdint>
#include <cmath>
#include <array>

/*************
NNUE constants
*************/

constexpr int INPUT_BUCKET_COUNT = 8;
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

constexpr int OUTPUT_BUCKET_COUNT = 8;

constexpr int INPUT_SIZE = 768 * INPUT_BUCKET_COUNT;
constexpr int ACC_SIZE = 1024;

constexpr int L1_INPUT_SIZE = 2 * ACC_SIZE;
constexpr int L1_OUTPUT_SIZE = 1;

constexpr int L0_WEIGHTS_SIZE = INPUT_SIZE * ACC_SIZE;
constexpr int L0_BIAS_SIZE = ACC_SIZE;

constexpr int L1_WEIGHTS_SIZE = L1_INPUT_SIZE * L1_OUTPUT_SIZE;
constexpr int L1_BIAS_SIZE = L1_OUTPUT_SIZE;

constexpr int BUCKETED_L1_WEIGHTS_SIZE = OUTPUT_BUCKET_COUNT * L1_WEIGHTS_SIZE;
constexpr int BUCKETED_L1_BIAS_SIZE = OUTPUT_BUCKET_COUNT * L1_BIAS_SIZE;

using Accumulator = std::array<int16_t, ACC_SIZE>;
using Accumulators = std::array<Accumulator, 2>;

/****************
general constants
****************/

constexpr int PIECETYPE_COUNT = 6;

constexpr int TT_MIN_SIZE = 2;
constexpr int TT_MAX_SIZE = 4096;

constexpr int ENGINE_MAX_DEPTH = 63;

constexpr int QSEARCH_SOFT_DEPTH_LIMIT = 6;
constexpr int QSEARCH_HARD_DEPTH_LIMIT = 15;

constexpr int MAX_PLY = 256;
constexpr int STACK_PADDING_SIZE = 2;

constexpr int BENCHMARK_DEPTH = 12;

constexpr int DEPTH_UNSEARCHED = -1;
constexpr int DEPTH_QSEARCH = 0;

constexpr int BEST_MOVE_SCORE = 100'000;
constexpr int WORST_MOVE_SCORE = -100'000;
constexpr int BAD_SEE_THRESHOLD = 10'000;

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