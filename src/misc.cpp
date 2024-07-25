#include "misc.hpp"

// This is a circular buffer to implement FIFO for killer moves
void CircularBuffer3::add_move(chess::Move move){
    if (data[curr_idx] != move.move()) data[curr_idx] = move.move(); // avoid storing the same move multiple times.
    // data[curr_idx] = move.move();
    curr_idx++;
    curr_idx %= 3;
}

bool CircularBuffer3::in_buffer(chess::Move move){
    uint16_t move_val = move.move();
    return (data[0] == move_val) || (data[1] == move_val) || (data[2] == move_val);
}

void History::clear(){
    std::fill(std::begin(history[0]), std::end(history[0]), 0);
    std::fill(std::begin(history[1]), std::end(history[1]), 0);
}

int History::get_history_bonus(int from, int to, bool color){
    return history[color][from*64 + to];
}