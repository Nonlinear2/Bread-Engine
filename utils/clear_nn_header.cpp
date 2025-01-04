#include <iostream>
#include <fstream>
#include <cstdint>

int main(){
    std::ofstream output_file;
    output_file.open(bread_NN_HEADER_PATH);
    output_file << "#pragma once\n";
    output_file << "#include <cstdint>\n";
    output_file << "// this file has been auto generated. Please do not modify it by hand.\n\n";

    output_file << "namespace nn_data {\n";

    output_file << "inline int w1[] = {};\n";
    output_file << "inline int w2[] = {};\n";
    output_file << "inline int w3[] = {};\n";
    output_file << "inline int w4[] = {};\n";

    output_file << "inline int* weights[] = {w1, w2, w3, w4};\n";

    output_file << "inline int b1[] = {};\n";
    output_file << "inline int b2[] = {};\n";
    output_file << "inline int b3[] = {};\n";
    output_file << "inline int b4[] = {};\n";

    output_file << "inline int* bias[] = {b1, b2, b3, b4};\n";
    output_file << "}";
    output_file.close();
    return 0;
}