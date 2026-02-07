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

extern "C" {
    extern const int16_t ft_weights_start[];
    extern const int16_t ft_bias_start[];

    extern const int16_t l1_weights_start[];
    extern const int32_t l1_bias_start[];
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

void compute_accumulator(Accumulator& new_acc, const Features active_features){
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


void update_accumulator(Accumulator& prev_acc, Accumulator& new_acc, const ModifiedFeatures& m_features){
    assert(m_features.valid());
    constexpr int CHUNK_SIZE = NUM_AVX_REGISTERS * INT16_PER_REG;

    switch (m_features.type)
    {
    case ModifiedFeatures::NORMAL:
        for (int j = 0; j < ACC_SIZE; j += CHUNK_SIZE){
            auto* prev = &prev_acc[j];
            auto* out  = &new_acc[j];

            auto* w_add = &ft_weights[m_features.added_1   * ACC_SIZE + j];
            auto* w_rem = &ft_weights[m_features.removed_1 * ACC_SIZE + j];

            for (int i = 0; i < CHUNK_SIZE; i += INT16_PER_REG){

                auto r = load_epi16(prev + i);

                r = add_epi16(r, load_epi16(w_add + i));
                r = sub_epi16(r, load_epi16(w_rem + i));

                store_epi16(out + i, r);
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