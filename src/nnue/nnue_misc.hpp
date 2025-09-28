#include <immintrin.h>
#include <cstdint>
#include <cmath>
#include <chess.hpp>

constexpr int NUM_AVX_REGISTERS = 16;
constexpr int INT32_PER_REG = 8;
constexpr int INT16_PER_REG = 16;
constexpr int INT8_PER_REG = 32;

namespace NNUE_UTILS {

void crelu32_to_8(int32_t *input, int8_t *output, int size);
void crelu16_to_8(int16_t *input, int8_t *output, int input_size);

}; // namespace NNUE_UTILS