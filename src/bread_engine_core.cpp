#include "bread_engine_core.hpp"

//TODO sparse matrix multiplication
//TODO fix crashes
//TODO test asp search again

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

template<chess::movegen::MoveGenType MoveGenType>
std::array<CircularBuffer3, 25> Engine::SortedMoveGen<MoveGenType>::killer_moves = {};

template<chess::movegen::MoveGenType MoveGenType>
Engine::SortedMoveGen<MoveGenType>::SortedMoveGen(NnueBoard& board): board(board) {};

template<chess::movegen::MoveGenType MoveGenType>
void Engine::SortedMoveGen<MoveGenType>::generate_moves(){
    chess::movegen::legalmoves<MoveGenType>(legal_moves, board);
}

// set move score to be sorted later
template <>
void Engine::SortedMoveGen<chess::movegen::MoveGenType::ALL>::set_score(chess::Move& move, int depth){
    
    const chess::Square from = move.from();
    const chess::Square to = move.to();
    const chess::Piece piece = board.at(from);
    const chess::Piece to_piece = board.at(to);
    float score = 0;

    if ((piece != chess::Piece::WHITEKING) && (piece != chess::Piece::BLACKKING)){
        score += psm.get_psm(piece, from, to);
    } else {
        bool is_endgame = board.occ().count() <= ENDGAME_PIECE_COUNT;
        score += psm.get_ksm(piece, is_endgame, to, from);
    }
    
    // captures should be searched early, so
    // to_value = piece_value(to) - piece_value(from) doesn't seem to work.
    // however, find a way to make these captures even better ?
    if (to_piece != chess::Piece::NONE){
        score += piece_value[static_cast<int>(to_piece.type())] * MATERIAL_CHANGE_MULTIPLIER;
    }

    if (move.typeOf() == chess::Move::PROMOTION){
        score += piece_value[static_cast<int>(move.promotionType())] * MATERIAL_CHANGE_MULTIPLIER;
    }
    
    if (killer_moves[depth].in_buffer(move)){
        score += KILLER_SCORE;
    }
    move.setScore(score);
}

template <>
void Engine::SortedMoveGen<chess::movegen::MoveGenType::CAPTURE>::set_score(chess::Move& move){
    move.setScore(piece_value[static_cast<int>(board.at(move.to()).type())] - 
                  piece_value[static_cast<int>(board.at(move.from()).type())]);
}

template <>
void Engine::SortedMoveGen<chess::movegen::MoveGenType::ALL>::set_scores(int depth, chess::Move tt_move){
    for (auto& move: legal_moves){
        set_score(move, depth);
    }

    // makes sure this move is searched first, regardless of depth of search.
    if (tt_move != NO_MOVE){
        auto move = std::find(legal_moves.begin(), legal_moves.end(), tt_move);
        // if there is a zobrist key collision, move might not be legal
        if (move != legal_moves.end()) {
            move->setScore(BEST_MOVE_SCORE);
        }
    }
}

template <>
void Engine::SortedMoveGen<chess::movegen::MoveGenType::CAPTURE>::set_scores(){
    for (auto& move: legal_moves){
        set_score(move);
    }
}

// legal_moves must not be empty
template<chess::movegen::MoveGenType MoveGenType>
bool Engine::SortedMoveGen<MoveGenType>::next(chess::Move& move){
    // to implement element removal from a movelist object,
    // the movelist is split into an unseen part first, and a seen part.
    move_idx++;
    int move_list_size = legal_moves.size()-move_idx;
    if (move_list_size == 0){
        return false;
    }
    float score;
    int best_move_idx;
    float best_score = WORST_MOVE_SCORE;
    for (int i = 0; i < move_list_size; i++){
        score = legal_moves[i].score();
        if (score > best_score){
            best_score = score;
            best_move_idx = i;
        }
    }
    // pop best_move from move_list
    if (best_move_idx != move_list_size-1){
        chess::Move swap = legal_moves[best_move_idx];
        legal_moves[best_move_idx] = legal_moves[move_list_size-1];
        legal_moves[move_list_size-1] = swap;
    }

    move = legal_moves[move_list_size-1];
    return true;
}

