#include "datagen.hpp"

namespace Datagen {

void genfens(std::mt19937 rng, int count){
    Movelist move_list;
    Board board = Board();
    
    for (int i = 0; i < count; i++){
        do {
            board.setFen(constants::STARTPOS);
            for (int j = 0; j < NUM_GENFENS_RANDOM_MOVES; j++){
                movegen::legalmoves(move_list, board);
                board.makeMove(move_list[rng() % move_list.size()]);
                if (std::get<1>(board.isGameOver()) != GameResult::NONE)
                    break;
            }
        } while (std::get<1>(board.isGameOver()) != GameResult::NONE);
    
        std::cout << "info string genfens " << board.getFen() << std::endl;
    }
}

} // namespace Benchmark
