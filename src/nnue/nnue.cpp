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
            ".balign 16\n" \
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

/*********
NNUE class
*********/

NNUE::NNUE(){
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

NNUE::~NNUE(){
    operator delete[](ft_weights, std::align_val_t(32));
    operator delete[](ft_bias, std::align_val_t(32));

    operator delete[](l1_weights, std::align_val_t(32));
    operator delete[](l1_bias, std::align_val_t(32));
};

void NNUE::load_model(){
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

void NNUE::compute_accumulator(Accumulator& new_acc, const std::vector<int> active_features){
    // we have 256 int16 to process, and there are 16 avx registers which can hold 16 int16.
    // therefore we need only one pass to the registers.

    constexpr int num_chunks = ACC_SIZE / INT16_PER_REG;

    vec_int16 registers[num_chunks];

    // load the bias from memory
    for (int i = 0; i < num_chunks; i++){
        registers[i] = load_epi16(&ft_bias[i*INT16_PER_REG]);
    }

    for (const int &a: active_features){
        for (int i = 0; i < num_chunks; i++){
            // a*acc size is the index of the a-th row. We then accumulate the weights.
            registers[i] = add_epi16(
                registers[i],
                load_epi16(&ft_weights[a*ACC_SIZE + i*INT16_PER_REG])
                );
        }
    }

    // store the result in the accumulator
    for (int i = 0; i < num_chunks; i++){
        store_epi16(&new_acc[i*INT16_PER_REG], registers[i]);
    }
};

void NNUE::update_accumulator(Accumulator& prev_acc, Accumulator& new_acc, const modified_features m_features){

    constexpr int num_chunks = ACC_SIZE / INT16_PER_REG;

    vec_int16 registers[num_chunks];

    // load the accumulator
    for (int i = 0; i < num_chunks; i++){
        registers[i] = load_epi16(&prev_acc[i*INT16_PER_REG]); 
    }

    // added feature
    for (int i = 0; i < num_chunks; i++){
        // m_features.added*acc_size is the index of the added featured row. We then accumulate the weights.
        registers[i] = add_epi16(
            registers[i],
            load_epi16(&ft_weights[m_features.added*ACC_SIZE + i*INT16_PER_REG])
            );
    }

    // removed feature
    for (int i = 0; i < num_chunks; i++){
        // m_features.removed*acc_size is to get the right column.
        registers[i] = sub_epi16(
            registers[i],
            load_epi16(&ft_weights[m_features.removed*ACC_SIZE + i*INT16_PER_REG])
            );
    }

    if (m_features.captured != -1){
        for (int i = 0; i < num_chunks; i++){
            registers[i] = sub_epi16(
                registers[i],
                load_epi16(&ft_weights[m_features.captured*ACC_SIZE + i*INT16_PER_REG])
                );
        }
    }

    //store the result in the accumulator
    for (int i = 0; i < num_chunks; i++){
        store_epi16(&new_acc[i*INT16_PER_REG], registers[i]);
    }
};


int32_t NNUE::run_output_layer(int16_t* input, int16_t* weights, int32_t* bias, int bucket){
    constexpr int input_size = 2*ACC_SIZE;

    const vec_int16 one = set1_epi16(1);

    vec_int32 result = set1_epi32(0);
    for (int i = 0; i < input_size; i += INT16_PER_REG){
        vec_int16 input_chunk = load_epi16(&input[i]);
        vec_int16 weight_chunk = load_epi16(&weights[bucket * L1_WEIGHTS_SIZE + i]);
        vec_int32 prod = madd_epi16(input_chunk, mullo_epi16(input_chunk, weight_chunk));

        // madd pairs to int32 to avoid overflows in int16
        result = add_epi32(result, prod);
    }

    return reduce1_epi32(result) / 255 + bias[bucket];
};

int NNUE::run(Accumulators& accumulators, Color stm, int piece_count){
    constexpr int pieces_per_bucket = 32 / OUTPUT_BUCKET_COUNT;
    int bucket = (piece_count - 2) / pieces_per_bucket;

    crelu16_to_16(accumulators[stm].data(), &ft_clipped_output[0], ACC_SIZE);
    crelu16_to_16(accumulators[!stm].data(), &ft_clipped_output[ACC_SIZE], ACC_SIZE);

    int output = run_output_layer(ft_clipped_output, l1_weights, l1_bias, bucket);
    return (output * 600) / (64 * 255); // scale is 600
};
