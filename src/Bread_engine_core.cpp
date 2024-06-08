#include "bread_engine_core.hpp"

// This is a circular buffer to implement FIFO for killer moves
void CircularBuffer3::add_move(chess::Move move){
    data[curr_idx] = move.move();
    curr_idx++;
    curr_idx %= 3;
}

bool CircularBuffer3::in_buffer(chess::Move move){
    return std::find(data.begin(), data.end(), move.move()) != data.end();
}

// set move score to be sorted later
void Engine::set_move_score(chess::Move& move, int depth){
    
    const chess::Square from = move.from();
    const chess::Square to = move.to();
    const chess::Piece piece = inner_board.at(from);
    const chess::Piece to_piece = inner_board.at(to);
    float score = 0;

    if ((piece != chess::Piece::WHITEKING) & (piece != chess::Piece::BLACKKING)){
        score += psm.get_psm(piece, from, to);
    } else {
        bool is_endgame = inner_board.occ().count() <= ENDGAME_PIECE_COUNT;
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

void Engine::set_capture_score(chess::Move& move){
    move.setScore(piece_value[static_cast<int>(inner_board.at(move.to()).type())] - 
                  piece_value[static_cast<int>(inner_board.at(move.from()).type())]);
}

// sort moves based on set_move_score. If a move exists in the tt, it is always placed first.
void Engine::order_moves(chess::Movelist& moves, uint64_t zobrist_hash, int depth){
    for (auto& move: moves){
        set_move_score(move, depth);
    }
    
    bool is_hit;
    TEntry* transposition = transposition_table.probe(is_hit, zobrist_hash);
    if (is_hit & transposition->best_move != NO_MOVE){
        moves[moves.find(transposition->best_move)].setScore(1000);
        // makes sure this move is searched first, regardless of depth of search.
    }
    // sort in descending order
    std::sort(moves.begin(), moves.end(), [](const chess::Move &a, const chess::Move &b) { return a.score() > b.score(); });
}

void Engine::order_captures(chess::Movelist& moves){
    for (auto& move: moves){
        set_capture_score(move);
    }
    // no check for transpositions. May be better or not.

    std::sort(moves.begin(), moves.end(), [](const chess::Move &a, const chess::Move &b) { return a.score() > b.score(); });
}

// should only be used when there are no legal moves on the board.
// It means there is either checkmate or stalemate on the board.
float Engine::get_outcome_eval(int depth){
    float eval;
    if (inner_board.inCheck()){ // this is checkmate
        // to make the engine prefer faster checkmates instead of stalling,
        // we decrease the eval if the checkmate is higher in the search tree.
        eval = -(1+depth); 
    } else { // if there are no legal moves and it's not check, it is stalemate
        eval = 0;
    }
    return eval;
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
        if (inner_board.inCheck()){ // checkmate
            eval = -1;
        } else { // stalemate
            eval = 0;
        }
        return true;
    }
    return false;
}

std::pair<chess::Move, TFlag> Engine::minimax_root(int depth, int color, float alpha = -100, float beta = 100){
    chess::Move best_move;
    const float initial_alpha = alpha;
    uint64_t zobrist_hash = inner_board.hash();
    inner_board.synchronize();

    bool is_hit;
    TEntry* transposition = transposition_table.probe(is_hit, zobrist_hash);
    if (is_hit & (transposition->depth >= depth) & (!inner_board.isRepetition(1))){
        switch (transposition->flag){
            case TFlag::EXACT:
                best_move = transposition->best_move;
                best_move.setScore(transposition->evaluation);
                return {best_move, TFlag::EXACT};
                break;
        }
    }

    chess::Movelist legal_moves;
    chess::movegen::legalmoves(legal_moves, inner_board);
    order_moves(legal_moves, zobrist_hash, depth);

    float pos_eval;
    float max_eval = -100;
    chess::Move move;
    for (int i = 0; i < legal_moves.size(); i++){
        move = legal_moves[i];
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
        if (!is_hit || transposition->best_move == NO_MOVE){
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

chess::Move Engine::search(std::string fen, int time_limit_, int min_depth_, int max_depth_){
    inner_board.setFen(fen);
    return iterative_deepening(time_limit_, min_depth_, max_depth_);
};

chess::Move Engine::search(int time_limit_, int min_depth_, int max_depth_){
    return iterative_deepening(time_limit_, min_depth_, max_depth_);
};

chess::Move Engine::iterative_deepening(int time_limit_, int min_depth_=4, int max_depth_=4){
    std::string initial_fen = inner_board.getFen();

    std::string pv;
    std::string ponder_move = "";

    chess::Move best_move;

    time_limit = time_limit_;
    start_time = now();
    min_depth = min_depth_;
    max_depth = max_depth_;

    std::fill(killer_moves.begin(), killer_moves.end(), CircularBuffer3());

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
            (current_depth >= engine_max_depth)){break;}
        
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
    
    if ((search_depth > min_depth) & (run_time > time_limit)){
        return true;
    }
    return false;
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
    if (can_return()){
        throw TerminateSearch();
    }

    if (inner_board.isRepetition(2)){ // there are no such checks in qsearch as captures can never lead to repetition.
        return 0;
    }

    const float initial_alpha = alpha;
    // if depth == 0, gets recomputed in qsearch but minor performance loss.
    uint64_t zobrist_hash = inner_board.hash();

    bool is_hit;
    TEntry* transposition = transposition_table.probe(is_hit, zobrist_hash);
    if (is_hit & (transposition->depth >= depth) & (!inner_board.isRepetition(1))){
        // if is repetition(1), danger of repetition so TT is unreliable
        switch (transposition->flag){
            case TFlag::EXACT:
                // avoid using static evaluation instead of quiescence search:
                if (transposition->depth == 0){
                    break;
                }
                return transposition->evaluation;
                break;
            case TFlag::LOWER_BOUND:
                alpha = std::max(alpha, transposition->evaluation);
                break;
            case TFlag::UPPER_BOUND:
                beta = std::min(beta, transposition->evaluation);
                break;
        }
    }

    if (depth == 0){
        return qsearch(alpha, beta, color, QSEARCH_MAX_DEPTH);
    }
    
    float max_eval = -100;

    chess::Move best_move;

    chess::Movelist legal_moves;
    chess::movegen::legalmoves(legal_moves, inner_board);


    if (legal_moves.empty()){ // avoid calling expensive try_outcome_eval function
        max_eval = get_outcome_eval(depth);

        // what depth to store is hard to choose, as we know this eval will be exact at any depth, but 
        // we also don't want this eval to pollute the transposition table. We want it replaced at some point.
        // this is why we stick to the "depth". 
        transposition_table.store(zobrist_hash, max_eval, depth, NO_MOVE, TFlag::EXACT, static_cast<uint8_t>(inner_board.fullMoveNumber()));
        return max_eval;
    }

    order_moves(legal_moves, zobrist_hash, depth);

    bool lmr = false;
    float pos_eval;
    chess::Move move;
    int new_depth = depth-1;
    for (int i = 0; i < legal_moves.size(); i++){
        move = legal_moves[i];

        inner_board.update_state(move);

        // depth > 2 is to make sure the new depth is not less than 0.
        // (!inner_board.inCheck()) is to see if the move gives check. 
        // (we already updated the inner board so we only need to check if it is check)
        if ((i > 3) & (depth > 2) & (inner_board.at(move.to()) == chess::Piece::NONE)
                    & (!inner_board.inCheck())){
            new_depth--;
            lmr = true;
        }

        pos_eval = -negamax(new_depth, -color, -beta, -alpha);

        if (lmr & (pos_eval > alpha)){
            // do full search at original depth - 1.
            pos_eval = -negamax(depth-1, -color, -beta, -alpha);
        }

        inner_board.restore_state(move);
        
        if (pos_eval > max_eval){
            max_eval = pos_eval;
            best_move = move;
        }

        alpha = std::max(alpha, pos_eval);
        if (beta <= alpha){
            killer_moves[depth].add_move(move);
            break;
        }
    }

    if ((max_eval == 0) & (inner_board.isRepetition(1))){
        // if max eval was obtained because of a threefold repetition,
        // the eval should not be stored in the transposition table, as
        // this is a history dependent evaluation.
        // what if max eval was not obtained because of a threefold? in this case we loose
        // one TT entry which is completely fine.
        // what about evaluations higher in the tree where inner_board.isRepetition(1) is false?
        // these evals are not history dependent as they mean that one side can force a draw.
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
        if (transposition->depth == 0){ // careful, we use the fact that outcomes are stored at depth 3 here.
            stand_pat = transposition->evaluation;

            // we can already check for cutoff
            if (stand_pat >= beta){
                // return stand_pat;
                return beta;
            }
        } else if (transposition->flag == TFlag::EXACT){
            // this means that it is either a higher depth search, in which case we can return,
            // as this is a better evaluation than the qsearch
            // or it can mean that it is outcome, in which case we can return too.
            return transposition->evaluation;
        } else {
            is_hit = false; // hit wasn't used, so later it should not be considered that there was a hit.
        }
    }

    chess::Movelist legal_captures;
    if (depth != 0){
        chess::movegen::legalmoves<chess::movegen::MoveGenType::CAPTURE>(legal_captures, inner_board);
    }

    if (!is_hit){
        // if it is hit, no need to check for outcome, as it wouldn't be stored in the
        // transposition table at a 0 depth. 
        if (legal_captures.empty()){
            float outcome_eval;
            if (try_outcome_eval(outcome_eval)){ // only generate non captures?
                transposition_table.store(zobrist_hash, outcome_eval, 3, NO_MOVE, TFlag::EXACT, static_cast<uint8_t>(inner_board.fullMoveNumber()));
                // higher depth stored because is exact at any depth.
                return outcome_eval;
            }
        }

        stand_pat = inner_board.evaluate();
        transposition_table.store(zobrist_hash, stand_pat, 0, NO_MOVE, TFlag::EXACT, static_cast<uint8_t>(inner_board.fullMoveNumber()));
        if (stand_pat >= beta){
            // return stand_pat;
            return beta;
        }

        if (depth == 0){
            return stand_pat;
        }
    }

    order_captures(legal_captures);
    alpha = std::max(alpha, stand_pat);

    float pos_eval;
    chess::Move move;
    for (int i = 0; i < legal_captures.size(); i++){
        move = legal_captures[i];

        // delta pruning
        // move.score() is calculated with set_capture_score which is material difference.
        if (stand_pat + move.score()/10 + 0.8 < alpha){ // division by 10 is to convert from pawn to "engine" centipawns.
            // 0.8 is the equivalent of a queen, as a safety margin
            continue;
        }

        inner_board.update_state(move);
        pos_eval = -qsearch(-beta, -alpha, -color, depth-1);
        inner_board.restore_state(move);
        
        if (pos_eval > alpha){
            alpha = pos_eval;
            if (alpha >= beta){ // only check for cutoffs when alpha gets updated.
                // return alpha;
                return beta;
            }
        }
    }
    return alpha;
}