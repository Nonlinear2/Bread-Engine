#pragma once

#include <vector>
#include <string>
#include <iostream>
#include "chess.hpp"
#include "tbprobe.hpp"
#include "misc.hpp"

class Nonsense {
    public:
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
    bool should_bongcloud(uint64_t hash, int move_number);
    void play_bongcloud();
    void play_worst_winning_move(chess::Move move, chess::Movelist moves);
    private:
    static constexpr uint64_t starting_pos_hash = 5060803636482931868;
    bool is_bongcloud = false;
};

