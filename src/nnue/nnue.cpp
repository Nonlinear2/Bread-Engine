#include "nnue.hpp"

/*************
NN Layers
*************/

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
void NNLayer<in_type, in_size, out_type, out_size>::load_from_binary(std::string path){
    std::string weights_path = path + "/weights.bin";
    std::string bias_path = path + "/bias.bin";
    std::ifstream weights_stream;
    weights_stream.open(weights_path, std::ios::binary);
    if (weights_stream.is_open()){
        weights_stream.read(reinterpret_cast<char*>(weights), 
                            input_size*output_size*sizeof(in_type));        
    } else {
        std::cout << "error loading weights \n";
    }

    std::ifstream bias_stream;
    bias_stream.open(bias_path, std::ios::binary);
    if (bias_stream.is_open()){
        bias_stream.read(reinterpret_cast<char*>(bias), output_size*sizeof(out_type));        
    } else {
        std::cout << "error loading bias \n";
    }
};

template<typename in_type, int in_size, typename out_type, int out_size>
void NNLayer<in_type, in_size, out_type, out_size>::load_from_header(LayerName name){
    #if bread_EMBED_NN
    for (int i = 0; i < input_size*output_size; i++){
        weights[i] = nn_data::weights[name][i];
    }
    for (int i = 0; i < output_size; i++){
        bias[i] = nn_data::bias[name][i];
    }
    #endif
};

inline __m256i _mm256_add8x256_epi32(__m256i* inputs){ // 8 256 avx registers with contents to hadd.
    inputs[0] = _mm256_hadd_epi32(inputs[0], inputs[1]);
    inputs[2] = _mm256_hadd_epi32(inputs[2], inputs[3]);
    inputs[4] = _mm256_hadd_epi32(inputs[4], inputs[5]);
    inputs[6] = _mm256_hadd_epi32(inputs[6], inputs[7]);

    inputs[0] = _mm256_hadd_epi32(inputs[0], inputs[2]);
    inputs[4] = _mm256_hadd_epi32(inputs[4], inputs[6]);

    return _mm256_add_epi32(
        // swap lanes of the two regs
        _mm256_permute2x128_si256(inputs[0], inputs[4], 0b00110001),
        _mm256_permute2x128_si256(inputs[0], inputs[4], 0b00100000)
    );
};

template<int in_size, int out_size>
void HiddenLayer<in_size, out_size>::run(int8_t* input, int32_t* output){

    __m256i output_chunks[int32_per_reg];
    const __m256i one = _mm256_set1_epi16(1);

    for (int j = 0; j < num_output_chunks; j++){
        __m256i result = _mm256_set1_epi32(0);
        for (int i = 0; i < num_input_chunks; i++){
            __m256i input_chunk = _mm256_loadu_epi8(&input[i*int8_per_reg]);
            for (int k = 0; k < int32_per_reg; k++){
                output_chunks[k] = _mm256_maddubs_epi16(
                    input_chunk,
                    _mm256_loadu_epi8(&this->weights[(j*int32_per_reg+k) * this->input_size + i*int8_per_reg])
                );
                output_chunks[k] = _mm256_madd_epi16(output_chunks[k], one); // hadd pairs to int32
            }
            result = _mm256_add_epi32(result, _mm256_add8x256_epi32(output_chunks));
        }
        result = _mm256_add_epi32(result, _mm256_loadu_epi32(&this->bias[j*int32_per_reg]));
        result = _mm256_srai_epi32(result, 6); // this integer divides the result by 64 which is the scale.
        _mm256_storeu_epi32(&output[j*int32_per_reg], result);
    }     
};

