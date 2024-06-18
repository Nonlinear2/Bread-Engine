#include "bread_engine_core.hpp"

inline std::vector<std::string> fens = {
    "1k1r4/pp1b1R2/3q2pp/4p3/2B5/4Q3/PPP2B2/2K5 b - -",
    "3r1k2/4npp1/1ppr3p/p6P/P2PPPP1/1NR5/5K2/2R5 w - -",
    "2q1rr1k/3bbnnp/p2p1pp1/2pPp3/PpP1P1P1/1P2BNNP/2BQ1PRK/7R b - -",
    "rnbqkb1r/p3pppp/1p6/2ppP3/3N4/2P5/PPP1QPPP/R1B1KB1R w KQkq -",
    "r1b2rk1/2q1b1pp/p2ppn2/1p6/3QP3/1BN1B3/PPP3PP/R4RK1 w - -",
    "2r3k1/pppR1pp1/4p3/4P1P1/5P2/1P4K1/P1P5/8 w - -",
    "1nk1r1r1/pp2n1pp/4p3/q2pPp1N/b1pP1P2/B1P2R2/2P1B1PP/R2Q2K1 w - -",
    "4b3/p3kp2/6p1/3pP2p/2pP1P2/4K1P1/P3N2P/8 w - -",
    "2kr1bnr/pbpq4/2n1pp2/3p3p/3P1P1B/2N2N1Q/PPP3PP/2KR1B1R w - -",
    "3rr1k1/pp3pp1/1qn2np1/8/3p4/PP1R1P2/2P1NQPP/R1B3K1 b - -",
    "2r1nrk1/p2q1ppp/bp1p4/n1pPp3/P1P1P3/2PBB1N1/4QPPP/R4RK1 w - -",
    "r3r1k1/ppqb1ppp/8/4p1NQ/8/2P5/PP3PPP/R3R1K1 b - -",
    "r2q1rk1/4bppp/p2p4/2pP4/3pP3/3Q4/PP1B1PPP/R3R1K1 w - -",
    "rnb2r1k/pp2p2p/2pp2p1/q2P1p2/8/1Pb2NP1/PB2PPBP/R2Q1RK1 w - -",
    "2r3k1/1p2q1pp/2b1pr2/p1pp4/6Q1/1P1PP1R1/P1PN2PP/5RK1 w - -",
    "r1bqkb1r/4npp1/p1p4p/1p1pP1B1/8/1B6/PPPN1PPP/R2Q1RK1 w kq -",
    "r2q1rk1/1ppnbppp/p2p1nb1/3Pp3/2P1P1P1/2N2N1P/PPB1QP2/R1B2RK1 b - -",
    "r1bq1rk1/pp2ppbp/2np2p1/2n5/P3PP2/N1P2N2/1PB3PP/R1B1QRK1 b - -",
    "3rr3/2pq2pk/p2p1pnp/8/2QBPP2/1P6/P5PP/4RRK1 b - -",
    "r4k2/pb2bp1r/1p1qp2p/3pNp2/3P1P2/2N3P1/PPP1Q2P/2KRR3 w - -",
    "3rn2k/ppb2rpp/2ppqp2/5N2/2P1P3/1P5Q/PB3PPP/3RR1K1 w - -",
    "2r2rk1/1bqnbpp1/1p1ppn1p/pP6/N1P1P3/P2B1N1P/1B2QPP1/R2R2K1 b - -",
    "r1bqk2r/pp2bppp/2p5/3pP3/P2Q1P2/2N1B3/1PP3PP/R4RK1 b kq -",
    "r2qnrnk/p2b2b1/1p1p2pp/2pPpp2/1PP1P3/PRNBB3/3QNPPP/5RK1 w - -",
};

inline std::vector<std::string> correct_best_moves = {
    "Qd1+",
    "d5",
    "f5",
    "e6",
    "Nd5",
    "g6",
    "Nf6",
    "f5",
    "f5",
    "Ne5",
    "f4",
    "Bf5",
    "b4",
    "Qd2 ",
    "Qxg7+",
    "Ne4",
    "h5",
    "Nb3",
    "Rxe4",
    "g4",
    "Nh6",
    "Bxe4",
    "f6",
    "f4",
};

float sum(std::vector<float> values){
    float total = 0;
    for (auto v: values){
        total += v;
    }
    return total;
}

