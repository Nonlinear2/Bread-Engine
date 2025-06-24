#include "nnue.hpp"

/*************
NNLayer
*************/

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

template<typename in_type, int in_size, typename out_type, int out_size>
NNLayer<in_type, in_size, out_type, out_size>::NNLayer(){
    weights = static_cast<in_type*>(operator new[](sizeof(in_type)*input_size*output_size, std::align_val_t{32}));
    bias = static_cast<out_type*>(operator new[](sizeof(out_type)*output_size, std::align_val_t{32}));
}

template<typename in_type, int in_size, typename out_type, int out_size>
NNLayer<in_type, in_size, out_type, out_size>::~NNLayer(){
    operator delete[](weights, std::align_val_t(32));
    operator delete[](bias, std::align_val_t(32));
}

template<typename in_type, int in_size, typename out_type, int out_size>
void NNLayer<in_type, in_size, out_type, out_size>::load_from_header(LayerName name){
    for (int i = 0; i < input_size*output_size; i++){
        weights[i] = nn_data::weights[name][i];
    }
    for (int i = 0; i < output_size; i++){
        bias[i] = nn_data::bias[name][i];
    }
};

// feature_transformer
template class NNLayer<int16_t, HKP_size, int16_t, acc_size>;

// hidden layers
template class NNLayer<int8_t, 512, int32_t, 32>;
template class NNLayer<int8_t, 32, int32_t, 32>;

// output layer
template class NNLayer<int8_t, 32, int16_t, 1>;


/*************
HiddenLayer
*************/

inline __m256i _mm256_add8x256_epi32(__m256i* inputs){

    inputs[1] = _mm256_castps_si256(_mm256_permute_ps(_mm256_castsi256_ps(inputs[1]), 0b00111001));
    inputs[2] = _mm256_castps_si256(_mm256_permute_ps(_mm256_castsi256_ps(inputs[2]), 0b01001110));
    inputs[3] = _mm256_castps_si256(_mm256_permute_ps(_mm256_castsi256_ps(inputs[3]), 0b10010011));

    inputs[5] = _mm256_castps_si256(_mm256_permute_ps(_mm256_castsi256_ps(inputs[5]), 0b00111001));
    inputs[6] = _mm256_castps_si256(_mm256_permute_ps(_mm256_castsi256_ps(inputs[6]), 0b01001110));
    inputs[7] = _mm256_castps_si256(_mm256_permute_ps(_mm256_castsi256_ps(inputs[7]), 0b10010011));

    // alternative:
    // inputs[0] = _mm256_add_epi32(inputs[0], inputs[1]);
    // inputs[0] = _mm256_add_epi32(inputs[0], inputs[2]);
    // inputs[0] = _mm256_add_epi32(inputs[0], inputs[3]);

    inputs[0] = _mm256_add_epi32(inputs[0], inputs[1]);
    inputs[2] = _mm256_add_epi32(inputs[2], inputs[3]);
    inputs[0] = _mm256_add_epi32(inputs[0], inputs[2]);

    
    inputs[4] = _mm256_add_epi32(inputs[4], inputs[5]);
    inputs[6] = _mm256_add_epi32(inputs[6], inputs[7]);
    inputs[4] = _mm256_add_epi32(inputs[4], inputs[6]);
    
    return _mm256_add_epi32(
        inputs[0],
        // swap lanes of the second register
        _mm256_castps_si256(_mm256_permute2f128_ps(_mm256_castsi256_ps(inputs[4]), _mm256_castsi256_ps(inputs[4]), 1))
    );
};


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
template<int in_size, int out_size>
void SparseLayer<in_size, out_size>::run(int8_t* input, int32_t* output){
    // we process 4 int8s at a time, as an int32.
    constexpr int MAX_NNZ_INPUTS = in_size / sizeof(uint32_t);
    int nnz_indices[MAX_NNZ_INPUTS];
    int num_nnz_inputs = 0;

    __m256i output_chunks[int32_per_reg];
    const __m256i one = _mm256_set1_epi16(1);

    __m256i input_chunk;
    for (int i = 0; i < in_size / int8_per_reg; i++){
        input_chunk = _mm256_loadu_si256((const __m256i*)&input[i*int8_per_reg]);
        int nnz_bitmask = _mm256_movemask_ps(
            (__m256)_mm256_cmpeq_epi32(input_chunk, _mm256_setzero_si256())
        );

        int idx;
        while (nnz_bitmask){
            idx = lsb(nnz_bitmask);
            nnz_bitmask &= nnz_bitmask - 1;
            nnz_indices[num_nnz_inputs++] = i*int8_per_reg + idx;
        }
    }


    // load the bias from memory
    for (int i = 0; i < in_size / int32_per_reg; i++){
        output_chunks[i] = _mm256_loadu_si256((const __m256i*)&this->bias[i*int32_per_reg]);
    }

    for (int i = 0; i < num_nnz_inputs; i++){
        // load the nonzero input group
        const __m256i input_group = _mm256_set1_epi32(*reinterpret_cast<const int32_t*>(&input[nnz_indices[i]]));
        for (int j = 0; j < num_output_chunks; j++){
            output_chunks[j] = _mm256_maddubs_epi16(
                input_group,
                _mm256_loadu_si256((const __m256i*)&this->weights[(nnz_indices[i]*out_size*4) + j*int8_per_reg])
            );
            output_chunks[j] = _mm256_madd_epi16(output_chunks[j], one); // hadd pairs to int32
        }
    }
    
    for (int i = 0; i < int32_per_reg; i++){
        // this integer divides the result by 64 which is the scale.
        output_chunks[i] = _mm256_srai_epi32(output_chunks[i], 6);
        _mm256_storeu_si256((__m256i*)&output[i*int32_per_reg], output_chunks[i]); // store int32
    }
};

