#include "misc.hpp"

// This is a circular buffer to implement FIFO for killer moves
void CircularBuffer3::add_move(Move move){
    if (data[curr_idx] != move.move())
        data[curr_idx] = move.move(); // avoid storing the same move multiple times.
    curr_idx++;
    curr_idx %= 3;
}

bool CircularBuffer3::in_buffer(Move move){
    uint16_t move_val = move.move();
    return data[0] == move_val || data[1] == move_val || data[2] == move_val;
}

void History::clear(){
    std::fill(std::begin(history[0]), std::end(history[0]), 0);
    std::fill(std::begin(history[1]), std::end(history[1]), 0);
}

int History::get_history_bonus(int from, int to, bool color){
    return history[color][from*64 + to];
}

bool SEE::evaluate(const Board& board, Move move, int threshold){ // return true if greater than threshold
    assert(SEE_KING_VALUE > piece_value[static_cast<int>(PieceType::QUEEN)]*10);
    const Square from_sq = move.from();
    const Square to_sq = move.to();
    PieceType next_piece = board.at(from_sq).type();
    Color attacker_color = board.at(from_sq).color();
    bool attacker_turn = true;

    // number of pawns in the favor of the initial capturing side, after the move is made, it is the value of to_piece
    int balance = 0;
    int value_on_square = 0;

    Bitboard queens = board.pieces(PieceType::QUEEN);
    Bitboard bishops_and_queens = board.pieces(PieceType::BISHOP) | queens;
    Bitboard rooks_and_queens = board.pieces(PieceType::ROOK) | queens;

    std::array<Bitboard, 2> attackers = {
        attacks::attackers(board, ~attacker_color, to_sq),
        attacks::attackers(board, attacker_color, to_sq)
    };
    
    Bitboard occupied = board.occ().clear(from_sq.index());

    switch (move.typeOf()){
        case Move::ENPASSANT:
            value_on_square = 1;
            occupied.clear(to_sq.ep_square().index());
            // update attacks
            attackers[attacker_turn] |= attacks::rook(to_sq, occupied) & rooks_and_queens;
            break;
        case Move::PROMOTION:
            next_piece = move.promotionType();
            balance += piece_value[static_cast<int>(next_piece)] - piece_value[static_cast<int>(PieceType::PAWN)];
            break;
        default:
            break;
    }
    if (board.at(to_sq) != Piece::NONE)
        value_on_square = piece_value[static_cast<int>(board.at(to_sq).type())];

    do {
        // make capture

        // add xray pieces to the attackers
        if (next_piece == PieceType::PAWN || next_piece == PieceType::BISHOP || next_piece == PieceType::QUEEN)
            attackers[attacker_turn] |= attacks::bishop(to_sq, occupied) & bishops_and_queens;

        if (next_piece == PieceType::ROOK || next_piece == PieceType::QUEEN)
            attackers[attacker_turn] |= attacks::rook(to_sq, occupied) & rooks_and_queens;

        attackers[attacker_turn] &= occupied;

        balance += (attacker_turn ? 1 : -1) * value_on_square;

        value_on_square = (next_piece == PieceType::KING) ? SEE_KING_VALUE : piece_value[static_cast<int>(next_piece)];
        attacker_turn = !attacker_turn;

        // if the side to move is up material, they don't need to capture again.
        if (attacker_turn && balance >= threshold)
            return true;
        if (!attacker_turn && balance < threshold)
            return false;
        // prepare next piece
        next_piece = pop_lva(
            board,
            occupied,
            attackers[attacker_turn],
            attacker_turn ? attacker_color : ~attacker_color
        );
    } while (next_piece != PieceType::NONE);

    return (balance >= threshold);
}

// get the value of the least valuable attacker and pop the square on the occupancy bitboard
PieceType SEE::pop_lva(const Board& board, Bitboard& occupied, const Bitboard& attackers, Color color){
    for (int piece_idx = 0; piece_idx < 6; piece_idx++){
        PieceType piece_type = static_cast<PieceType::underlying>(piece_idx);
        Bitboard attacking_pieces_of_type = board.pieces(piece_type, color) & attackers;
        if (attacking_pieces_of_type) {
            occupied.clear(attacking_pieces_of_type.lsb());
            return piece_type;
        }
    }
    return PieceType::NONE;
}