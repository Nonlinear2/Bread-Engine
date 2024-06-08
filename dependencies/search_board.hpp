#pragma once

#include "chess.hpp"
#include "piece_square_tables.hpp"
#include "tbprobe.hpp"

#include <string>

class SearchBoard: public chess::Board {
    public:

    virtual void synchronize() = 0;

    virtual void update_state(chess::Move move) = 0;

    virtual void restore_state(chess::Move move) = 0;

    virtual bool is_basic_move(chess::Move move) = 0;
    
    bool probe_wdl(float& eval);

    bool probe_dtz(chess::Move& move);

    virtual float evaluate() = 0;

};