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

INCBIN(l2_weights, bread_NNUE_MODEL_PATH "/layer_2/weights.bin");
INCBIN(l2_bias, bread_NNUE_MODEL_PATH "/layer_2/bias.bin");

extern "C" {
    extern const int16_t ft_weights_start[];
    extern const int16_t ft_bias_start[];

    extern const int8_t l1_weights_start[];
    extern const int32_t l1_bias_start[];

    extern const int16_t l2_weights_start[];
    extern const int32_t l2_bias_start[];
};

bool ModifiedFeatures::valid() const {
    return added_1 != -1 && removed_1 != -1;
}

/******************
NNUE implementation
******************/

namespace NNUE {

int16_t* ft_weights = nullptr;
int16_t* ft_bias    = nullptr;

int8_t* l1_weights = nullptr;
int32_t* l1_bias    = nullptr;

int16_t* l2_weights = nullptr;
int32_t* l2_bias    = nullptr;

alignas(32) uint8_t ft_clamped_output[L1_INPUT_SIZE];

alignas(32) int32_t l1_output[L1_OUTPUT_SIZE];
alignas(32) int16_t l1_clamped_output[L1_OUTPUT_SIZE];

void load_model(){
    // feature transformer
    for (int i = 0; i < L0_WEIGHTS_SIZE; i++)
        ft_weights[i] = ft_weights_start[i];

    for (int i = 0; i < L0_BIAS_SIZE; i++)
        ft_bias[i] = ft_bias_start[i];

    // layer 1

    // permute weights
    int idx = 0;
    for (int bucket = 0; bucket < NUM_OUTPUT_BUCKETS; bucket++)
        for (int row_block = 0; row_block < L1_OUTPUT_SIZE; row_block += INT32_PER_REG)
            for (int col_block = 0; col_block < L1_INPUT_SIZE; col_block += 4)
                for (int m = 0; m < INT32_PER_REG; m++)                 // row within block
                    for (int n = 0; n < 4; n++)                         // col within block
                        l1_weights[idx++] = l1_weights_start[
                            bucket * L1_WEIGHTS_SIZE
                            + (row_block + m) * L1_INPUT_SIZE
                            + (col_block + n)
                        ];

    for (int i = 0; i < BUCKETED_L1_BIAS_SIZE; i++)
        l1_bias[i] = l1_bias_start[i] >> 1;

    // layer 2
    for (int i = 0; i < BUCKETED_L2_WEIGHTS_SIZE; i++){
        l2_weights[i] = l2_weights_start[i];
    }
    for (int i = 0; i < BUCKETED_L2_BIAS_SIZE; i++){
        l2_bias[i] = l2_bias_start[i];
    }
};

void init(){
    ft_weights = static_cast<int16_t*>(
        operator new[](sizeof(int16_t)*L0_WEIGHTS_SIZE, std::align_val_t{32})
    );
    ft_bias = static_cast<int16_t*>(
        operator new[](sizeof(int16_t)*L0_BIAS_SIZE, std::align_val_t{32})
    );

    l1_weights = static_cast<int8_t*>(
        operator new[](sizeof(int8_t)*BUCKETED_L1_WEIGHTS_SIZE, std::align_val_t{32})
    );
    l1_bias = static_cast<int32_t*>(
        operator new[](sizeof(int32_t)*BUCKETED_L1_BIAS_SIZE, std::align_val_t{32})
    );

    l2_weights = static_cast<int16_t*>(
        operator new[](sizeof(int16_t)*BUCKETED_L2_WEIGHTS_SIZE, std::align_val_t{32})
    );
    l2_bias = static_cast<int32_t*>(
        operator new[](sizeof(int32_t)*BUCKETED_L2_BIAS_SIZE, std::align_val_t{32})
    );

    load_model();
};

void cleanup(){
    operator delete[](ft_weights, std::align_val_t(32));
    operator delete[](ft_bias, std::align_val_t(32));

    operator delete[](l1_weights, std::align_val_t(32));
    operator delete[](l1_bias, std::align_val_t(32));

    operator delete[](l2_weights, std::align_val_t(32));
    operator delete[](l2_bias, std::align_val_t(32));
};

void compute_accumulator(Accumulator& new_acc, const Features active_features){
    for (int i = 0; i < ACC_SIZE; i += INT16_PER_REG){
        auto r = load_epi16(&ft_bias[i]);

        for (const int &a: active_features)
            r = add_epi16(r, load_epi16(&ft_weights[a * ACC_SIZE + i]));

        store_epi16(&new_acc[i], r);
    }
};

void update_accumulator(Accumulator& prev_acc, Accumulator& new_acc, const ModifiedFeatures& m_features){
    assert(m_features.valid());
    constexpr int CHUNK_SIZE = NUM_AVX_REGISTERS * INT16_PER_REG;

    switch (m_features.type)
    {
    case ModifiedFeatures::NORMAL:
        for (int j = 0; j < ACC_SIZE; j += CHUNK_SIZE){
            auto* prev = &prev_acc[j];
            auto* out  = &new_acc[j];
            auto* w_add = &ft_weights[m_features.added_1 * ACC_SIZE + j];
            auto* w_rem = &ft_weights[m_features.removed_1 * ACC_SIZE + j];

            for (int i = 0; i < CHUNK_SIZE; i += INT16_PER_REG * 4){ // process 4 registers at once
                
                auto r1 = load_epi16(prev + i);
                auto r2 = load_epi16(prev + i + INT16_PER_REG);
                auto r3 = load_epi16(prev + i + INT16_PER_REG*2);
                auto r4 = load_epi16(prev + i + INT16_PER_REG*3);

                r1 = add_epi16(r1, load_epi16(w_add + i));
                r2 = add_epi16(r2, load_epi16(w_add + i + INT16_PER_REG));
                r3 = add_epi16(r3, load_epi16(w_add + i + INT16_PER_REG*2));
                r4 = add_epi16(r4, load_epi16(w_add + i + INT16_PER_REG*3));

                r1 = sub_epi16(r1, load_epi16(w_rem + i));
                r2 = sub_epi16(r2, load_epi16(w_rem + i + INT16_PER_REG));
                r3 = sub_epi16(r3, load_epi16(w_rem + i + INT16_PER_REG*2));
                r4 = sub_epi16(r4, load_epi16(w_rem + i + INT16_PER_REG*3));

                store_epi16(out + i, r1);
                store_epi16(out + i + INT16_PER_REG, r2);
                store_epi16(out + i + INT16_PER_REG*2, r3);
                store_epi16(out + i + INT16_PER_REG*3, r4);
            }
        }
        break;
    case ModifiedFeatures::CAPTURE:
        for (int j = 0; j < ACC_SIZE; j += CHUNK_SIZE){
            auto* prev = &prev_acc[j];
            auto* out  = &new_acc[j];

            auto* w_add = &ft_weights[m_features.added_1   * ACC_SIZE + j];
            auto* w_rem = &ft_weights[m_features.removed_1 * ACC_SIZE + j];
            auto* w_cap = &ft_weights[m_features.removed_2 * ACC_SIZE + j];

            for (int i = 0; i < CHUNK_SIZE; i += INT16_PER_REG){
                auto r = load_epi16(prev + i);

                r = add_epi16(r, load_epi16(w_add + i));
                r = sub_epi16(r, load_epi16(w_rem + i));
                r = sub_epi16(r, load_epi16(w_cap + i));

                store_epi16(out + i, r);
            }
        }
        break;
    case ModifiedFeatures::CASTLING:
        for (int j = 0; j < ACC_SIZE; j += CHUNK_SIZE){
            auto* prev = &prev_acc[j];
            auto* out  = &new_acc[j];

            auto* w_add  = &ft_weights[m_features.added_1   * ACC_SIZE + j];
            auto* w_add2 = &ft_weights[m_features.added_2 * ACC_SIZE + j];
            auto* w_rem  = &ft_weights[m_features.removed_1 * ACC_SIZE + j];
            auto* w_cap  = &ft_weights[m_features.removed_2 * ACC_SIZE + j];

            for (int i = 0; i < CHUNK_SIZE; i += INT16_PER_REG){

                auto r = load_epi16(prev + i);

                r = add_epi16(r, load_epi16(w_add  + i));
                r = add_epi16(r, load_epi16(w_add2 + i));
                r = sub_epi16(r, load_epi16(w_rem  + i));
                r = sub_epi16(r, load_epi16(w_cap  + i));

                store_epi16(out + i, r);
            }
        }
        break;
    }
}

void update_accumulator(Accumulator& prev_acc, Accumulator& new_acc,
        const Features& added_features,
        const Features& removed_features){

    vec_int16 registers[NUM_AVX_REGISTERS];
    constexpr int CHUNK_SIZE = NUM_AVX_REGISTERS * INT16_PER_REG;

    for (int j = 0; j < ACC_SIZE; j += CHUNK_SIZE){
        for (int i = 0; i < NUM_AVX_REGISTERS; i++){
            registers[i] = load_epi16(&prev_acc[j + i*INT16_PER_REG]); 
        }

        for (const int &a: added_features){
            for (int i = 0; i < NUM_AVX_REGISTERS; i++){
                // a*acc size is the index of the a-th row. We then accumulate the weights.
                registers[i] = add_epi16(
                    registers[i],
                    load_epi16(&ft_weights[a*ACC_SIZE + j + i*INT16_PER_REG])
                );
            }
        }

        for (const int &r: removed_features){
            for (int i = 0; i < NUM_AVX_REGISTERS; i++){
                // r*acc size is the index of the r-th row. We then accumulate the weights.
                registers[i] = sub_epi16(
                    registers[i],
                    load_epi16(&ft_weights[r*ACC_SIZE + j + i*INT16_PER_REG])
                );
            }
        }

        // store the result in the accumulator
        for (int i = 0; i < NUM_AVX_REGISTERS; i++){
            store_epi16(&new_acc[j + i*INT16_PER_REG], registers[i]);
        }
    }
}

// in the avx2 case:
// non permuted weights (input_size=12, output_size=16):
//
// input size (12)
// --------------->
//   0   1   2   3    4   5   6   7    8   9  10  11 | output size (16)
//  12  13  14  15   16  17  18  19   20  21  22  23 V
//  24  25  26  27   28  29  30  31   32  33  34  35
//  36  37  38  39   40  41  42  43   44  45  46  47
//  48  49  50  51   52  53  54  55   56  57  58  59
//  60  61  62  63   64  65  66  67   68  69  70  71
//  72  73  74  75   76  77  78  79   80  81  82  83
//  84  85  86  87   88  89  90  91   92  93  94  95  <- end of row_block 0 (rows 0-7)

//  96  97  98  99  100 101 102 103  104 105 106 107
// 108 109 110 111  112 113 114 115  116 117 118 119
// 120 121 122 123  124 125 126 127  128 129 130 131
// 132 133 134 135  136 137 138 139  140 141 142 143
// 144 145 146 147  148 149 150 151  152 153 154 155
// 156 157 158 159  160 161 162 163  164 165 166 167
// 168 169 170 171  172 173 174 175  176 177 178 179
// 180 181 182 183  184 185 186 187  188 189 190 191  <- end of row_block 1 (rows 8-15)
//
//
// blocks are 8 rows x 4 cols
// 3 col_blocks (12/4), 2 row_blocks (16/8)
//
//
// blocks are flattened:
// [0]  0  1  2  3  12 13 14 15  24 25 26 27  36 37 38 39  48 49 50 51  60 61 62 63  72 73 74 75  84 85 86 87
// [1]  4  5  6  7  16 17 18 19  28 29 30 31  40 41 42 43  52 53 54 55  64 65 66 67  76 77 78 79  88 89 90 91
// ...
//
// and stored in this order:
// [0] [1] [2]
// [3] [4] [5] 
//
// out:
// acc 0:  [in[0..4] @ [0]] + [in[5..7] @ [1]] + [in[8..11] @ [2]]
// acc 1:  [in[0..4] @ [3]] + [in[5..7] @ [4]] + [in[8..11] @ [5]]

void run_L1(uint8_t* input, int32_t* output, int bucket){

    vec_int32 accs[L1_OUTPUT_SIZE / INT32_PER_REG] = {0};

    for (int i = 0; i < L1_INPUT_SIZE / 4; i++) { // horizontal block idx
        vec_int8 inputs = set1_epi32(*reinterpret_cast<int32_t*>(&input[i*4])); // set1 as epi32 to load 4 int8s at a time
        for (int j = 0; j < L1_OUTPUT_SIZE / INT32_PER_REG; j++) // vertical block idx
            accs[j] = dpbusd_epi32(
                accs[j],
                inputs,
                load_epi8(&l1_weights[
                    bucket * L1_WEIGHTS_SIZE 
                    + j * (L1_INPUT_SIZE / 4) * (INT32_PER_REG * 4)  // row stride
                    + i * (INT32_PER_REG * 4)                        // col stride
                ]
            )
        );
    }

    for (int k = 0; k < L1_OUTPUT_SIZE; k += INT32_PER_REG){
        vec_int32 out = srai_epi32(
            add_epi32(load_epi32(&l1_bias[bucket * L1_OUTPUT_SIZE + k]), accs[k / INT32_PER_REG]), 5
        );
        store_epi32(&output[k], out);
    }
};

// int32_t run_L2(int16_t* input, int bucket){
//     const vec_int16 zero = setzero_epi16();
//     const vec_int16 qscale = set1_epi16(255);
//     vec_int32 result = set1_epi32(0);

//     for (int i = 0; i < L2_INPUT_SIZE; i += INT16_PER_REG){
//         vec_int16 in = load_epi16(&input[i]);
//         in = min_epi16(qscale, max_epi16(in, zero));

//         vec_int16 weight_chunk = load_epi16(&l2_weights[bucket * L2_WEIGHTS_SIZE + i]);

//         // madd pairs to int32 to avoid overflows in int16, while applying screlu
//         vec_int32 prod = madd_epi16(in, mullo_epi16(in, weight_chunk));

//         result = add_epi32(result, prod);
//     }
//     // result is (in*255) * (in*255) * (w*64) 

//     return reduce1_epi32(result) / 255 + l2_bias[bucket];
// };

int32_t run_L2(int16_t* clamped_input, int32_t* input, int bucket){
    int32_t result = 0;

    for (int i = 0; i < L1_OUTPUT_SIZE; i++){
        int16_t c_in = std::clamp(clamped_input[i], (int16_t)0, (int16_t)255);
        int32_t in = input[i];

        result += c_in * l2_weights[bucket * L2_WEIGHTS_SIZE + i];
        result += std::clamp(in * in, 0, 255*255) * l2_weights[bucket * L2_WEIGHTS_SIZE + L1_OUTPUT_SIZE + i] / 255;
    }

    return result + l2_bias[bucket];
};

int run(Accumulators& accumulators, Color stm, int piece_count){
    constexpr int pieces_per_bucket = 32 / NUM_OUTPUT_BUCKETS;
    int bucket = (piece_count - 2) / pieces_per_bucket;

    assert(bucket >= 0 && bucket < NUM_OUTPUT_BUCKETS);

    pairwise_screlu16_to_8(
        &accumulators[stm][0],
        &accumulators[stm][ACC_SIZE / 2],
        ft_clamped_output, ACC_SIZE / 2
    );

    pairwise_screlu16_to_8(
        &accumulators[!stm][0],
        &accumulators[!stm][ACC_SIZE / 2],
        &ft_clamped_output[ACC_SIZE / 2], ACC_SIZE / 2
    );

    run_L1(ft_clamped_output, l1_output, bucket);

    crelu32_to_16(l1_output, l1_clamped_output, L1_OUTPUT_SIZE);

    int output = run_L2(l1_clamped_output, l1_output, bucket);

    return (output * 600) / (64 * 255); // scale is 600
};

//  __m256i avx_regs[num_avx_registers];

//     // load the accumulator
//     for (int i = 0; i < num_avx_registers; i++){
//         avx_regs[i] = _mm256_loadu_si256((const __m256i*)&accumulator[color][i*int16_per_reg]); // load int16
//     }

//     // added feature
//     for (int i = 0; i < num_avx_registers; i++){
//         // m_features.added*acc_size is the index of the added featured row. We then accumulate the weights.
//         avx_regs[i] = _mm256_add_epi16(
//             avx_regs[i],
//             _mm256_loadu_si256((const __m256i*)&ft_weights[m_features.added*acc_size + i*int16_per_reg]) // load int16
//             );
//     }
//     // removed feature
//     for (int i = 0; i < num_avx_registers; i++){
//         // m_features.removed*acc_size is to get the right column.
//         avx_regs[i] = _mm256_sub_epi16(
//             avx_regs[i],
//             _mm256_loadu_si256((const __m256i*)&ft_weights[m_features.removed*acc_size + i*int16_per_reg]) // load int16
//             );
//     }

//     if (m_features.captured != -1){
//         for (int i = 0; i < num_avx_registers; i++){
//             avx_regs[i] = _mm256_sub_epi16(
//                 avx_regs[i],
//                 _mm256_loadu_si256((const __m256i*)&ft_weights[m_features.captured*acc_size + i*int16_per_reg]) // load int16
//                 );
//         }
//     }

//     //store the result in the accumulator
//     for (int i = 0; i < num_avx_registers; i++){
//         _mm256_storeu_si256((__m256i*)&accumulator[color][i*int16_per_reg], avx_regs[i]); // store int16
//     }
// };


// // weight section:
// // [  ] ...
// // [  ] ...
// // [  ] ...
// // [  ] ...
// // ...
// // flattened: [  ][  ][  ][  ]...
// // height = out_size, width = 4

// // input:      1234|1234|1234|1234|1234|1234|1234|1234
// // weights:    [  ]|[  ]|[  ]|[  ]|[  ]|[  ]|[  ]|[  ]
// // maddubs:    * * |* * |* * |* * |* * |* * |* * |* *
// // madd:       x   |x   |x   |x   |x   |x   |x   |x   

// // -> accumulate for nnz chunks, and get output.

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

//     for (int i = 0; i < num_input_chunks; i++){
//         __m256i input_chunk = _mm256_loadu_si256((const __m256i*)&input[i*int8_per_reg]);
//         uint8_t z_bitmask = _mm256_movemask_ps(
//             (__m256)_mm256_cmpeq_epi32(input_chunk, _mm256_setzero_si256())
//         );

//         uint8_t nnz_bitmask = ~z_bitmask;
//         int idx;
//         while (nnz_bitmask){
//             idx = lsb(nnz_bitmask);
//             nnz_bitmask &= nnz_bitmask - 1;
//             nnz_indices[num_nnz_inputs++] = i*int8_per_reg + idx*4;
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


// // // sparse matrix multiplication
// // void NNUE::run_sparse(int8_t* input, int32_t* output, int input_size, int output_size, int8_t* weights, int32_t* bias){
// //     const int num_input_chunks = input_size/int8_per_reg;
// //     const int num_output_chunks = output_size/int32_per_reg;

// //     // 4 int8s at a time, as an int32.
// //     const int MAX_NNZ_INPUTS = input_size / 4;
// //     int nnz_indices[MAX_NNZ_INPUTS];
// //     int num_nnz_inputs = 0;

// //     __m256i output_chunks[num_output_chunks];
// //     const __m256i one = _mm256_set1_epi16(1);

// //     for (int i = 0; i < num_input_chunks / 8; i++){
// //         uint64_t nnz_bitmask = 0;
// //         for (int j = 0; j < 8; j++){
// //             __m256i input_chunk = _mm256_loadu_si256((const __m256i*)&input[(i*8 + j)*int8_per_reg]);
// //             uint8_t z_bitmask = _mm256_movemask_ps(
// //                 (__m256)_mm256_cmpeq_epi32(input_chunk, _mm256_setzero_si256())
// //             );
// //             nnz_bitmask |= ((uint64_t)(~z_bitmask & 0xFF)) << (j * 8);
// //         }

// //         int idx;
// //         while (nnz_bitmask){
// //             idx = lsb(nnz_bitmask);
// //             nnz_bitmask &= nnz_bitmask - 1;
// //             nnz_indices[num_nnz_inputs++] = i*8*int8_per_reg + idx*4;
// //         }
// //     }

// //     assert(num_nnz_inputs <= MAX_NNZ_INPUTS);
// //     // std::cout << num_nnz_inputs << " ";

// //     // load the bias from memory
// //     for (int i = 0; i < num_output_chunks; i++){
// //         output_chunks[i] = _mm256_loadu_si256((const __m256i*)&bias[i*int32_per_reg]);
// //     }

// //     for (int i = 0; i < num_nnz_inputs; i++){
// //         // load the nonzero input group
// //         __m256i input_group = _mm256_set1_epi32(*reinterpret_cast<const uint32_t*>(&input[nnz_indices[i]]));
// //         for (int j = 0; j < num_output_chunks; j++){
// //             __m256i mixed_input = _mm256_maddubs_epi16(
// //                 input_group,
// //                 _mm256_loadu_si256((const __m256i*)&weights[(nnz_indices[i]*output_size) + j*int8_per_reg])
// //             );
// //             output_chunks[j] = _mm256_add_epi32(output_chunks[j], _mm256_madd_epi16(mixed_input, one)); // hadd pairs to int32
// //         }
// //     }

// //     for (int i = 0; i < num_output_chunks; i++){
// //         // this integer divides the result by 64 which is the scale.
// //         output_chunks[i] = _mm256_srai_epi32(output_chunks[i], 6);
// //         _mm256_storeu_si256((__m256i*)&output[i*int32_per_reg], output_chunks[i]); // store int32
// //     }
// // };

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

// int NNUE::run_cropped_nn(bool color){
//     crelu16(accumulator[color], &ft_clipped_output[0], acc_size);
//     crelu16(accumulator[!color], &ft_clipped_output[acc_size], acc_size);

//     run_sparse(ft_clipped_output, l2_unclipped_output, l2_input_size, l2_output_size, l2_weights, l2_bias);
//     crelu32(l2_unclipped_output, l2_clipped_output, l2_output_size);

//     run_dense(l2_clipped_output, l3_unclipped_output, l3_input_size, l3_output_size, l3_weights, l3_bias);
//     crelu32(l3_unclipped_output, l3_clipped_output, l3_output_size);

//     int16_t output = run_output_layer(l3_clipped_output, l4_weights, l4_bias);
//     return output / 16;

// };
}; // namespace NNUE
