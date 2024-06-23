#include <iostream>
#include <vector>
#include <fstream>
#include "nnue.hpp"

int main(){
    NNUE nnue(bread_NNUE_MODEL_PATH);

    std::ofstream output_file;
    output_file.open(bread_NN_HEADER_PATH);
    output_file << "#pragma once\n";
    output_file << "#include <cstdint>\n";
    output_file << "#include <vector>\n";
    output_file << "#define NN_HEADER_GENERATED 1\n";
    output_file << "// this file has been auto generated. Please do not modify it by hand.\n\n";

    output_file << "namespace nn_data {\n";

    output_file << "inline int w1[] = {";
    nnue.feature_transformer.weights_to_header(output_file);
    output_file << "};\n";
    output_file << "inline int w2[] = {";
    nnue.layer_2.weights_to_header(output_file);
    output_file << "};\n";
    output_file << "inline int w3[] = {";
    nnue.layer_3.weights_to_header(output_file);
    output_file << "};\n";
    output_file << "inline int w4[] = {";
    nnue.layer_4.weights_to_header(output_file);
    output_file << "};\n";

    output_file << "inline int* weights[] = {w1, w2, w3, w4};\n";

    output_file << "inline int b1[] = {";
    nnue.feature_transformer.bias_to_header(output_file);
    output_file << "};\n";
    output_file << "inline int b2[] = {";
    nnue.layer_2.bias_to_header(output_file);
    output_file << "};\n";
    output_file << "inline int b3[] = {";
    nnue.layer_3.bias_to_header(output_file);
    output_file << "};\n";
    output_file << "inline int b4[] = {";
    nnue.layer_4.bias_to_header(output_file);
    output_file << "};\n";

    output_file << "inline int* bias[] = {b1, b2, b3, b4};\n";
    output_file << "}";
    output_file.close();
    return 0;
}