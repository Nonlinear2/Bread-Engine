#include "nnue.hpp"

#define STR2(x) #x
#define STR(x) STR2(x)

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

    extern const int8_t l1_weights_start[];
    extern const int32_t l1_bias_start[];
};

/****************
Accumulator class
****************/
int16_t* Accumulator::operator[](bool color){
    return color ? &accumulator[0] : &accumulator[ACC_SIZE];
};
/*********
NNUE class
*********/

NNUE::NNUE(){
    ft_weights = static_cast<int16_t*>(
        operator new[](sizeof(int16_t)*INPUT_SIZE*ACC_SIZE, std::align_val_t{32})
    );
    ft_bias = static_cast<int16_t*>(
        operator new[](sizeof(int16_t)*ACC_SIZE, std::align_val_t{32})
    );

    l1_weights = static_cast<int8_t*>(
        operator new[](sizeof(int8_t)*l1_input_size*l1_output_size, std::align_val_t{32})
    );
    l1_bias = static_cast<int32_t*>(
        operator new[](sizeof(int32_t)*l1_output_size, std::align_val_t{32})
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
    for (int i = 0; i < INPUT_SIZE*ACC_SIZE; i++){
        ft_weights[i] = ft_weights_start[i];
    }
    for (int i = 0; i < ACC_SIZE; i++){
        ft_bias[i] = ft_bias_start[i];
    }

    // layer 1
    for (int i = 0; i < l1_input_size*l1_output_size; i++){
        l1_weights[i] = l1_weights_start[i];
    }
    for (int i = 0; i < l1_output_size; i++){
        l1_bias[i] = l1_bias_start[i];
    }
};

void NNUE::compute_accumulator(const std::vector<int> active_features, bool color){
    // we have 256 int16 to process, and there are 16 avx registers which can hold 16 int16.
    // therefore we need only one pass to the registers.

    __m256i avx_regs[num_avx_registers];

    // load the bias from memory
    for (int i = 0; i < num_avx_registers; i++){
        avx_regs[i] = _mm256_loadu_si256((const __m256i*)&ft_bias[i*int16_per_reg]); // load int16
    }

    for (const int &a: active_features){
        for (int i = 0; i < num_avx_registers; i++){
            // a*acc size is the index of the a-th row. We then accumulate the weights.
            avx_regs[i] = _mm256_add_epi16(
                avx_regs[i],
                _mm256_loadu_si256((const __m256i*)&ft_weights[a*ACC_SIZE + i*int16_per_reg]) // load int16
                );
        }
    }
    // store the result in the accumulator
    for (int i = 0; i < num_avx_registers; i++){
        _mm256_storeu_si256((__m256i*)&accumulator[color][i*int16_per_reg], avx_regs[i]); // store int16
    }
};

void NNUE::update_accumulator(const modified_features m_features, bool color){

    __m256i avx_regs[num_avx_registers];

    // load the accumulator
    for (int i = 0; i < num_avx_registers; i++){
        avx_regs[i] = _mm256_loadu_si256((const __m256i*)&accumulator[color][i*int16_per_reg]); // load int16
    }

    // added feature
    for (int i = 0; i < num_avx_registers; i++){
        // m_features.added*acc_size is the index of the added featured row. We then accumulate the weights.
        avx_regs[i] = _mm256_add_epi16(
            avx_regs[i],
            _mm256_loadu_si256((const __m256i*)&ft_weights[m_features.added*ACC_SIZE + i*int16_per_reg]) // load int16
            );
    }
    // removed feature
    for (int i = 0; i < num_avx_registers; i++){
        // m_features.removed*acc_size is to get the right column.
        avx_regs[i] = _mm256_sub_epi16(
            avx_regs[i],
            _mm256_loadu_si256((const __m256i*)&ft_weights[m_features.removed*ACC_SIZE + i*int16_per_reg]) // load int16
            );
    }

    if (m_features.captured != -1){
        for (int i = 0; i < num_avx_registers; i++){
            avx_regs[i] = _mm256_sub_epi16(
                avx_regs[i],
                _mm256_loadu_si256((const __m256i*)&ft_weights[m_features.captured*ACC_SIZE + i*int16_per_reg]) // load int16
                );
        }
    }

    //store the result in the accumulator
    for (int i = 0; i < num_avx_registers; i++){
        _mm256_storeu_si256((__m256i*)&accumulator[color][i*int16_per_reg], avx_regs[i]); // store int16
    }
};


int32_t NNUE::run_output_layer(int8_t* input, int8_t* weights, int32_t* bias){
    constexpr int input_size = 2*ACC_SIZE;
    constexpr int num_input_chunks = input_size/int8_per_reg;

    const __m256i one = _mm256_set1_epi16(1);

    __m256i result = _mm256_set1_epi32(0);
    for (int i = 0; i < num_input_chunks; i++){
        __m256i input_chunk = _mm256_loadu_si256((const __m256i*)&input[i*int8_per_reg]); // load int8
        __m256i prod = _mm256_maddubs_epi16(
            input_chunk,
            _mm256_loadu_si256((const __m256i*)&weights[i*int8_per_reg]) //load int8
        ); // outputs int16
        prod = _mm256_madd_epi16(prod, one); // hadd pairs to int32 to avoid overflows in int16
        result = _mm256_add_epi32(result, prod);
    }

    // accumulate together
    result = _mm256_hadd_epi32(result, result);

    int32_t out_ptr[8];
    _mm256_storeu_si256((__m256i*)out_ptr, result);

    return out_ptr[0] + out_ptr[1] + out_ptr[4] + out_ptr[5] + bias[0];
};

// input size must be a multiple of 32
// there is no max size.
void NNUE::crelu16(int16_t *input, int8_t *output, int size){

    assert(size % int8_per_reg == 0);

    const int num_regs = size / int8_per_reg;
    const __m256i zero = _mm256_setzero_si256();

    for (int i = 0; i < num_regs; i++){
        __m256i in_1 = _mm256_loadu_si256((const __m256i*)&input[(2*i)*int16_per_reg]); // load int16
        __m256i in_2 = _mm256_loadu_si256((const __m256i*)&input[(2*i+1)*int16_per_reg]); // load int16
        // packs sets negative values to 0 and saturates at 255, which effectively applies relu
        __m256i out = _mm256_packus_epi16(in_1, in_2);
        out = _mm256_permute4x64_epi64(out, 0b11011000); // undo the packus shuffle
        _mm256_storeu_si256((__m256i*)&output[i*int8_per_reg], out); // store int8
    }
};

int NNUE::run_cropped_nn(bool color){
    crelu16(accumulator[color], &ft_clipped_output[0], ACC_SIZE);
    crelu16(accumulator[!color], &ft_clipped_output[ACC_SIZE], ACC_SIZE);

    int output = run_output_layer(ft_clipped_output, l1_weights, l1_bias);
    return output / 32; // 64 * 255 * true_output / 32 = 510 * true_output so roughly scale which is 600
};
