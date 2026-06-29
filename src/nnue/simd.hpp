#pragma once

#if defined(__AVX512F__)
    #define USE_AVX512
    #define HAS_VNNI512 __AVX512VNNI__
#elif defined(__AVX2__)
    #define USE_AVX2
    // #define HAS_VNNI256 __AVXVNNI__
#else
    #error "bread requires the AVX2 instruction set to run."
#endif

#include <immintrin.h>

#ifdef USE_AVX512
    using vec_int8 = __m512i;
    using vec_uint8 = __m512i;
    using vec_int16 = __m512i;
    using vec_uint16 = __m512i;
    using vec_int32 = __m512i;
    using vec_uint32 = __m512i;

    inline vec_int8 setzero_epi8() {
        return _mm512_setzero_si512();
    }

    inline vec_int16 setzero_epi16() {
        return _mm512_setzero_si512();
    }

    inline vec_int32 setzero_epi32() {
        return _mm512_setzero_si512();
    }

    inline vec_int16 set1_epi16(int i) {
        return _mm512_set1_epi16(i);
    }

    inline vec_int32 set1_epi32(int i) {
        return _mm512_set1_epi32(i);
    }

    inline vec_int8 load_epi8(int8_t* ptr) {
        return _mm512_loadu_si512((const __m256i*)ptr);
    }

    inline vec_int16 load_epi16(int16_t* ptr) {
        return _mm512_loadu_si512((const __m256i*)ptr);
    }

    inline vec_int32 load_epi32(int32_t* ptr) {
        return _mm512_loadu_si512((const __m256i*)ptr);
    }

    inline void store_epi8(int8_t* ptr, vec_int8 v) {
        _mm512_storeu_si512((__m256i*)ptr, v);
    }

    inline void storeu_epi8(uint8_t* ptr, vec_uint8 v) {
        _mm512_storeu_si512((__m256i*)ptr, v);
    }

    inline void store_epi16(int16_t* ptr, vec_int16 v) {
        _mm512_storeu_si512((__m256i*)ptr, v);
    }

    inline void store_epi32(int32_t* ptr, vec_int32 v) {
        _mm512_storeu_si512((__m256i*)ptr, v);
    }

    inline vec_int8 packs_epi16(vec_int16 v1, vec_int16 v2) {
        return _mm512_packs_epi16(v1, v2);
    }

    inline vec_int16 packs_epi32(vec_int32 v1, vec_int32 v2) {
        return _mm512_packs_epi32(v1, v2);
    }

    inline vec_int8 packus_epi16(vec_int16 v1, vec_int16 v2) {
        return _mm512_packus_epi16(v1, v2);
    }

    inline vec_int16 packus_epi32(vec_int32 v1, vec_int32 v2) {
        return _mm512_packus_epi32(v1, v2);
    }

    inline vec_int8 max_epi8(vec_int8 v1, vec_int8 v2) {
        return _mm512_max_epi8(v1, v2);
    }

    inline vec_int16 max_epi16(vec_int16 v1, vec_int16 v2) {
        return _mm512_max_epi16(v1, v2);
    }

    inline vec_int16 min_epi16(vec_int16 v1, vec_int16 v2) {
        return _mm512_min_epi16(v1, v2);
    }

    inline vec_int16 add_epi16(vec_int16 v1, vec_int16 v2) {
        return _mm512_add_epi16(v1, v2);
    }

    inline vec_int32 add_epi32(vec_int32 v1, vec_int32 v2) {
        return _mm512_add_epi32(v1, v2);
    }

    inline vec_int16 sub_epi16(vec_int16 v1, vec_int16 v2) {
        return _mm512_sub_epi16(v1, v2);
    }

    inline vec_int32 madd_epi16(vec_int16 v1, vec_int16 v2) {
        return _mm512_madd_epi16(v1, v2);
    }

    inline vec_int16 mullo_epi16(vec_int16 v1, vec_int16 v2) {
        return _mm512_mullo_epi16(v1, v2);
    }
    
    inline vec_int16 mulhi_epi16(vec_int16 v1, vec_int16 v2) {
        return _mm512_mulhi_epi16(v1, v2);
    }

    inline vec_int16 slli_epi16(vec_int16 v, int i) {
        return _mm512_slli_epi16(v, i);
    }

    inline vec_int32 srai_epi32(vec_int32 v, int i) {
        return _mm512_srai_epi32(v, i);
    }

    inline __m256 cvtsepi32_epi16(vec_int32 v) {
        return _mm512_cvtsepi32_epi16(v);
    }
    

    #ifdef HAS_VNNI512
        inline vec_int32 dpbusd_epi32(vec_int32 sum, vec_int8 v1, vec_int8 v2) {
            return _mm512_dpbusd_epi32(sum, v1, v2);
        }
    #else
        inline vec_int32 dpbusd_epi32(vec_int32 sum, vec_int8 v1, vec_int8 v2) {
            const vec_int16 prod = maddubs_epi16(v1, v2);
            return add_epi32(sum, madd_epi16(prod, set1_epi16(1)));
        }
    #endif

    // defined when USE_AVX512 only:

    inline int reduce1_epi32(vec_int32 v) {
        return _mm512_reduce_add_epi32(v);
    }

    inline vec_int8 permutexvar_epi32(vec_int8 mask, vec_int8 v) {
        return _mm512_permutexvar_epi32(mask, v);
    }

    using vec_int8_half = __m256i;
    using vec_int16_half = __m256i;
    using vec_int32_half = __m256i;

    inline vec_int16_half min_epi16_half(vec_int16_half a, vec_int16_half b) {
        return _mm256_min_epi16(a, b);
    }

    inline vec_int16_half max_epi16_half(vec_int16_half a, vec_int16_half b) {
        return _mm256_max_epi16(a, b);
    }

    inline vec_int16_half setzero_epi16_half() {
        return _mm256_setzero_si256();
    }

    inline vec_int16_half set1_epi16_half(int16_t v) {
        return _mm256_set1_epi16(v);
    }

    inline vec_int16_half load_epi16_half(int16_t* ptr) {
        return _mm256_loadu_si256((__m256i*)ptr);
    }

    inline void store_epi16_half(int16_t* p, vec_int16_half v) {
        _mm256_storeu_si256((__m256i*)p, v);
    }

    inline vec_int32_half hadd_epi32_half(vec_int32_half a, vec_int32_half b) {
        return _mm256_hadd_epi32(a, b);
    }

    inline void store_epi32_half(int32_t* p, vec_int32_half v) {
        _mm256_storeu_si256((__m256i*)p, v);
    }

    inline vec_int32_half set1_epi32_half(int i) {
        return _mm256_set1_epi32(i);
    }

    inline vec_int16_half mullo_epi16_half(vec_int16_half a, vec_int16_half b) {
        return _mm256_mullo_epi16(a, b);
    }

    inline vec_int32_half madd_epi16_half(vec_int16_half a, vec_int16_half b) {
        return _mm256_madd_epi16(a, b);
    }

    inline vec_int32_half add_epi32_half(vec_int32_half a, vec_int32_half b) {
        return _mm256_add_epi32(a, b);
    }