int16_t OutputLayer::run(int8_t* input){

    __m256i input_reg = _mm256_loadu_epi8(input);

    __m256i output_reg = _mm256_maddubs_epi16(input_reg, _mm256_loadu_epi8(weights));
    // output now in epi16
    
    // accumulate together
    output_reg = _mm256_hadd_epi16(output_reg, output_reg);
    output_reg = _mm256_hadd_epi16(output_reg, output_reg);
    
    int16_t out_ptr[16];
    _mm256_storeu_epi16(out_ptr, output_reg);

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

NNUE::NNUE(std::string model_path){
    load_model(model_path);
};

void NNUE::load_model(std::string path){
    #if bread_EMBED_NN
    feature_transformer.load_from_header(FEATURE_TRANSFORMER);
    layer_2.load_from_header(LAYER_1);
    layer_3.load_from_header(LAYER_2);
    layer_4.load_from_header(LAYER_3);
    #else
    feature_transformer.load_from_binary(path + "/feature_transformer");
    layer_2.load_from_binary(path + "/layer_2");
    layer_3.load_from_binary(path + "/layer_3");
    layer_4.load_from_binary(path + "/layer_4");
    #endif
};

void NNUE::compute_accumulator(const std::vector<int> active_features, bool color){
    // we have 256 int16 to process, and there are 16 avx registers which can hold 16 int16.
    // therefore we need only one pass to the registers.

    __m256i avx_regs[num_avx_registers];

    // load the bias from memory
    for (int i = 0; i < num_avx_registers; i++){
        avx_regs[i] = _mm256_loadu_epi16(&feature_transformer.bias[i*int16_per_reg]);
    }

    for (const int &a: active_features){
        for (int i = 0; i < num_avx_registers; i++){
            // a*acc size is the index of the a-th row. We then accumulate the weights.
            avx_regs[i] = _mm256_add_epi16(
                avx_regs[i],
                _mm256_loadu_epi16(&feature_transformer.weights[a*acc_size + i*int16_per_reg])
                );
        }
    }
    // store the result in the accumulator
    for (int i = 0; i < num_avx_registers; i++){
        _mm256_storeu_epi16(&accumulator[color][i*int16_per_reg], avx_regs[i]);
    }
};

void NNUE::update_accumulator(const modified_features m_features, bool color){

    __m256i avx_regs[num_avx_registers];

    // load the accumulator
    for (int i = 0; i < num_avx_registers; i++){
        avx_regs[i] = _mm256_loadu_epi16(&accumulator[color][i*int16_per_reg]);
    }

    // added feature
    for (int i = 0; i < num_avx_registers; i++){
        // m_features.added*acc_size is the index of the added featured row. We then accumulate the weights.
        avx_regs[i] = _mm256_add_epi16(
            avx_regs[i],
            _mm256_loadu_epi16(&feature_transformer.weights[m_features.added*acc_size + i*int16_per_reg])
            );
    }
    // removed feature
    for (int i = 0; i < num_avx_registers; i++){
        // m_features.removed*acc_size is to get the right column.
        avx_regs[i] = _mm256_sub_epi16(
            avx_regs[i],
            _mm256_loadu_epi16(&feature_transformer.weights[m_features.removed*acc_size + i*int16_per_reg])
            );
    }

    if (m_features.captured != -1){
        for (int i = 0; i < num_avx_registers; i++){
            avx_regs[i] = _mm256_sub_epi16(
                avx_regs[i],
                _mm256_loadu_epi16(&feature_transformer.weights[m_features.captured*acc_size + i*int16_per_reg])
                );
        }
    }

    //store the result in the accumulator
    for (int i = 0; i < num_avx_registers; i++){
        _mm256_storeu_epi16(&accumulator[color][i*int16_per_reg], avx_regs[i]);
    }
};


// input size must be a multiple of 32
// there is no max size.
void NNUE::crelu16(int16_t *input, int8_t *output, int size){

    assert(size % int8_per_reg == 0);

    const int num_regs = size / int8_per_reg;
    const __m256i zero = _mm256_setzero_si256();

    for (int i = 0; i < num_regs; i++){
        __m256i in_1 = _mm256_loadu_epi16(&input[(2*i)*int16_per_reg]);
        __m256i in_2 = _mm256_loadu_epi16(&input[(2*i+1)*int16_per_reg]);
        __m256i out = _mm256_max_epi8(_mm256_packs_epi16(in_1, in_2), zero); // packs saturates at 127, so only max is applied
        out = _mm256_permute4x64_epi64(out, 0b11011000);
        _mm256_storeu_epi8(&output[i*int8_per_reg], out);
    }
};

// input size must be a multiple of 32
// there is no max size.
void NNUE::crelu32(int32_t *input, int8_t *output, int size){

    assert(size % int8_per_reg == 0);

    const int num_regs = size / int8_per_reg;
    const __m256i zero = _mm256_setzero_si256();

    for (int i = 0; i < num_regs; i++){
        __m256i in_1 = _mm256_loadu_epi32(&input[(4*i)*int32_per_reg]);
        __m256i in_2 = _mm256_loadu_epi32(&input[(4*i+1)*int32_per_reg]);
        __m256i in_3 = _mm256_loadu_epi32(&input[(4*i+2)*int32_per_reg]);
        __m256i in_4 = _mm256_loadu_epi32(&input[(4*i+3)*int32_per_reg]);

        in_1 = _mm256_permute4x64_epi64(_mm256_packs_epi32(in_1, in_2), 0b10'00'11'01);
        in_3 = _mm256_permute4x64_epi64(_mm256_packs_epi32(in_3, in_4), 0b10'00'11'01);

        __m256i out = _mm256_packs_epi16(in_1, in_3);
        out = _mm256_max_epi8(out, zero); // packs saturates at 127, so only max is applied
        out = _mm256_permute4x64_epi64(out, 0b01'11'00'10);
        _mm256_storeu_epi8(&output[i*int8_per_reg], out);
    }
};

float NNUE::run_cropped_nn(bool color){
    crelu16(accumulator[color], &ft_clipped_output[0], acc_size);
    crelu16(accumulator[!color], &ft_clipped_output[acc_size], acc_size);

    layer_2.run(ft_clipped_output, layer_2_unclipped_output);
    crelu32(layer_2_unclipped_output, layer_2_clipped_output, layer_2.output_size);

    layer_3.run(layer_2_clipped_output, layer_3_unclipped_output);
    crelu32(layer_3_unclipped_output, layer_3_clipped_output, layer_3.output_size);

    int16_t output = layer_4.run(layer_3_clipped_output);
    return std::tanh(static_cast<float>(output)/(64*127));

};

