#include "core.hpp"
#include "nnue.cpp"
#include <fstream>

std::vector<std::string> load_fens(const std::string& filename) {
    std::vector<std::string> fens;
    std::ifstream file(filename);
    std::string line;
    
    if (!file.is_open()) {
        std::cerr << "error, could not open file " << filename << std::endl;
        return fens;
    }
    
    while (std::getline(file, line))
        fens.push_back(line);

    file.close();
    return fens;
}

int main(){

    std::cout << "============================== \n";
    std::cout << "accumulator activations benchmark: \n";

    std::array<int, ACC_SIZE> zero_count = {0};

    std::vector<std::string> fens = load_fens(bread_NNUE_FENS_PATH);

    NnueBoard board = NnueBoard();
    for (auto fen: fens){

        board.setFen(fen);
        board.synchronize();
    
        Color stm = board.sideToMove();
        Accumulators& accumulators = board.accumulators_stack.top();
        uint8_t ft_clamped_output[L1_INPUT_SIZE];
    
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

        for (int j = 0; j < ACC_SIZE; j++){
            zero_count[j] += (ft_clamped_output[j] == 0);
        }
    }

    for (int i = 0; i < ACC_SIZE; i++){
        std::cout << zero_count[i] << ", ";
    }

    std::cout << std::endl;
    std::cout << "============================== \n";
    return 0;
}