#pragma once

#include "doctest.h"
#include "nnue_board.hpp"

TEST_SUITE("Nnue Board"){
    TEST_CASE("get_hkp"){
        NnueBoard cb = NnueBoard();

        std::vector<std::string> fens = {
            "r2q1rk1/pp1bbppp/2pp1nn1/3Pp3/2P1P3/2NB1N1P/PP3PP1/R1BQR1K1 w - - 2 11",
            "r2q1rk1/pp2bpp1/2pp2n1/3Pp3/P1P1P1Qp/2NB4/1P6/R1B1R1K1 b - - 0 18",
            "6k1/7p/6p1/8/p1r5/P2RK3/6PP/8 w - - 0 38",
            "3k3r/p1r2p2/1qpbpPpp/3p2P1/1PbP4/B1P1QN2/5P1P/R3K2R b KQ - 2 24",
            "R3Rbk1/5q2/4p1p1/2pp4/8/8/4KP2/8 w - - 0 51",
        };

        std::vector<std::vector<int>> correct_w = {
            {39680, 39685, 39800, 39804, 39829, 39830, 39914, 39917, 39947, 39948, 40043, 40058, 40067, 40187, 40200, 40201, 40205, 40206,40207, 40210, 40211, 40220, 40283, 40290, 40292, 40303, 40304,40305, 40309, 40310},
            {39680, 39685, 39800, 39804, 39830, 39914, 39948, 40043, 40058,40067, 40166, 40200, 40201, 40205, 40206, 40210, 40211, 40220, 40231, 40283, 40288, 40290, 40292, 40305},
            {28194, 28267, 28687, 28694, 28704, 28776, 28790, 28791},
            {38407, 38410, 38520, 38527, 38637, 38675, 38690, 38760, 38801,38892, 38920, 38925, 38930, 38932, 38934, 38935, 38939, 38997, 39006, 39009, 39011, 39018, 39029, 39031},
            {33344, 33348, 33541, 33677, 33812, 33814, 33818, 33819, 33909},
        };

        std::vector<std::vector<int>> correct_b = {
            {36483, 36487, 36602, 36607, 36626, 36629, 36713, 36714, 36741, 36756, 36851, 36852, 36868, 36988, 37001, 37002, 37006, 37007, 37008, 37019, 37021, 37028, 37091, 37100, 37101, 37104, 37105, 37106, 37110, 37111},
            {36483, 36487, 36602, 36607, 36629, 36713, 36741, 36756, 36851,36889, 36988, 37006, 37019, 37021, 37023, 37028, 37080, 37091, 37100, 37101, 37105, 37106, 37110, 37111},
            {36500, 36573, 37000, 37001, 37015, 37087, 37097, 37104},
            {38400, 38407, 38517, 38520, 38546, 38679, 38749, 38764, 38803, 38894, 38920, 38922, 38933, 38940, 38942, 38945, 38954, 39012, 39016, 39017, 39019, 39021, 39026, 39031},
            {36539, 36543, 36858, 36978, 37002, 37092, 37093, 37097, 37099},
        };

        std::vector<int> computed;
        for (int i = 0; i<fens.size(); i++){
            cb.setFen(fens[i]);

            // white perspective
            computed = cb.get_HKP(Color::WHITE);
            std::sort(computed.begin(), computed.end());
            CHECK(computed == correct_w[i]);

            // black perspective
            computed = cb.get_HKP(Color::BLACK);
            std::sort(computed.begin(), computed.end());
            CHECK(computed == correct_b[i]);
        }
    }

    TEST_CASE("synchronize/compute_accumulator"){
        NnueBoard cb = NnueBoard();
        std::vector<std::string> fens = {
            "r2q1rk1/pp1bbpp1/2pp1nn1/3Pp3/P1P1P1Pp/2NB1P2/1P6/R1BQR1K1 b - - 0 16",
            "r2q1rk1/pp2bpp1/2pp2n1/3Pp3/P1P1P1Qp/2NB4/1P6/R1B1R1K1 b - - 0 18",
            "6k1/7p/6p1/8/p1r5/P2RK3/6PP/8 w - - 0 38",
            "r2qk2r/pp1bbp1p/2n1pnp1/1B1p4/5P2/4BN2/PPP1N1PP/R2Q1RK1 w kq - 3 12",
            "3k3r/p1r2p2/1qpbpPpp/3p2P1/1PbP4/B1P1QN2/5P1P/R3K2R b KQ - 2 24",
        };
        Accumulator correct_acc;

        for (auto fen: fens){
            cb.synchronize();
            for (int i = 0; i < 2; i++){
                Color color = i ? Color::WHITE : Color::BLACK;
                std::vector<int> active_features = cb.get_HKP(color);

                // unoptimized but working way to compute accumulator:
                for (int i = 0; i < acc_size; i++){
                    correct_acc[color][i] = cb.nnue_.feature_transformer.bias[i];
                }
                
                for (const int &a: active_features){
                    for (int i = 0; i < acc_size; i++) {
                        correct_acc[color][i] += cb.nnue_.feature_transformer.weights[a*acc_size + i];
                    }
                }
            }

            for (int i = 0; i < 256; i++){
                CHECK(cb.nnue_.accumulator[true][i] == correct_acc[true][i]);
                CHECK(cb.nnue_.accumulator[false][i] == correct_acc[false][i]);
            }
        }
    }

    TEST_CASE("update_state"){
        std::cout << "update state\n";
        NnueBoard cb = NnueBoard();
        chess::Movelist legal_moves;
        int random_number;
        int positions = 0;

        Accumulator computed_acc;

        cb.setFen(chess::constants::STARTPOS);
        cb.synchronize();
        while (cb.isGameOver().second == chess::GameResult::NONE){
            chess::movegen::legalmoves(legal_moves, cb);
            random_number = std::rand()%legal_moves.size();
            cb.update_state(legal_moves[random_number]);

            computed_acc = cb.nnue_.accumulator;
            cb.synchronize();

            bool is_same = true;
            for (int i = 0; i<256; i++){
                is_same &= (computed_acc[true][i] == cb.nnue_.accumulator[true][i]);
                is_same &= (computed_acc[false][i] == cb.nnue_.accumulator[false][i]);
            }
            CHECK(is_same);

            positions++;
        }
        std::cout << "total positions: " << positions << "\n";
    }

    TEST_CASE("restore_state"){
        NnueBoard cb = NnueBoard();
        chess::Movelist legal_moves;
        int random_number;
        int positions = 0;

        Accumulator computed_acc;
        cb.setFen(chess::constants::STARTPOS);
        cb.synchronize();
        std::vector<chess::Move> moves;

        // make moves
        while (cb.isGameOver().second == chess::GameResult::NONE){
            chess::movegen::legalmoves(legal_moves, cb);
            random_number = std::rand()%legal_moves.size();
            moves.insert(moves.begin(), legal_moves[random_number]);
            cb.update_state(legal_moves[random_number]);
        }

        // restore
        cb.synchronize();
        for (auto move: moves){
            cb.restore_state(move);
            computed_acc = cb.nnue_.accumulator;
            cb.synchronize();
            
            bool is_same = true;
            for (int i = 0; i<256; i++){
                is_same &= (computed_acc[true][i] == cb.nnue_.accumulator[true][i]);
                is_same &= (computed_acc[false][i] == cb.nnue_.accumulator[false][i]);
            }
            CHECK(is_same);
            positions++;
        }

        std::cout << "total positions: " << positions << "\n";
    }
}