template<typename E>
float benchmark_engine_nodes(E& engine, int depth){
    std::vector<float> nodes;
    
    for (auto fen: fens){
        engine.search(fen, 1'000, depth, depth);
        nodes.push_back(engine.nodes);
    }
    float avg_nodes = sum(nodes)/fens.size();
    std::cout << "average nodes: " << avg_nodes << "\n";
    return avg_nodes;
}

void optimize_parameters(std::vector<float> params, int num_iter){
    int depth = 7;
    Engine engine = Engine();

    Engine::SortedMoveGen<chess::movegen::MoveGenType::ALL>::KILLER_SCORE = params[0];
    Engine::SortedMoveGen<chess::movegen::MoveGenType::ALL>::MATERIAL_CHANGE_MULTIPLIER = params[1];
    Engine::SortedMoveGen<chess::movegen::MoveGenType::ALL>::ENDGAME_PIECE_COUNT = params[2];

    Engine::SortedMoveGen<chess::movegen::MoveGenType::ALL>::PAWN_SCALE = params[3];
    Engine::SortedMoveGen<chess::movegen::MoveGenType::ALL>::BISHOP_SCALE = params[4];
    Engine::SortedMoveGen<chess::movegen::MoveGenType::ALL>::KNIGHT_SCALE = params[5];
    Engine::SortedMoveGen<chess::movegen::MoveGenType::ALL>::ROOK_SCALE = params[6];
    Engine::SortedMoveGen<chess::movegen::MoveGenType::ALL>::QUEEN_SCALE = params[7];
    Engine::SortedMoveGen<chess::movegen::MoveGenType::ALL>::ENDGAME_SCALE = params[8];
    Engine::SortedMoveGen<chess::movegen::MoveGenType::ALL>::MIDDLEGAME_SCALE = params[9];

    float best_nodes = benchmark_engine_nodes(engine, depth);
    float initial_nodes = best_nodes;
    std::vector<float> best_params = params;

    float node;
    for (int i = 0; i < num_iter; i++){
        engine.transposition_table.clear();

        for (int j = 0; j < best_params.size(); j++){
            float r = rand()%4;
            if ((r == 0)||(r == 1)){
                r=0;
            } else if (r == 2){
                r=0.1;
            } else if (r == 3){
                r=-0.1;
            }
            params[j] = std::max(best_params[j] + r, 0.0F);
        }
        Engine::SortedMoveGen<chess::movegen::MoveGenType::ALL>::KILLER_SCORE = params[0];
        Engine::SortedMoveGen<chess::movegen::MoveGenType::ALL>::MATERIAL_CHANGE_MULTIPLIER = params[1];
        Engine::SortedMoveGen<chess::movegen::MoveGenType::ALL>::ENDGAME_PIECE_COUNT = params[2];

        Engine::SortedMoveGen<chess::movegen::MoveGenType::ALL>::PAWN_SCALE = params[3];
        Engine::SortedMoveGen<chess::movegen::MoveGenType::ALL>::BISHOP_SCALE = params[4];
        Engine::SortedMoveGen<chess::movegen::MoveGenType::ALL>::KNIGHT_SCALE = params[5];
        Engine::SortedMoveGen<chess::movegen::MoveGenType::ALL>::ROOK_SCALE = params[6];
        Engine::SortedMoveGen<chess::movegen::MoveGenType::ALL>::QUEEN_SCALE = params[7];
        Engine::SortedMoveGen<chess::movegen::MoveGenType::ALL>::ENDGAME_SCALE = params[8];
        Engine::SortedMoveGen<chess::movegen::MoveGenType::ALL>::MIDDLEGAME_SCALE = params[9];


        std::cout << "============================\n";
        std::cout << "new params: ";
        for (auto it: params){
            std::cout << it << " ";
        }
        std::cout << "\n";
        std::cout << "============================\n";

        node = benchmark_engine_nodes(engine, depth);
        if (node < best_nodes){
            best_nodes = node;
            best_params = params;
        }
        std::cout << "============================\n";
        std::cout << "best nodes count so far: " << best_nodes << "\n";
        std::cout << "best params so far: ";
        for (auto it: best_params){
            std::cout << it << " ";
        }
        std::cout << "\n";
        std::cout << "============================\n";
    }

    std::cout << "initial nodes: " << initial_nodes << std::endl;
    std::cout << "final best nodes: " << best_nodes << "\n";
    std::cout << "final best params: ";
    for (auto it: best_params){
        std::cout << it << ", ";
    }
    std::cout << "\n";
}

int main(){
    srand(114);
    optimize_parameters({15.1, 12, 1.7, 2.3, 2, 1, 0.4, 0.8, 4.5, 0.5}, 100);
}

// initial nodes: 165571
// final best nodes: 156914
// final best params: 9.9 6.2 11.5 1.1 1.3 0.6 0.4 1.1 1.1 1

// initial nodes: 165571
// final best nodes: 156120
// final best params: 9.9, 6.4, 11.4, 0.9, 1.4, 0.6, 0.4, 0.8, 1.3, 0.6,

// initial nodes: 155894
// final best nodes: 155868
// final best params: 9.8, 6.4, 11.3, 0.9, 1.4, 0.6, 0.4, 0.8, 1.3, 0.5,

// initial nodes: 155886
// final best nodes: 154075
// final best params: 10.3, 6.9, 12.3, 0.4, 1.4, 0.6, 0, 0.8, 3.3, 1,

// initial nodes: 155645
// final best nodes: 153546
// final best params: 10.5, 7.3, 13.1, 0.4, 1.4, 0.6, 0.2, 0.8, 3.3, 1.6,

// initial nodes: 166302
// final best nodes: 166037
// final best params: 10.6, 7.3, 13.1, 0.5, 1.4, 0.5, 0.2, 0.8, 3.2, 1.7,

// initial nodes: 165522
// final best nodes: 165395
// final best params: 15.2, 12.1, 1.6, 2.1, 1.9, 0.9, 0.8, 0.9, 4.9, 1,

//depth 7
// initial nodes: 505647
// final best nodes: 463794
// final best params: 14.2, 12.1, 2.6, 1.1, 3.9, 0.9, 1, 0, 2.9, 1

// initial nodes: 505647
// final best nodes: 485726
// final best params: 15.1, 12, 1.7, 2.3, 2, 1, 0.4, 0.8, 4.5, 0.5

// initial nodes: 485839
// final best nodes: 482332
// final best params: 14.9, 11.9, 1.6, 2.8, 1.9, 1, 0.4, 0.8, 4.5, 0.4,