template<chess::movegen::MoveGenType MoveGenType>
bool Engine::SortedMoveGen<MoveGenType>::is_empty(){return legal_moves.empty(); }

template<chess::movegen::MoveGenType MoveGenType>
inline int Engine::SortedMoveGen<MoveGenType>::index(){return move_idx; }

template <>
void Engine::SortedMoveGen<chess::movegen::MoveGenType::ALL>::clear_killer_moves(){
    std::fill(killer_moves.begin(), killer_moves.end(), CircularBuffer3());
}

bool Engine::try_outcome_eval(float& eval){
    // chess::GameResult outcome = inner_board.isGameOver().second;
    
    // we dont want history dependent data to be stored in the TT.
    // the evaluation stored in the TT should only depend on the
    // position on the board and not how we got there, otherwise these evals would be reused
    // incorrectly.
    // here, threefold repetition is already handled and resulting evals
    // are not stored in the TT.
    // the fifty move rule is not yet supported
    if (inner_board.isInsufficientMaterial()){
        eval = 0;
        return true;
    }

    chess::Movelist movelist;
    chess::movegen::legalmoves(movelist, inner_board);

    if (movelist.empty()){
        // checkmate/stalemate.
        eval = inner_board.inCheck() ? -1 : 0;
        return true;
    }
    return false;
}

std::pair<chess::Move, TFlag> Engine::minimax_root(int depth, int color, float alpha = WORST_EVAL, float beta = BEST_EVAL){
    chess::Move best_move;
    const float initial_alpha = alpha;
    uint64_t zobrist_hash = inner_board.hash();
    inner_board.synchronize();

    chess::Move tt_move = NO_MOVE;
    bool is_hit;
    TEntry* transposition = transposition_table.probe(is_hit, zobrist_hash);
    if (is_hit){
        if ((transposition->depth() >= depth) && (!inner_board.isRepetition(1)) && (transposition->flag() == TFlag::EXACT)){
            best_move = transposition->best_move;
            best_move.setScore(transposition->evaluation);
            return {best_move, TFlag::EXACT};
        }

        if (transposition->best_move != NO_MOVE) tt_move = transposition->best_move;
    }

    SortedMoveGen sorted_move_gen = SortedMoveGen<chess::movegen::MoveGenType::ALL>(inner_board);
    sorted_move_gen.generate_moves();
    sorted_move_gen.set_scores(depth, tt_move);
    
    float pos_eval;
    float max_eval = WORST_EVAL;
    chess::Move move;
    while(sorted_move_gen.next(move)){

        inner_board.update_state(move);
        
        pos_eval = -negamax(depth-1, -color, -beta, -alpha);
        
        move.setScore(pos_eval);

        if (pos_eval > max_eval){
            best_move = move;
            max_eval = pos_eval;
        }
        inner_board.restore_state(move);

        alpha = std::max(alpha, pos_eval);
        if (beta <= alpha){
            break;
        }
    }

    TFlag node_type;
    if (beta <= alpha){ // this means there was a cutoff. So true eval is equal or higher
        node_type = TFlag::LOWER_BOUND;
    } else if (max_eval <= initial_alpha){ 
        // this means that the evals were all lower than best option for us
        // so it is possible there was a beta cutoff. So eval is equal or lower
        node_type = TFlag::UPPER_BOUND;
    } else {
        // eval is between initial alpha and beta. so search was completed without cutoffs, and is exact
        node_type = TFlag::EXACT;
    }

    transposition_table.store(zobrist_hash, max_eval, depth, best_move, node_type, static_cast<uint8_t>(inner_board.fullMoveNumber()));

    return {best_move, node_type};
}

float Engine::get_think_time(float time_left, int num_moves_out_of_book, int num_moves_until_time_control=0, int increment=0){
    float target;
    float move_num = num_moves_out_of_book < 10 ? static_cast<float>(num_moves_out_of_book) : 10;
    float factor = 2 -  move_num / 10;
    if (num_moves_until_time_control == 0){
        target = time_left / 30;
    } else {
        target = time_left / (num_moves_until_time_control+5);
    }
    return factor*target + 0.9F*increment;
}