#endif

#ifdef USE_AVX2
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

    inline vec_int8 load_epi8(uint8_t* ptr) {
        return _mm256_loadu_si256((const __m256i*)ptr);
    }

    inline vec_int16 load_epi16(int16_t* ptr) {
        return _mm256_loadu_si256((const __m256i*)ptr);
    }

    inline vec_int16 load_epi16(uint16_t* ptr) {
        return _mm256_loadu_si256((const __m256i*)ptr);
    }

    inline vec_int32 load_epi32(int32_t* ptr) {
        return _mm256_loadu_si256((const __m256i*)ptr);
    }

    inline void store_epi8(int8_t* ptr, vec_int8 v) {
        _mm256_storeu_si256((__m256i*)ptr, v);
    }

    inline void store_epi8(uint8_t* ptr, vec_int8 v) {
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

    inline vec_int8 max_epi8(vec_int8 v1, vec_int8 v2) {
        return _mm256_max_epi8(v1, v2);
    }

    inline vec_int8 min_epi8(vec_int8 v1, vec_int8 v2) {
        return _mm256_min_epi8(v1, v2);
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

    inline vec_int16 mullo_epi16(vec_int16 v1, vec_int16 v2) {
        return _mm256_mullo_epi16(v1, v2);
    }

    inline vec_int16 mulhi_epi16(vec_int16 v1, vec_int16 v2) {
        return _mm256_mulhi_epi16(v1, v2);
    }

    inline vec_int16 maddubs_epi16(vec_int8 v1, vec_int8 v2) {
        return _mm256_maddubs_epi16(v1, v2);
    }

    inline vec_int32 slli_epi16(vec_int32 v, int i) {
        return _mm256_slli_epi16(v, i);
    }

    inline vec_int32 srai_epi32(vec_int32 v, int i) {
        return _mm256_srai_epi32(v, i);
    }

    #ifdef HAS_VNNI256
        inline vec_int32 dpbusd_epi32(vec_int32 sum, vec_int8 v1, vec_int8 v2) {
            return _mm256_dpbusd_epi32(sum, v1, v2);
        }
    #else
        inline vec_int32 dpbusd_epi32(vec_int32 sum, vec_int8 v1, vec_int8 v2) {
            const vec_int16 prod = maddubs_epi16(v1, v2);
            return add_epi32(sum, madd_epi16(prod, set1_epi16(1)));
        }
    #endif

    // defined when USE_AVX2 only:

    template<int mask>
    inline vec_int32 permute4x64_epi64(vec_int32 v) {
        return _mm256_permute4x64_epi64(v, mask);
    }

    template<int mask>
    inline vec_int32 permute2x128_si256(vec_int32 v1, vec_int32 v2) {
        return _mm256_permute2x128_si256(v1, v2, mask);
    }

    inline vec_int32 hadd_epi32(vec_int32 v1, vec_int32 v2) {
        return _mm256_hadd_epi32(v1, v2);
    }

#endif