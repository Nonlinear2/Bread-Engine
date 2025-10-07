#include <cstdint>
#include <cmath>
#include <chess.hpp>

#if defined(__AVX2__)
    #include <immintrin.h>

    using vec_int8 = __m256i;
    using vec_uint8 = __m256i;
    using vec_int16 = __m256i;
    using vec_uint16 = __m256i;
    using vec_int32 = __m256i;
    using vec_uint32 = __m256i;

    inline vec_int8 setzero_epi8() {
        return _mm256_setzero_si256();
    }

    inline vec_int16 setzero_epi16() {
        return _mm256_setzero_si256();
    }

    inline vec_int32 setzero_epi32() {
        return _mm256_setzero_si256();
    }

    inline vec_int16 set1_epi16(int i) {
        return _mm256_set1_epi16(i);
    }

    inline vec_int32 set1_epi32(int i) {
        return _mm256_set1_epi32(i);
    }

    inline vec_int8 load_epi8(int8_t* ptr) {
        return _mm256_loadu_si256((const __m256i*)ptr);
    }

    inline vec_int16 load_epi16(int16_t* ptr) {
        return _mm256_loadu_si256((const __m256i*)ptr);
    }

    inline vec_int32 load_epi32(int32_t* ptr) {
        return _mm256_loadu_si256((const __m256i*)ptr);
    }

    inline void store_epi8(int8_t* ptr, vec_int8 v) {
        _mm256_storeu_si256((__m256i*)ptr, v);
    }

    inline void store_epi16(int16_t* ptr, vec_int16 v) {
        _mm256_storeu_si256((__m256i*)ptr, v);
    }

    inline void store_epi32(int32_t* ptr, vec_int32 v) {
        _mm256_storeu_si256((__m256i*)ptr, v);
    }

    inline vec_int8 packs_epi16(vec_int16 v1, vec_int16 v2) {
        return _mm256_packs_epi16(v1, v2);
    }

    inline vec_int16 packs_epi32(vec_int32 v1, vec_int32 v2) {
        return _mm256_packs_epi32(v1, v2);
    }

    inline vec_int8 packus_epi16(vec_int16 v1, vec_int16 v2) {
        return _mm256_packus_epi16(v1, v2);
    }

    inline vec_int16 packus_epi32(vec_int32 v1, vec_int32 v2) {
        return _mm256_packus_epi32(v1, v2);
    }

    template<int mask>
    inline vec_int32 permute4x64_epi64(vec_int32 v) {
        return _mm256_permute4x64_epi64(v, mask);
    }

    template<int mask>
    inline vec_int32 permute2x128_si256(vec_int32 v1, vec_int32 v2) {
        return _mm256_permute2x128_si256(v1, v2, mask);
    }

    inline vec_int8 max_epi8(vec_int8 v1, vec_int8 v2) {
        return _mm256_max_epi8(v1, v2);
    }

    inline vec_int16 max_epi16(vec_int16 v1, vec_int16 v2) {
        return _mm256_max_epi16(v1, v2);
    }

    inline vec_int16 min_epi16(vec_int16 v1, vec_int16 v2) {
        return _mm256_min_epi16(v1, v2);
    }

    inline vec_int16 add_epi16(vec_int16 v1, vec_int16 v2) {
        return _mm256_add_epi16(v1, v2);
    }

    inline vec_int32 add_epi32(vec_int32 v1, vec_int32 v2) {
        return _mm256_add_epi32(v1, v2);
    }

    inline vec_int16 sub_epi16(vec_int16 v1, vec_int16 v2) {
        return _mm256_sub_epi16(v1, v2);
    }

    inline vec_int32 madd_epi16(vec_int16 v1, vec_int16 v2) {
        return _mm256_madd_epi16(v1, v2);
    }

    inline vec_int32 hadd_epi32(vec_int32 v1, vec_int32 v2) {
        return _mm256_hadd_epi32(v1, v2);
    }

    inline vec_int16 mullo_epi16(vec_int16 v1, vec_int16 v2) {
        return _mm256_mullo_epi16(v1, v2);
    }

#else
    #error "bread requires the AVX2 instruction set to run."
#endif

constexpr int NUM_AVX_REGISTERS = 16;
constexpr int INT32_PER_REG = sizeof(vec_int32) / sizeof(int32_t);
constexpr int INT16_PER_REG = sizeof(vec_int16) / sizeof(int16_t);
constexpr int INT8_PER_REG = sizeof(vec_int8) / sizeof(int8_t);

namespace NNUE_UTILS {

vec_int8 crelu32_to_8(vec_int32 in_1, vec_int32 in_2, vec_int32 in_3, vec_int32 in_4, vec_int8 zero);
vec_int8 crelu16_to_8(vec_int16 in_1, vec_int16 in_2);
vec_int16 crelu16_to_16(vec_int16 in, vec_int16 zero, vec_int16 qscale);

int32_t reduce1_epi32(vec_int32 input); // horizontal add 1 int32 avx register.
vec_int32 reduce8_epi32(vec_int32* inputs);

}; // namespace NNUE_UTILS