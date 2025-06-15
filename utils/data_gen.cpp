#include "core.hpp"
#include "chess.hpp"
#include <iostream>
#include <Windows.h>

float sigmoid(float x){
    return x / (2 + 2*std::abs(x)) + 0.5;
}

bool move_in_list(Move move, Movelist ml){
    for (auto m: ml){
        if (move == m)
            return true;
    }
    return false;
}

int main(){
    std::string path = "";
    std::ofstream output_file;

    std::vector<bool> stm = {};
    std::vector<std::string> fens = {};
    std::vector<int> scores = {};

    output_file.open(path, std::ios::app);

    const float scale = 600;
    const float alpha = 0.3;

    const int total_games = 1'000'000;

    srand((unsigned int)time(NULL));
    Board cb = Board();
    Engine engine = Engine();

    engine.set_uci_display(false);

    Movelist move_list;
    Move best_move;
    int value;
    float wdl;
    int count = 0;
    for (int i = 0; i < total_games; i++){
        cb.setFen(constants::STARTPOS);
        for (int i = 0; i < 7; i++){
            if (std::get<1>(cb.isGameOver()) != GameResult::NONE){
                break;
            }
            movegen::legalmoves(move_list, cb);
            cb.makeMove(move_list[rand()%move_list.size()]);
        }
        while (std::get<1>(cb.isGameOver()) == GameResult::NONE){
            movegen::legalmoves(move_list, cb);
            best_move = engine.search(cb.getFen(), SearchLimit(LimitType::Nodes, 5000));

            if (!move_in_list(best_move, move_list)){
                std::cerr << "illegal move played" << std::endl;
            }

            if (!cb.isCapture(best_move) && !cb.inCheck() && std::abs(best_move.score()) < TB_VALUE){
                stm.push_back(cb.sideToMove() == Color::WHITE);
                fens.push_back(cb.getFen());
                scores.push_back(best_move.score());
            }
            cb.makeMove(best_move);
        }

        if (std::get<1>(cb.isGameOver()) == GameResult::DRAW){
            wdl = 0.5;
        } else if (cb.sideToMove() == Color::WHITE){
            wdl = 0;
        } else {
            wdl = 1;
        }

        for (int i = 0; i < fens.size(); i++){
            output_file << fens[i]
                        << ", "
                        << sigmoid(scores[i] / scale) * (1.0 - alpha) + alpha * (stm[i] ? wdl : 1 - wdl)
                        <<  "\n";
        }
        fens.clear();
        scores.clear();
    
        if((GetKeyState('1') & 0x8000) && (GetKeyState('5') & 0x8000) && (GetKeyState('0') & 0x8000)){
            break;
        }

        count++;
        if (count % 100 == 0){
            std::cout << "games completed: " << count << "/" << total_games << std::endl;
        }
    }
    return 0;
}   