// dense matrix multiplication
template<int in_size, int out_size>
void DenseLayer<in_size, out_size>::run(int8_t* input, int32_t* output){

    __m256i output_chunks[int32_per_reg];
    const __m256i one = _mm256_set1_epi16(1);

    for (int j = 0; j < num_output_chunks; j++){
        __m256i result = _mm256_loadu_si256((const __m256i*)&this->bias[j*int32_per_reg]);
        for (int i = 0; i < num_input_chunks; i++){
            __m256i input_chunk = _mm256_loadu_si256((const __m256i*)&input[i*int8_per_reg]); // load int8
            for (int k = 0; k < int32_per_reg; k++){
                output_chunks[k] = _mm256_maddubs_epi16(
                    input_chunk,
                    _mm256_loadu_si256((const __m256i*)&this->weights[(j*int32_per_reg+k) * this->input_size + i*int8_per_reg]) //load int8
                );
                output_chunks[k] = _mm256_madd_epi16(output_chunks[k], one); // hadd pairs to int32
            }
            result = _mm256_add_epi32(result, _mm256_add8x256_epi32(output_chunks));
        }
        result = _mm256_srai_epi32(result, 6); // this integer divides the result by 64 which is the scale.
        _mm256_storeu_si256((__m256i*)&output[j*int32_per_reg], result); // store int32
    }
};

// layer 2
template class SparseLayer<512, 32>;

// layer 3
template class DenseLayer<32, 32>;

/*************
OutputLayer
*************/

int16_t OutputLayer::run(int8_t* input){

    __m256i input_reg = _mm256_loadu_si256((const __m256i*)input); // load int8

    __m256i output_reg = _mm256_maddubs_epi16(input_reg, _mm256_loadu_si256((const __m256i*)weights)); // load int8
    // output now in epi16
    
    // accumulate together
    output_reg = _mm256_hadd_epi16(output_reg, output_reg);
    output_reg = _mm256_hadd_epi16(output_reg, output_reg);
    
    int16_t out_ptr[16];
    _mm256_storeu_si256((__m256i*)out_ptr, output_reg); // store int16

    return out_ptr[0] + out_ptr[1] + out_ptr[8] + out_ptr[9] + bias[0];

};

/****************
Accumulator class
****************/
int16_t* Accumulator::operator[](bool color){
    return color ? &accumulator[0] : &accumulator[acc_size];
};
/*********
NNUE class
*********/

NNUE::NNUE(){
    load_model();
};

void NNUE::load_model(){
    feature_transformer.load_from_header(FEATURE_TRANSFORMER);
    layer_2.load_from_header(LAYER_1);
    layer_3.load_from_header(LAYER_2);
    layer_4.load_from_header(LAYER_3);
};

