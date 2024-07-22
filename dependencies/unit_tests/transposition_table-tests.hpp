#pragma once

#include "doctest.h"
#include "transposition_table.hpp"

TEST_SUITE("Transposition Table"){
    TEST_CASE("store/retrieve"){
        TranspositionTable tt = TranspositionTable();
        tt.allocateKB(10);
        tt.info();
        chess::Move best = chess::Move::make(chess::Square(chess::Square::underlying::SQ_E2),
                                             chess::Square(chess::Square::underlying::SQ_E4),
                                             chess::PieceType::PAWN);
        for (int zobrist = 0; zobrist < 1'000; zobrist++){
            tt.store(zobrist, zobrist/1'000, zobrist, best, TFlag::EXACT, 10);
        }

        bool is_hit;
        for (int zobrist = 0; zobrist < 1'000; zobrist++){
            TEntry* transposition = tt.probe(is_hit, zobrist);
            if (is_hit){
                CHECK(transposition->best_move == best);
                CHECK(transposition->evaluation == zobrist/1'000);
                CHECK(transposition->depth == static_cast<uint8_t>(zobrist));
            }
        }
    }
}