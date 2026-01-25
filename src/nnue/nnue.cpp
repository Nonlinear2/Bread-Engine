#include "nnue.hpp"

using namespace NNUE_UTILS;

#define STR2(x) #x
#define STR(x) STR2(x)

#if !defined(_MSC_VER)
    constexpr
#endif
    int
    lsb(uint32_t bits) {
    assert(bits != 0);
#if __cplusplus >= 202002L
    return std::countr_zero(bits);
#else
#if defined(__GNUC__)
    return __builtin_ctzll(bits);
#elif defined(_MSC_VER)
    unsigned long idx;
    _BitScanForward64(&idx, bits);
    return static_cast<int>(idx);
#else
#error "Compiler not supported."
#endif
#endif
}

#ifdef __APPLE__
#define USTR(x) "_" STR(x)
#else
#define USTR(x) STR(x)
#endif

#ifdef _WIN32
    #define INCBIN_SECTION ".rdata, \"dr\""
#elif defined __APPLE__
    #define INCBIN_SECTION "__TEXT,__const"
#else
    #define INCBIN_SECTION ".rodata"
#endif

// credit: https://gist.github.com/mmozeiko/ed9655cf50341553d282
#define INCBIN(name, file) \
    __asm__(".section " INCBIN_SECTION "\n" \
            ".global " STR(name) "_start\n" \
            ".balign 32\n" \
            STR(name) "_start:\n" \
            ".incbin \"" file "\"\n" \
            \
            ".global " STR(name) "_end\n" \
            ".balign 1\n" \
            STR(name) "_end:\n" \
            ".byte 0\n" \
    ); \

INCBIN(ft_weights, bread_NNUE_MODEL_PATH "/feature_transformer/weights.bin");
INCBIN(ft_bias, bread_NNUE_MODEL_PATH "/feature_transformer/bias.bin");

INCBIN(l1_weights, bread_NNUE_MODEL_PATH "/layer_1/weights.bin");
INCBIN(l1_bias, bread_NNUE_MODEL_PATH "/layer_1/bias.bin");

extern "C" {
    extern const int16_t ft_weights_start[];
    extern const int16_t ft_bias_start[];

    extern const int16_t l1_weights_start[];
    extern const int32_t l1_bias_start[];
};

bool ModifiedFeatures::valid() const {
    return added != -1;
}

/******************
NNUE implementation
******************/

