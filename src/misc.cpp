#include "misc.hpp"

// This is a circular buffer to implement FIFO for killer moves
void CircularBuffer3::add_move(chess::Move move){
    data[curr_idx] = move.move();
    curr_idx++;
    curr_idx %= 3;
}

bool CircularBuffer3::in_buffer(chess::Move move){
    uint16_t move_val = move.move();
    return (data[0] == move_val) || (data[1] == move_val) || (data[2] == move_val);
}