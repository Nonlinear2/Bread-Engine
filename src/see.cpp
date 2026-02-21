#include "see.hpp"

bool SEE::evaluate(const Board& board, Move move, int threshold){ // return true if greater than threshold
    assert(SEE_KING_VALUE > piece_value[static_cast<int>(PieceType::QUEEN)] * 10);

    if (move.typeOf() != Move::NORMAL)
        return threshold <= 0;

    const Color attacker_color = board.sideToMove();
    const Square to_sq = move.to();

    PieceType next_piece = board.at(move.from()).type();
    bool attacker_turn = true;

    // number of pawns in the favor of the initial capturing side, after the move is made, it is the value of to_piece
    int balance = 0;
    int value_on_square = 0;

    Bitboard occupied = board.occ().clear(move.from().index());

    if (board.at(to_sq) != Piece::NONE)
        value_on_square = piece_value[board.at(to_sq).type()];

    if (value_on_square < threshold)
        return false;

    if (threshold <= value_on_square - ((next_piece == PieceType::KING) ? SEE_KING_VALUE : piece_value[next_piece]))
        return true;

    Bitboard queens = board.pieces(PieceType::QUEEN);
    Bitboard bishops_and_queens = board.pieces(PieceType::BISHOP) | queens;
    Bitboard rooks_and_queens = board.pieces(PieceType::ROOK) | queens;

    std::array<Bitboard, 2> attackers = {
        attacks::attackers(board, ~attacker_color, to_sq),
        attacks::attackers(board, attacker_color, to_sq)
    };

    do {
        // check if one side is up material after the move
        balance += (attacker_turn ? 1 : -1) * value_on_square;

        // if the side to move after the capture is up material, they don't need to capture again.
        if (!attacker_turn && balance >= threshold)
            return true;
        if (attacker_turn && balance < threshold)
            return false;

        // make capture

        // add xray pieces to the attackers
        if (next_piece == PieceType::PAWN || next_piece == PieceType::BISHOP || next_piece == PieceType::QUEEN)
            attackers[attacker_turn] |= attacks::bishop(to_sq, occupied) & bishops_and_queens;

        if (next_piece == PieceType::ROOK || next_piece == PieceType::QUEEN)
            attackers[attacker_turn] |= attacks::rook(to_sq, occupied) & rooks_and_queens;

        attackers[attacker_turn] &= occupied;

        attacker_turn = !attacker_turn;

        value_on_square = (next_piece == PieceType::KING) ? SEE_KING_VALUE : piece_value[next_piece];

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
    for (PieceType piece_type : {
            PieceType::PAWN, PieceType::KNIGHT,
            PieceType::BISHOP, PieceType::ROOK,
            PieceType::QUEEN, PieceType::KING
        }){
        Bitboard attacking_pieces_of_type = board.pieces(piece_type, color) & attackers;
        if (attacking_pieces_of_type){
            occupied.clear(attacking_pieces_of_type.lsb());
            return piece_type;
        }
    }
    return PieceType::NONE;
}