std::pair<std::string, std::string> Engine::get_pv_pmove(std::string fen){
    std::string pv = "";
    std::string ponder_move = "";
    chess::Board pv_visitor;

    pv_visitor.setFen(fen);

    for (int i = 0; i < search_depth; i++){
        bool is_hit;
        TEntry* transposition = transposition_table.probe(is_hit, pv_visitor.hash());
        if ((!is_hit) || (transposition->best_move == NO_MOVE)){
            break;
        }
        if (i == 1){
            ponder_move = chess::uci::moveToUci(transposition->best_move);
        }
        pv += " " + chess::uci::moveToUci(transposition->best_move);
        pv_visitor.makeMove(transposition->best_move);
    }
    return std::pair(pv, ponder_move);
}

chess::Move Engine::search(std::string fen, int time_limit, int min_depth, int max_depth){
    inner_board.setFen(fen);
    return iterative_deepening(time_limit, min_depth, max_depth);
};

chess::Move Engine::search(int time_limit, int min_depth, int max_depth){
    return iterative_deepening(time_limit, min_depth, max_depth);
};

chess::Move Engine::iterative_deepening(int time_limit, int min_depth=4, int max_depth=4){
    std::string initial_fen = inner_board.getFen();

    std::string pv;
    std::string ponder_move = "";

    chess::Move best_move;

    this->time_limit = time_limit;
    start_time = now();
    this->min_depth = min_depth;
    this->max_depth = max_depth;

    SortedMoveGen<chess::movegen::MoveGenType::ALL>::clear_killer_moves();

    nodes = 0;

    int current_depth = 1;

    engine_color = (inner_board.sideToMove() == chess::Color::WHITE) ? 1: -1;

    chess::Move tb_move;
    if (inner_board.probe_dtz(tb_move)){
        std::cout << "bestmove " << chess::uci::moveToUci(tb_move) << std::endl;
        return tb_move;
    };

    while (true){
        search_depth = current_depth;
        try {
            best_move = minimax_root(search_depth, engine_color).first;
        } catch (TerminateSearch& e){
            break;
        }

        std::pair<std::string, std::string> pv_pmove = get_pv_pmove(initial_fen);
        pv = pv_pmove.first;
        if (pv_pmove.second.size() > 0){
            ponder_move = pv_pmove.second;
        }

        std::cout << "info depth " << current_depth << " nodes " << nodes << " nps " << static_cast<int>(nodes/run_time*1000) << std::endl;
        std::cout << "info hashfull " << transposition_table.hashfull() << std::endl;
        std::cout << "info pv" << pv << " score cp " << static_cast<int>(best_move.score()*1111) << std::endl;

        // should the search really stop if there is a mate for the oponent?
        if ((best_move.score() >= 1) || // checkmate
            (best_move.score() <= -1) || // checkmate
            (current_depth >= max_depth) ||
            (current_depth >= ENGINE_MAX_DEPTH)) break;
        
        current_depth++;
    }
    std::cout << "info time " << static_cast<int>(run_time) << std::endl;

    std::cout << "bestmove " << chess::uci::moveToUci(best_move);
    if (ponder_move.size() > 0){
        std::cout << " ponder " << ponder_move;
    }
    std::cout << std::endl;
    return best_move;
}

bool Engine::can_return(){
    if (interrupt_flag){
        return true;
    }
    update_run_time();
    return (search_depth > min_depth) && (run_time > time_limit);
}

void Engine::set_interrupt_flag(){
    interrupt_flag = true;
}

void Engine::unset_interrupt_flag(){
    interrupt_flag = false;
}

void Engine::update_run_time(){
    run_time = std::chrono::duration<float, std::milli>(now() - start_time).count();
};

