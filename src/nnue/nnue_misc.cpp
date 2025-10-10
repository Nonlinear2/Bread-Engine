#include "nnue_misc.hpp"

[[maybe_unused]]
void NNUE_UTILS::crelu32_to_8(int32_t *input, int8_t *output, int size){

    assert(size % INT8_PER_REG == 0);

    const int num_regs = size / INT8_PER_REG;
    const vec_int8 zero = setzero_epi8();

    for (int i = 0; i < num_regs; i++){
        vec_int32 in_1 = load_epi32(&input[(4*i)*INT32_PER_REG]);
        vec_int32 in_2 = load_epi32(&input[(4*i+1)*INT32_PER_REG]);
        vec_int32 in_3 = load_epi32(&input[(4*i+2)*INT32_PER_REG]);
        vec_int32 in_4 = load_epi32(&input[(4*i+3)*INT32_PER_REG]);

        in_1 = permute4x64_epi64<0b10'00'11'01>(packs_epi32(in_1, in_2));
        in_3 = permute4x64_epi64<0b10'00'11'01>(packs_epi32(in_3, in_4));

        vec_int8 out = packs_epi16(in_1, in_3);
        out = max_epi8(out, zero); // packs saturates at 127, so only max is applied
        out = permute4x64_epi64<0b01'11'00'10>(out);
        store_epi8(&output[i*INT8_PER_REG], out);
    }
}

[[maybe_unused]]
void NNUE_UTILS::crelu16_to_8(int16_t *input, int8_t *output, int size){

    assert(size % INT8_PER_REG == 0);

    const int num_regs = size / INT8_PER_REG;

    for (int i = 0; i < num_regs; i++){
        vec_int16 in_1 = load_epi16(&input[(2*i)*INT16_PER_REG]);
        vec_int16 in_2 = load_epi16(&input[(2*i+1)*INT16_PER_REG]);
        // packs sets negative values to 0 and saturates at 255, which effectively applies relu
        vec_int8 out = packus_epi16(in_1, in_2);
        out = permute4x64_epi64<0b11'01'10'00>(out); // undo the packus shuffle
        store_epi8(&output[i*INT8_PER_REG], out);
    }
}

[[maybe_unused]]
void NNUE_UTILS::crelu16_to_16(int16_t *input, int16_t *output, int size){

    assert(size % INT16_PER_REG == 0);

    const vec_int16 zero = setzero_epi16();
    const vec_int16 qscale = set1_epi16(255);

    for (int i = 0; i < size; i += INT16_PER_REG){
        vec_int16 in = load_epi16(&input[i]);
        vec_int16 out = min_epi16(qscale, max_epi16(in, zero));
        store_epi16(&output[i], out);
    }
}

int32_t NNUE_UTILS::reduce1_epi32(vec_int32& input){ // horizontal add 1 int32 avx register.
    input = hadd_epi32(input, input);

    int32_t out_ptr[8];
    store_epi32(out_ptr, input);

    return out_ptr[0] + out_ptr[1] + out_ptr[4] + out_ptr[5];
}

[[maybe_unused]]
vec_int32 NNUE_UTILS::reduce8_epi32(vec_int32* inputs){ // horizontal add 8 int32 avx registers.
    inputs[0] = hadd_epi32(inputs[0], inputs[1]);
    inputs[2] = hadd_epi32(inputs[2], inputs[3]);
    inputs[4] = hadd_epi32(inputs[4], inputs[5]);
    inputs[6] = hadd_epi32(inputs[6], inputs[7]);

    inputs[0] = hadd_epi32(inputs[0], inputs[2]);
    inputs[4] = hadd_epi32(inputs[4], inputs[6]);

    return add_epi32(
        // swap lanes of the two registers
        permute2x128_si256<0b00110001>(inputs[0], inputs[4]),
        permute2x128_si256<0b00100000>(inputs[0], inputs[4])
    );
}