namespace NNUE {

int16_t* ft_weights = nullptr;
int16_t* ft_bias    = nullptr;

int16_t* l1_weights = nullptr;
int32_t* l1_bias    = nullptr;

void load_model(){
    // feature transformer
    for (int i = 0; i < L0_WEIGHTS_SIZE; i++)
        ft_weights[i] = ft_weights_start[i];

    for (int i = 0; i < L0_BIAS_SIZE; i++)
        ft_bias[i] = ft_bias_start[i];

    // layer 1
    for (int i = 0; i < BUCKETED_L1_WEIGHTS_SIZE; i++)
        l1_weights[i] = l1_weights_start[i];

    for (int i = 0; i < BUCKETED_L1_BIAS_SIZE; i++)
        l1_bias[i] = l1_bias_start[i];
};

void init(){
    ft_weights = static_cast<int16_t*>(
        operator new[](sizeof(int16_t)*L0_WEIGHTS_SIZE, std::align_val_t{32})
    );
    ft_bias = static_cast<int16_t*>(
        operator new[](sizeof(int16_t)*L0_BIAS_SIZE, std::align_val_t{32})
    );

    l1_weights = static_cast<int16_t*>(
        operator new[](sizeof(int16_t)*BUCKETED_L1_WEIGHTS_SIZE, std::align_val_t{32})
    );
    l1_bias = static_cast<int32_t*>(
        operator new[](sizeof(int32_t)*BUCKETED_L1_BIAS_SIZE, std::align_val_t{32})
    );

    load_model();
};

void cleanup(){
    operator delete[](ft_weights, std::align_val_t(32));
    operator delete[](ft_bias, std::align_val_t(32));

    operator delete[](l1_weights, std::align_val_t(32));
    operator delete[](l1_bias, std::align_val_t(32));
};

void compute_accumulator(Accumulator& new_acc, const std::vector<int> active_features){
    vec_int16 registers[NUM_AVX_REGISTERS];

    constexpr int CHUNK_SIZE = NUM_AVX_REGISTERS*INT16_PER_REG;

    for (int j = 0; j < ACC_SIZE; j += CHUNK_SIZE){
        // load the bias from memory
        for (int i = 0; i < NUM_AVX_REGISTERS; i++){
            registers[i] = load_epi16(&ft_bias[j + i*INT16_PER_REG]);
        }

        for (const int &a: active_features){
            for (int i = 0; i < NUM_AVX_REGISTERS; i++){
                // a*acc size is the index of the a-th row. We then accumulate the weights.
                registers[i] = add_epi16(
                    registers[i],
                    load_epi16(&ft_weights[a*ACC_SIZE + j + i*INT16_PER_REG])
                    );
            }
        }

        // store the result in the accumulator
        for (int i = 0; i < NUM_AVX_REGISTERS; i++){
            store_epi16(&new_acc[j + i*INT16_PER_REG], registers[i]);
        }
    }
};

void update_accumulator(Accumulator& prev_acc, Accumulator& new_acc, const ModifiedFeatures m_features){
    assert(m_features.valid());

    vec_int16 registers[NUM_AVX_REGISTERS];
    constexpr int CHUNK_SIZE = NUM_AVX_REGISTERS * INT16_PER_REG;

    if (m_features.captured != -1){
        for (int j = 0; j < ACC_SIZE; j += CHUNK_SIZE){
            for (int i = 0; i < NUM_AVX_REGISTERS; i++){
                registers[i] = load_epi16(&prev_acc[j + i*INT16_PER_REG]); 
            }
            for (int i = 0; i < NUM_AVX_REGISTERS; i++){
                registers[i] = add_epi16(
                    registers[i],
                    load_epi16(&ft_weights[m_features.added*ACC_SIZE + j + i*INT16_PER_REG])
                );
            }
            for (int i = 0; i < NUM_AVX_REGISTERS; i++){
                registers[i] = sub_epi16(
                    registers[i],
                    load_epi16(&ft_weights[m_features.removed*ACC_SIZE + j + i*INT16_PER_REG])
                );
            }
            for (int i = 0; i < NUM_AVX_REGISTERS; i++){
                registers[i] = sub_epi16(
                    registers[i],
                    load_epi16(&ft_weights[m_features.captured*ACC_SIZE + j + i*INT16_PER_REG])
                );
            }
            for (int i = 0; i < NUM_AVX_REGISTERS; i++){
                store_epi16(&new_acc[j + i*INT16_PER_REG], registers[i]);
            }
        }
    } else {
        for (int j = 0; j < ACC_SIZE; j += CHUNK_SIZE){
            for (int i = 0; i < NUM_AVX_REGISTERS; i++){
                registers[i] = load_epi16(&prev_acc[j + i*INT16_PER_REG]); 
            }
            for (int i = 0; i < NUM_AVX_REGISTERS; i++){
                registers[i] = add_epi16(
                    registers[i],
                    load_epi16(&ft_weights[m_features.added*ACC_SIZE + j + i*INT16_PER_REG])
                );
            }
            for (int i = 0; i < NUM_AVX_REGISTERS; i++){
                registers[i] = sub_epi16(
                    registers[i],
                    load_epi16(&ft_weights[m_features.removed*ACC_SIZE + j + i*INT16_PER_REG])
                );
            }
            for (int i = 0; i < NUM_AVX_REGISTERS; i++){
                store_epi16(&new_acc[j + i*INT16_PER_REG], registers[i]);
            }
        }
    }
}

// weight section:
// [  ] ...
// [  ] ...
// [  ] ...
// [  ] ...
// ...
// flattened: [  ][  ][  ][  ]...
// height = out_size, width = 4

// input:      1234|1234|1234|1234|1234|1234|1234|1234
// weights:    [  ]|[  ]|[  ]|[  ]|[  ]|[  ]|[  ]|[  ]
// maddubs:    * * |* * |* * |* * |* * |* * |* * |* *
// madd:       x   |x   |x   |x   |x   |x   |x   |x   

// -> accumulate for nnz chunks, and get output.

// sparse matrix multiplication
void NNUE::run_L2_sparse(int8_t* input, int32_t* output){

    int16_t* stm_data = accumulators[stm].data();
    int16_t* nstm_data = accumulators[!stm].data();

    // 4 int8s at a time, as an int32.
    const int MAX_NNZ_INPUTS = ACC_SIZE / 4;
    int nnz_indices[MAX_NNZ_INPUTS];
    int num_nnz_inputs = 0;

    const vec_int16 one = set1_epi16(1);

    const vec_int16 zero = setzero_epi16();
    const vec_int16 qscale = set1_epi16(255);
    vec_int32 result = set1_epi32(0);

    for (int i = 0; i < ACC_SIZE; i += INT16_PER_REG){
        vec_int16 in = load_epi16(&stm_data[i]);

        uint8_t z_bitmask = _mm256_movemask_ps(
            (__m256)_mm256_cmpeq_epi32(in, _mm256_setzero_si256())
        );

        uint8_t nnz_bitmask = ~z_bitmask;
        int idx;
        while (nnz_bitmask){
            idx = lsb(nnz_bitmask);
            nnz_bitmask &= nnz_bitmask - 1;
            nnz_indices[num_nnz_inputs++] = i*INT8_PER_REG + idx*4;
        }
    }

    assert(num_nnz_inputs <= MAX_NNZ_INPUTS);
    // std::cout << num_nnz_inputs << " ";

    vec_int16 registers[NUM_AVX_REGISTERS];
    constexpr int CHUNK_SIZE = NUM_AVX_REGISTERS * INT16_PER_REG;

    // load the bias from memory
    for (int i = 0; i < L1_OUTPUT_SIZE; i += INT32_PER_REG){
        registers[i] = load_epi16(&l1_bias[i*INT32_PER_REG]);
    }

    for (int i = 0; i < num_nnz_inputs; i++){
        // load the nonzero input group
        __m256i input_group = set1_epi32(*reinterpret_cast<const uint32_t*>(&input[nnz_indices[i]]));
        for (int j = 0; j < L1_OUTPUT_SIZE; j += INT32_PER_REG){
            __m256i mixed_input = _mm256_maddubs_epi16(
                input_group,
                load_epi16(&l1_weights[(nnz_indices[i]*L1_OUTPUT_SIZE) + j*INT8_PER_REG])
            );
            registers[j] = add_epi32(registers[j], madd_epi16(mixed_input, one)); // hadd pairs to int32
        }
    }

    for (int i = 0; i < L1_OUTPUT_SIZE; i += INT32_PER_REG){
        // this integer divides the result by 64 which is the scale.
        registers[i] = _mm256_srai_epi32(registers[i], 6);
        store_epi32(&output[i*INT32_PER_REG], registers[i]);
    }
};


// // sparse matrix multiplication
// void NNUE::run_sparse(int8_t* input, int32_t* output, int input_size, int output_size, int8_t* weights, int32_t* bias){
//     const int num_input_chunks = input_size/int8_per_reg;
//     const int num_output_chunks = output_size/int32_per_reg;

//     // 4 int8s at a time, as an int32.
//     const int MAX_NNZ_INPUTS = input_size / 4;
//     int nnz_indices[MAX_NNZ_INPUTS];
//     int num_nnz_inputs = 0;

//     __m256i output_chunks[num_output_chunks];
//     const __m256i one = _mm256_set1_epi16(1);

//     for (int i = 0; i < num_input_chunks / 8; i++){
//         uint64_t nnz_bitmask = 0;
//         for (int j = 0; j < 8; j++){
//             __m256i input_chunk = _mm256_loadu_si256((const __m256i*)&input[(i*8 + j)*int8_per_reg]);
//             uint8_t z_bitmask = _mm256_movemask_ps(
//                 (__m256)_mm256_cmpeq_epi32(input_chunk, _mm256_setzero_si256())
//             );
//             nnz_bitmask |= ((uint64_t)(~z_bitmask & 0xFF)) << (j * 8);
//         }

//         int idx;
//         while (nnz_bitmask){
//             idx = lsb(nnz_bitmask);
//             nnz_bitmask &= nnz_bitmask - 1;
//             nnz_indices[num_nnz_inputs++] = i*8*int8_per_reg + idx*4;
//         }
//     }

//     assert(num_nnz_inputs <= MAX_NNZ_INPUTS);
//     // std::cout << num_nnz_inputs << " ";

//     // load the bias from memory
//     for (int i = 0; i < num_output_chunks; i++){
//         output_chunks[i] = _mm256_loadu_si256((const __m256i*)&bias[i*int32_per_reg]);
//     }

//     for (int i = 0; i < num_nnz_inputs; i++){
//         // load the nonzero input group
//         __m256i input_group = _mm256_set1_epi32(*reinterpret_cast<const uint32_t*>(&input[nnz_indices[i]]));
//         for (int j = 0; j < num_output_chunks; j++){
//             __m256i mixed_input = _mm256_maddubs_epi16(
//                 input_group,
//                 _mm256_loadu_si256((const __m256i*)&weights[(nnz_indices[i]*output_size) + j*int8_per_reg])
//             );
//             output_chunks[j] = _mm256_add_epi32(output_chunks[j], _mm256_madd_epi16(mixed_input, one)); // hadd pairs to int32
//         }
//     }

//     for (int i = 0; i < num_output_chunks; i++){
//         // this integer divides the result by 64 which is the scale.
//         output_chunks[i] = _mm256_srai_epi32(output_chunks[i], 6);
//         _mm256_storeu_si256((__m256i*)&output[i*int32_per_reg], output_chunks[i]); // store int32
//     }
// };

// // dense matrix multiplication
// void NNUE::run_dense(int8_t* input, int32_t* output, int input_size, int output_size, int8_t* weights, int32_t* bias){
//     const int num_input_chunks = input_size/int8_per_reg;
//     const int num_output_chunks = output_size/int32_per_reg;

//     __m256i process_chunks[int32_per_reg];
//     const __m256i one = _mm256_set1_epi16(1);

//     for (int j = 0; j < num_output_chunks; j++){
//         __m256i result = _mm256_loadu_si256((const __m256i*)&bias[j*int32_per_reg]);
//         for (int i = 0; i < num_input_chunks; i++){
//             __m256i input_chunk = _mm256_loadu_si256((const __m256i*)&input[i*int8_per_reg]); // load int8
//             for (int k = 0; k < int32_per_reg; k++){
//                 process_chunks[k] = _mm256_maddubs_epi16(
//                     input_chunk,
//                     _mm256_loadu_si256((const __m256i*)&weights[(j*int32_per_reg+k) * input_size + i*int8_per_reg]) //load int8
//                 );
//                 process_chunks[k] = _mm256_madd_epi16(process_chunks[k], one); // hadd pairs to int32
//             }
//             result = _mm256_add_epi32(result, _mm256_add8x256_epi32(process_chunks));

int32_t run_L1(Accumulators& accumulators, Color stm, int bucket){
    int16_t* stm_data = accumulators[stm].data();
    int16_t* nstm_data = accumulators[!stm].data();

    const vec_int16 one = set1_epi16(1);
    const vec_int16 zero = setzero_epi16();
    const vec_int16 qscale = set1_epi16(255);
    vec_int32 result = set1_epi32(0);

    for (int i = 0; i < ACC_SIZE; i += INT16_PER_REG){
        vec_int16 in = load_epi16(&stm_data[i]);
        in = min_epi16(qscale, max_epi16(in, zero));

        vec_int16 weight_chunk = load_epi16(&l1_weights[bucket * L1_WEIGHTS_SIZE + i]);
        vec_int32 prod = madd_epi16(in, mullo_epi16(in, weight_chunk));

        // madd pairs to int32 to avoid overflows in int16
        result = add_epi32(result, prod);
    }

    for (int i = 0; i < ACC_SIZE; i += INT16_PER_REG){
        vec_int16 in = load_epi16(&nstm_data[i]);
        in = min_epi16(qscale, max_epi16(in, zero));

        vec_int16 weight_chunk = load_epi16(&l1_weights[bucket * L1_WEIGHTS_SIZE + ACC_SIZE + i]);
        vec_int32 prod = madd_epi16(in, mullo_epi16(in, weight_chunk));

        // madd pairs to int32 to avoid overflows in int16
        result = add_epi32(result, prod);
    }

    return reduce1_epi32(result) / 255 + l1_bias[bucket];
};

int run(Accumulators& accumulators, Color stm, int piece_count){
    constexpr int pieces_per_bucket = 32 / OUTPUT_BUCKET_COUNT;
    int bucket = (piece_count - 2) / pieces_per_bucket;
    assert(bucket >= 0 && bucket <= OUTPUT_BUCKET_COUNT);

    int output = run_L1(accumulators, stm, bucket);
    return (output * 600) / (64 * 255); // scale is 600
};

}; // namespace NNUE