float Engine::negamax(int depth, int color, float alpha, float beta){
    nodes++;

    if (inner_board.isRepetition(2)){ // there are no such checks in qsearch as captures can never lead to repetition.
        return 0;
    }

    // transpositions will be checked inside of qsearch
    // if isRepetition(1), qsearch will not consider the danger of draw as it searches captures.
    if (depth == 0){
        // we check can_return only at depth 0 to avoid doing it at all nodes
        if (can_return()){
            throw TerminateSearch();
        }
        return qsearch(alpha, beta, color, QSEARCH_MAX_DEPTH);
    }

    const float initial_alpha = alpha;
    uint64_t zobrist_hash = inner_board.hash();

    chess::Move tt_move = NO_MOVE;
    bool is_hit;
    TEntry* transposition = transposition_table.probe(is_hit, zobrist_hash);
    if (is_hit){
        if ((transposition->depth() >= depth) && (!inner_board.isRepetition(1))){
            // if is repetition(1), danger of repetition so TT is unreliable
            switch (transposition->flag()){
                case TFlag::EXACT:
                    return transposition->evaluation;
                case TFlag::LOWER_BOUND:
                    alpha = std::max(alpha, transposition->evaluation);
                    break;
                case TFlag::UPPER_BOUND:
                    beta = std::min(beta, transposition->evaluation);
                    break;
                default:
                    break;
            }
            // we need to do a cutoff check because we updated alpha/beta.
            if (beta <= alpha){
                // no need to store in transposition table as is already there.
                return transposition->evaluation;
            }
        }

        if (transposition->best_move != NO_MOVE){
            tt_move = transposition->best_move;
        }
    }
    
    SortedMoveGen sorted_move_gen = SortedMoveGen<chess::movegen::MoveGenType::ALL>(inner_board);
    sorted_move_gen.generate_moves();

    float max_eval = WORST_EVAL;

    if (sorted_move_gen.is_empty()){ // avoid calling expensive try_outcome_eval function
        // If board is in check, it is checkmate
        // to make the engine prefer faster checkmates instead of stalling,
        // we decrease the eval if the checkmate is higher in the search tree.
        // if there are no legal moves and it's not check, it is stalemate so eval is 0
        max_eval = inner_board.inCheck() ? -(1+depth) : 0;
        // we know this eval is exact at any depth, but 
        // we also don't want this eval to pollute the transposition table.
        // the full move number will make sure it is replaced at some point.
        transposition_table.store(zobrist_hash, max_eval, 255, NO_MOVE, TFlag::EXACT, static_cast<uint8_t>(inner_board.fullMoveNumber()));
        return max_eval;
    }

    sorted_move_gen.set_scores(depth, tt_move);

    bool lmr = false;
    chess::Move best_move;
    chess::Move move;
    float pos_eval;
    int new_depth;
    while (sorted_move_gen.next(move)){
        bool is_capture = inner_board.isCapture(move);
        inner_board.update_state(move);

        // depth > 2 is to make sure the new depth is not less than 0.
        // (!inner_board.inCheck()) is to see if the move gives check. 
        // (we already updated the inner board so we only need to check if it is check)
        if ((sorted_move_gen.index() > 3) && (depth > 2) && (!is_capture) && (!inner_board.inCheck())){
            new_depth = depth-2;
            lmr = true;
        } else {
            new_depth = depth-1;
            lmr = false;
        }

        pos_eval = -negamax(new_depth, -color, -beta, -alpha);

        if (lmr && (pos_eval > alpha)){
            // do full search
            pos_eval = -negamax(depth-1, -color, -beta, -alpha);
        }

        inner_board.restore_state(move);
        
        if (pos_eval > max_eval){
            max_eval = pos_eval;
            best_move = move;
        }

        alpha = std::max(alpha, pos_eval);
        if (beta <= alpha){
            SortedMoveGen<chess::movegen::MoveGenType::ALL>::killer_moves[depth].add_move(move);
            break;
        }
    }

    // if max eval was obtained because of a threefold repetition,
    // the eval should not be stored in the transposition table, as
    // this is a history dependent evaluation.
    // what if max eval was not obtained because of a threefold? in this case we loose
    // one TT entry which is completely fine.
    // what about evaluations higher in the tree where inner_board.isRepetition(1) is false?
    // these evals are not history dependent as they mean that one side can force a draw.
    if ((max_eval == 0) && (inner_board.isRepetition(1))){
        return max_eval;
        // we fall through without storing the eval in the TT.
    };

    TFlag node_type;
    if (beta <= alpha){ // this means there was a cutoff. So true eval is equal or higher
        node_type = TFlag::LOWER_BOUND;
    } else if (max_eval <= initial_alpha){ 
        // this means that the evals were all lower than best option for us
        // so it is possible there was a beta cutoff. So eval is equal or lower
        node_type = TFlag::UPPER_BOUND;
    } else {
        // eval is between initial alpha and beta. so search was completed without cutoffs, and is exact
        node_type = TFlag::EXACT;
    }
    transposition_table.store(zobrist_hash, max_eval, depth, best_move, node_type, static_cast<uint8_t>(inner_board.fullMoveNumber()));

    return max_eval;
}

