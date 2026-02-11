#pragma once

#include <vector>
#include <string>
#include <iostream>
#include "chess.hpp"
#include "tbprobe.hpp"
#include "constants.hpp"
#include "nnue_board.hpp"
#include "piece_square_tables.hpp"
#include "misc.hpp"

namespace Nonsense {

enum Stage {
    STANDARD, TAKE_PIECES, PROMOTE, CHECKMATE,
};

static const std::vector<int> nonsense_piece_value = {
    200, // pawn
    1500, // knight
    1000, // bishop
    0, // rook
    0, // queen
    0,  // king
    0, // none
};

static constexpr int rick_astley_odds = 5;
static constexpr int num_main_lyrics = 6;

static inline std::vector<std::string> rick_astley_lyrics = {
    "Never gonna give you up",
    "Never gonna let you down",
    "Never gonna run around and desert you",
    "Never gonna make you cry",
    "Never gonna say goodbye",
    "Never gonna tell a lie and hurt you",
    "We've known each other for so long",
    "We're no strangers to love",
    "You know the rules and so do I (do I)",
    "A full commitment's what I'm thinking of",
    "You wouldn't get this from any other guy",
    "I just wanna tell you how I'm feeling",
    "Gotta make you understand",
    "Your heart's been aching, but you're too shy to say it (say it)",
    "Inside, we both know what's been going on (going on)",
    "We know the game and we're gonna play it",
    "And if you ask me how I'm feeling",
    "Don't tell me you're too blind to see",
};

void display_info();
Move play_bongcloud(const Board& pos);

bool is_winning_side(Board& pos);
bool enough_material_for_nonsense(Board& pos);

int material_evaluate(Board& pos);
int evaluate(NnueBoard& pos);
bool only_knight_bishop(NnueBoard& pos);

} // namespace Nonsense