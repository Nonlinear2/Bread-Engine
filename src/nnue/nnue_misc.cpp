#include "nnue_misc.hpp"

// input size must be a multiple of 32
// there is no max size.
void NNUE_UTILS::crelu32_to_8(int32_t *input, int8_t *output, int size){

    assert(size % INT8_PER_REG == 0);

    const int num_regs = size / INT8_PER_REG;
    const __m256i zero = _mm256_setzero_si256();

    for (int i = 0; i < num_regs; i++){
        __m256i in_1 = _mm256_loadu_si256((const __m256i*)&input[(4*i)*INT32_PER_REG]); // load int32
        __m256i in_2 = _mm256_loadu_si256((const __m256i*)&input[(4*i+1)*INT32_PER_REG]); // load int32
        __m256i in_3 = _mm256_loadu_si256((const __m256i*)&input[(4*i+2)*INT32_PER_REG]); // load int32
        __m256i in_4 = _mm256_loadu_si256((const __m256i*)&input[(4*i+3)*INT32_PER_REG]); // load int32

        in_1 = _mm256_permute4x64_epi64(_mm256_packs_epi32(in_1, in_2), 0b10'00'11'01);
        in_3 = _mm256_permute4x64_epi64(_mm256_packs_epi32(in_3, in_4), 0b10'00'11'01);

        __m256i out = _mm256_packs_epi16(in_1, in_3);
        out = _mm256_max_epi8(out, zero); // packs saturates at 127, so only max is applied
        out = _mm256_permute4x64_epi64(out, 0b01'11'00'10);
        _mm256_storeu_si256((__m256i*)&output[i*INT8_PER_REG], out); // store int8
    }
}

// input size must be a multiple of 32
// there is no max size.
void NNUE_UTILS::crelu16_to_8(int16_t *input, int8_t *output, int input_size){

    assert(input_size % INT8_PER_REG == 0);

    const int num_regs = input_size / INT8_PER_REG;
    const __m256i zero = _mm256_setzero_si256();

    for (int i = 0; i < num_regs; i++){
        __m256i in_1 = _mm256_loadu_si256((const __m256i*)&input[(2*i)*INT16_PER_REG]); // load int16
        __m256i in_2 = _mm256_loadu_si256((const __m256i*)&input[(2*i+1)*INT16_PER_REG]); // load int16
        // packs sets negative values to 0 and saturates at 255, which effectively applies relu
        __m256i out = _mm256_packus_epi16(in_1, in_2);
        out = _mm256_permute4x64_epi64(out, 0b11011000); // undo the packus shuffle
        _mm256_storeu_si256((__m256i*)&output[i*INT8_PER_REG], out); // store int8
    }
}
