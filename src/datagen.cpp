#include "datagen.hpp"

namespace Datagen {

void genfens(Engine& engine, std::mt19937 rng, int count){
    Movelist move_list;
    Board board = Board();
    
    for (int i = 0; i < count; i++){
        do {
            board.setFen(constants::STARTPOS);
            for (int j = 0; j < NUM_GENFENS_RANDOM_MOVES; j++){
                movegen::legalmoves(move_list, board);
                Move move;
                do {
                    move = move_list[rng() % move_list.size()];
                } while (
                    PSM::REF_VALUE / 2 + PSM::get_psm(board.at(move.from()), move.to()) <= rng() % (3 * PSM::REF_VALUE / 2)
                );

                board.makeMove(move);
                if (board.isGameOver().second != GameResult::NONE)
                    break;
            }
        } while (
            board.isGameOver().second != GameResult::NONE
            || [&]() {
                engine.pos.setFen(board.getFen());
                int value = engine.iterative_deepening(SearchLimit(LimitType::Depth, GENFENS_FILTER_DEPTH)).score();
                return std::abs(value) > GENFENS_MAX_VALUE;
            }()
        );

        std::cout << "info string genfens " << board.getFen() << std::endl;
    }
}

} // namespace Benchmark
