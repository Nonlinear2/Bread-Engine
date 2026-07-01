#include "nnue.hpp"

using namespace NNUE_UTILS;

#define STR2(x) #x
#define STR(x) STR2(x)

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
    // result is (in*255) * (in*255) * (w*64) 

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

#ifdef USE_AVX512
    int32_t run_L2(int16_t* input, int bucket){
        const vec_int16_half zero = setzero_epi16_half();
        const vec_int16_half qscale = set1_epi16_half(255);
        vec_int16_half result = set1_epi32_half(0);

        assert(L2_INPUT_SIZE % INT16_PER_REG == 0);

        for (int i = 0; i < L2_INPUT_SIZE; i += INT16_PER_REG){
            vec_int16_half in = load_epi16_half(&input[i]);
            in = min_epi16_half(qscale, max_epi16_half(in, zero));

            vec_int16_half weight_chunk = load_epi16_half(&l2_weights[bucket * L2_WEIGHTS_SIZE + i]);

            // madd pairs to int32 to avoid overflows in int16, while applying screlu
            vec_int32_half prod = madd_epi16_half(in, mullo_epi16_half(in, weight_chunk));

            result = add_epi32_half(result, prod);
        }
        // result is (in*255) * (in*255) * (w*64) 

        return reduce1_epi32_half(result) / 255 + l2_bias[bucket];
    };
#endif
#ifdef USE_AVX2
    int32_t run_L2(int16_t* input, int bucket){
        const vec_int16 zero = setzero_epi16();
        const vec_int16 qscale = set1_epi16(255);
        vec_int32 result = set1_epi32(0);

        assert(L2_INPUT_SIZE % INT16_PER_REG == 0);

        for (int i = 0; i < L2_INPUT_SIZE; i += INT16_PER_REG){
            vec_int16 in = load_epi16(&input[i]);
            in = min_epi16(qscale, max_epi16(in, zero));

            vec_int16 weight_chunk = load_epi16(&l2_weights[bucket * L2_WEIGHTS_SIZE + i]);

            // madd pairs to int32 to avoid overflows in int16, while applying screlu
            vec_int32 prod = madd_epi16(in, mullo_epi16(in, weight_chunk));

            result = add_epi32(result, prod);
        }
        // result is (in*255) * (in*255) * (w*64) 

        return reduce1_epi32(result) / 255 + l2_bias[bucket];
    };
#endif

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

}; // namespace NNUE