void NNUE::compute_accumulator(const std::vector<int> active_features, bool color){
    // we have 256 int16 to process, and there are 16 avx registers which can hold 16 int16.
    // therefore we need only one pass to the registers.

    __m256i avx_regs[num_avx_registers];

    // load the bias from memory
    for (int i = 0; i < num_avx_registers; i++){
        avx_regs[i] = _mm256_loadu_si256((const __m256i*)&feature_transformer.bias[i*int16_per_reg]); // load int16
    }

    for (const int &a: active_features){
        for (int i = 0; i < num_avx_registers; i++){
            // a*acc size is the index of the a-th row. We then accumulate the weights.
            avx_regs[i] = _mm256_add_epi16(
                avx_regs[i],
                _mm256_loadu_si256((const __m256i*)&feature_transformer.weights[a*acc_size + i*int16_per_reg]) // load int16
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
            _mm256_loadu_si256((const __m256i*)&feature_transformer.weights[m_features.added*acc_size + i*int16_per_reg]) // load int16
            );
    }
    // removed feature
    for (int i = 0; i < num_avx_registers; i++){
        // m_features.removed*acc_size is to get the right column.
        avx_regs[i] = _mm256_sub_epi16(
            avx_regs[i],
            _mm256_loadu_si256((const __m256i*)&feature_transformer.weights[m_features.removed*acc_size + i*int16_per_reg]) // load int16
            );
    }

    if (m_features.captured != -1){
        for (int i = 0; i < num_avx_registers; i++){
            avx_regs[i] = _mm256_sub_epi16(
                avx_regs[i],
                _mm256_loadu_si256((const __m256i*)&feature_transformer.weights[m_features.captured*acc_size + i*int16_per_reg]) // load int16
                );
        }
    }

    //store the result in the accumulator
    for (int i = 0; i < num_avx_registers; i++){
        _mm256_storeu_si256((__m256i*)&accumulator[color][i*int16_per_reg], avx_regs[i]); // store int16
    }
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
        __m256i out = _mm256_max_epi8(_mm256_packs_epi16(in_1, in_2), zero); // packs saturates at 127, so only max is applied
        out = _mm256_permute4x64_epi64(out, 0b11011000);
        _mm256_storeu_si256((__m256i*)&output[i*int8_per_reg], out); // store int8
    }
};

// input size must be a multiple of 32
// there is no max size.
void NNUE::crelu32(int32_t *input, int8_t *output, int size){

    assert(size % int8_per_reg == 0);

    const int num_regs = size / int8_per_reg;
    const __m256i zero = _mm256_setzero_si256();

    for (int i = 0; i < num_regs; i++){
        __m256i in_1 = _mm256_loadu_si256((const __m256i*)&input[(4*i)*int32_per_reg]); // load int32
        __m256i in_2 = _mm256_loadu_si256((const __m256i*)&input[(4*i+1)*int32_per_reg]); // load int32
        __m256i in_3 = _mm256_loadu_si256((const __m256i*)&input[(4*i+2)*int32_per_reg]); // load int32
        __m256i in_4 = _mm256_loadu_si256((const __m256i*)&input[(4*i+3)*int32_per_reg]); // load int32

        in_1 = _mm256_permute4x64_epi64(_mm256_packs_epi32(in_1, in_2), 0b10'00'11'01);
        in_3 = _mm256_permute4x64_epi64(_mm256_packs_epi32(in_3, in_4), 0b10'00'11'01);

        __m256i out = _mm256_packs_epi16(in_1, in_3);
        out = _mm256_max_epi8(out, zero); // packs saturates at 127, so only max is applied
        out = _mm256_permute4x64_epi64(out, 0b01'11'00'10);
        _mm256_storeu_si256((__m256i*)&output[i*int8_per_reg], out); // store int8
    }
};

int NNUE::run_cropped_nn(bool color){
    crelu16(accumulator[color], &ft_clipped_output[0], acc_size);
    crelu16(accumulator[!color], &ft_clipped_output[acc_size], acc_size);

    layer_2.run(ft_clipped_output, layer_2_unclipped_output);
    crelu32(layer_2_unclipped_output, layer_2_clipped_output, layer_2.output_size);

    layer_3.run(layer_2_clipped_output, layer_3_unclipped_output);
    crelu32(layer_3_unclipped_output, layer_3_clipped_output, layer_3.output_size);

    int16_t output = layer_4.run(layer_3_clipped_output);
    return output / 16;

};

