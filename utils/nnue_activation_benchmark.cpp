#include "core.hpp"
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

    std::array<int, acc_size> zero_count = {0};

    std::vector<std::string> fens = load_fens(bread_NNUE_FENS_PATH);

    NnueBoard board = NnueBoard();
    for (auto fen: fens){

        board.setFen(fen);
        board.synchronize();
    
        board.nnue_.run_accumulator_activation(board.sideToMove() == Color::WHITE);
        for (int i = 0; i < 2; i++){
            for (int j = 0; j < acc_size; j++){
                if (board.nnue_.ft_clipped_output[i*acc_size + j] == 0)
                    zero_count[j]++;
            }
        }
    }

    for (int i = 0; i < acc_size; i++){
        std::cout << zero_count[i] << ", ";
    }

    std::cout << std::endl;
    std::cout << "============================== \n";
    return 0;
}