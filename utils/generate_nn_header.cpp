#include <iostream>
#include <fstream>
#include <cstdint>


// TODO: convert to int8 in header to lower executable size?


// this functions writes the weights from input_path to output_path with comas separating the values
template<typename weights_type>
void parameters_to_header(std::string input_path, int weights_size, std::ofstream& output_file){
    std::ifstream weights_stream;
    weights_type* weights = new weights_type[weights_size];

    weights_stream.open(input_path, std::ios::binary);
    if (weights_stream.is_open()){
        weights_stream.read(reinterpret_cast<char*>(weights), 
                            weights_size*sizeof(weights_type));        
    } else {std::cout << "error loading weights \n";}

    for (int i = 0; i < weights_size; i++){
        output_file << static_cast<int>(weights[i]) << ", ";
    }
};

int main(){
    std::string model_path = bread_NNUE_MODEL_PATH;

    // feature transformer
    std::string ft_path = model_path + "/feature_transformer";
    constexpr int ft_in_size = 40960;
    constexpr int ft_out_size = 256;

    // layer 1
    std::string l2_path = model_path + "/layer_2";
    constexpr int l2_in_size = 512;
    constexpr int l2_out_size = 32;

    // layer 2
    std::string l3_path = model_path + "/layer_3";
    constexpr int l3_in_size = 32;
    constexpr int l3_out_size = 32;

    // output layer
    std::string l4_path = model_path + "/layer_4";
    constexpr int l4_in_size = 32;
    constexpr int l4_out_size = 1;

    std::ofstream output_file;
    output_file.open(bread_NN_HEADER_PATH);
    output_file << "#pragma once\n";
    output_file << "#include <cstdint>\n";
    output_file << "// this file has been auto generated. Please do not modify it by hand.\n\n";

    output_file << "namespace nn_data {\n";

    output_file << "inline int w1[] = {";
    parameters_to_header<int16_t>(ft_path + "/weights.bin", ft_in_size*ft_out_size, output_file);
    output_file << "};\n";
    output_file << "inline int w2[] = {";
    parameters_to_header<int8_t>(l2_path + "/weights.bin", l2_in_size*l2_out_size, output_file);
    output_file << "};\n";
    output_file << "inline int w3[] = {";
    parameters_to_header<int8_t>(l3_path + "/weights.bin", l3_in_size*l3_out_size, output_file);
    output_file << "};\n";
    output_file << "inline int w4[] = {";
    parameters_to_header<int8_t>(l4_path + "/weights.bin", l4_in_size*l4_out_size, output_file);
    output_file << "};\n";

    output_file << "inline int* weights[] = {w1, w2, w3, w4};\n";

    output_file << "inline int b1[] = {";
    parameters_to_header<int16_t>(ft_path + "/bias.bin", ft_out_size, output_file);
    output_file << "};\n";
    output_file << "inline int b2[] = {";
    parameters_to_header<int32_t>(l2_path + "/bias.bin", l2_out_size, output_file);
    output_file << "};\n";
    output_file << "inline int b3[] = {";
    parameters_to_header<int32_t>(l3_path + "/bias.bin", l3_out_size, output_file);
    output_file << "};\n";
    output_file << "inline int b4[] = {";
    parameters_to_header<int16_t>(l4_path + "/bias.bin", l4_out_size, output_file);
    output_file << "};\n";

    output_file << "inline int* bias[] = {b1, b2, b3, b4};\n";
    output_file << "}";
    output_file.close();
    return 0;
}