float Engine::qsearch(float alpha, float beta, int color, int depth){
    nodes++;

    // tablebase probe
    float wdl_eval;
    if (inner_board.probe_wdl(wdl_eval)){
        return wdl_eval;
    }

    // this is recomputed when qsearch is called the first time. Performance loss is probably low. 
    uint64_t zobrist_hash = inner_board.hash();

    float stand_pat;

    // first we check for transposition. If it is outcome, it should have already been stored
    // with an exact flag, so the stand pat will be correct anyways.
    bool is_hit;
    TEntry* transposition = transposition_table.probe(is_hit, zobrist_hash);
    if (is_hit){
        switch (transposition->flag()){
            case TFlag::EXACT:
                if (transposition->depth() == 0){ // outcomes are stored at depth 255
                    stand_pat = transposition->evaluation;
                    // we can already check for cutoff
                    if (stand_pat >= beta) return stand_pat;
                } else {
                    return transposition->evaluation;
                }
                break;
            case TFlag::LOWER_BOUND:
                if (depth == QSEARCH_MAX_DEPTH){
                    alpha = std::max(alpha, transposition->evaluation);
                    if (beta <= alpha) return transposition->evaluation;
                    is_hit = false;
                }
                break;
            case TFlag::UPPER_BOUND:
                if (depth == QSEARCH_MAX_DEPTH){
                    beta = std::min(beta, transposition->evaluation);
                    if (beta <= alpha) return transposition->evaluation;
                    is_hit = false;
                }
                break;
            default:
                break;
        }
    }

    SortedMoveGen sorted_capture_gen = SortedMoveGen<chess::movegen::MoveGenType::CAPTURE>(inner_board);

    if (depth != 0){
        sorted_capture_gen.generate_moves();
    }

    if (!is_hit){
        // if it is hit, no need to check for outcome, as it wouldn't be stored in the
        // transposition table at a 0 depth. 
        float outcome_eval;
        if (sorted_capture_gen.is_empty() && try_outcome_eval(outcome_eval)){ // only generate non captures?
            transposition_table.store(zobrist_hash, outcome_eval, 255, NO_MOVE, TFlag::EXACT, static_cast<uint8_t>(inner_board.fullMoveNumber()));
            return outcome_eval;
        }

        stand_pat = inner_board.evaluate();
        transposition_table.store(zobrist_hash, stand_pat, 0, NO_MOVE, TFlag::EXACT, static_cast<uint8_t>(inner_board.fullMoveNumber()));
        if (stand_pat >= beta) {
            return stand_pat;
        }
    }   

    if (depth == 0){
        return stand_pat;
    }

    sorted_capture_gen.set_scores();

    alpha = std::max(alpha, stand_pat);

    float max_eval = stand_pat;
    float pos_eval;
    chess::Move move;
    while (sorted_capture_gen.next(move)){
        // delta pruning
        // move.score() is calculated with set_capture_score which is material difference.
        // 0.8 is the equivalent of a queen, as a safety margin
        if (stand_pat + move.score()/10 + 0.8 < alpha){ // division by 10 is to convert from pawn to "engine" centipawns.
            continue;
        }

        inner_board.update_state(move);
        pos_eval = -qsearch(-beta, -alpha, -color, depth-1);
        inner_board.restore_state(move);

        if (pos_eval > max_eval){
            max_eval = pos_eval;
        }
        alpha = std::max(alpha, pos_eval);
        if (alpha >= beta){ // only check for cutoffs when alpha gets updated.
            return max_eval;
        }
    }
    return max_eval;
}