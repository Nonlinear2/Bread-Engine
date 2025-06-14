#include "core.hpp"

void Engine::set_uci_display(bool v){
    display_uci = v;
}

int Engine::increment_mate_ply(int value){
    assert(is_mate(value));
    return (is_win(value) ? 1 : -1)*(std::abs(value) - 1);
}

bool Engine::is_mate(int value){
    return (std::abs(value) >= MATE_VALUE - MAX_MATE_PLY && std::abs(value) <= MATE_VALUE);
}

bool Engine::is_decisive(int value){
    return is_win(value) || is_loss(value);
}

bool Engine::is_win(int value){
    return (value >= TB_VALUE);
}

bool Engine::is_loss(int value){
    return (value <= -TB_VALUE);
}

// to make the engine prefer faster checkmates instead of stalling,
// we decrease the value if the checkmate is deeper in the search tree.
int Engine::get_mate_in_moves(int value){
    int ply = MATE_VALUE - std::abs(value);
    return (is_win(value) ? 1: -1)*(ply/2 + (ply%2 != 0)); 
}

int Engine::get_think_time(int time_left, int num_moves_out_of_book, int num_moves_until_time_control=0, int increment=0){
    float target;
    float move_num = num_moves_out_of_book < 10 ? static_cast<float>(num_moves_out_of_book) : 10;
    float factor = 2 -  move_num / 10;
    if (num_moves_until_time_control == 0){
        target = static_cast<float>(time_left) / 30;
    } else {
        target = static_cast<float>(time_left) / (num_moves_until_time_control+5);
    }
    return static_cast<int>(factor*target + 0.9F*increment);
}

bool Engine::update_interrupt_flag(){
    SearchLimit limit_ = limit.load();
    switch (limit_.type){
        case LimitType::Time:
            update_run_time();
            interrupt_flag = (run_time >= limit_.value);
            break;
        case LimitType::Nodes:
            interrupt_flag = (nodes >= limit_.value);
            break;
        default:
            interrupt_flag = false;
            break;
    }
    return interrupt_flag;
}

void Engine::update_run_time(){
    run_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start_time).count();
};

std::pair<std::string, std::string> Engine::get_pv_pmove(){
    std::string pv = "";
    std::string ponder_move = "";

    chess::Board pv_visitor = inner_board;

    for (int i = 0; i < current_depth; i++){
        bool is_hit;
        TEntry* transposition = transposition_table.probe(is_hit, pv_visitor.hash());
        if (!is_hit || transposition->best_move == NO_MOVE)
            break;

        if (pv_visitor.isRepetition(2) || pv_visitor.isHalfMoveDraw() || pv_visitor.isInsufficientMaterial())
            break;

        if (i == 1){
            ponder_move = chess::uci::moveToUci(transposition->best_move);
        }
        pv += " " + chess::uci::moveToUci(transposition->best_move);
        pv_visitor.makeMove(transposition->best_move);
    }
    return std::pair(pv, ponder_move);
}

chess::Move Engine::search(std::string fen, SearchLimit limit){
    inner_board.setFen(fen);
    return iterative_deepening(limit);
};

chess::Move Engine::search(SearchLimit limit){
    return iterative_deepening(limit);
};

chess::Move Engine::iterative_deepening(SearchLimit limit){
    if (is_nonsense) srand((unsigned int)time(NULL));
    if (is_nonsense && nonsense.should_bongcloud(inner_board.hash(), inner_board.fullMoveNumber()))
        return nonsense.play_bongcloud(display_uci);

    this->limit = limit;
    start_time = std::chrono::high_resolution_clock::now();

    std::string pv;
    std::string ponder_move = "";

    chess::Move best_move = NO_MOVE;

    SortedMoveGen<chess::movegen::MoveGenType::ALL>::clear_killer_moves();

    nodes = 0;

    current_depth = 0;

    root_moves.clear();

    // reset stack
    for (int i = 0; i < ENGINE_MAX_DEPTH + QSEARCH_MAX_DEPTH + 2; i++){
        stack[i] = Stack();
    }

    chess::Move tb_move;
    chess::Movelist tb_moves;
    if (inner_board.probe_root_dtz(tb_move, tb_moves, is_nonsense)){
        update_run_time();
        if (display_uci){
            std::cout << "info depth 0";
            std::cout << " score cp " << tb_move.score();
            std::cout << " nodes 0 nps 0";
            std::cout << " time " << run_time;
            std::cout << " hashfull " << transposition_table.hashfull();
        }
        if (is_nonsense && tb_move.score() == TB_VALUE){
            tb_move = nonsense.worst_winning_move(tb_move, tb_moves);
            tb_move.setScore(TB_VALUE);
            if (display_uci){
                std::cout << " pv " << chess::uci::moveToUci(tb_move) << std::endl;
                std::cout << "bestmove " << chess::uci::moveToUci(tb_move) << std::endl;
            }
            return tb_move;
        }
        if (display_uci){
            std::cout << " pv " << chess::uci::moveToUci(tb_move) << std::endl;
            std::cout << "bestmove " << chess::uci::moveToUci(tb_move) << std::endl;
        }
        return tb_move;
    };

    while (true){
        current_depth++;

        best_move = minimax_root(current_depth, root_ss);

        std::pair<std::string, std::string> pv_pmove = get_pv_pmove();
        pv = pv_pmove.first;
        if (pv_pmove.second.size() > 0){
            ponder_move = pv_pmove.second;
        }
        update_run_time();
        if (run_time == 0) run_time = 1; // avoid division by 0;
        // do not count interrupted searches in depth
        if (display_uci){
            std::cout << "info depth " << current_depth - interrupt_flag;
            if (is_mate(best_move.score())){
                std::cout << " score mate " << get_mate_in_moves(best_move.score()); 
            } else {
                std::cout << " score cp " << best_move.score();
            }
            std::cout << " nodes " << nodes;
            std::cout << " nps " << nodes*1000/run_time;
            std::cout << " time " << run_time;
            std::cout << " hashfull " << transposition_table.hashfull();
            std::cout << " pv" << pv << std::endl;
        }

        // should the search really stop if there is a mate for the oponent?
        if (interrupt_flag
            || is_mate(best_move.score())
            || (limit.type == LimitType::Depth && current_depth == limit.value)
            || current_depth == ENGINE_MAX_DEPTH) break;
    }
    if (is_nonsense && display_uci) nonsense.display_info();

    if (display_uci){
        std::cout << "bestmove " << chess::uci::moveToUci(best_move);
        if (ponder_move.size() > 0){
            std::cout << " ponder " << ponder_move;
        }
        std::cout << std::endl;
    }
    interrupt_flag = false;
    return best_move;
}

chess::Move Engine::minimax_root(int depth, Stack* ss){
    assert(depth < ENGINE_MAX_DEPTH);

    int alpha = -INFINITE_VALUE;
    int beta = INFINITE_VALUE;
    chess::Move best_move = NO_MOVE;
    uint64_t zobrist_hash = inner_board.hash();
    inner_board.synchronize();
    
    if (root_moves.empty()){
        chess::movegen::legalmoves(root_moves, inner_board);
        assert(!root_moves.empty());
        SortedMoveGen smg = SortedMoveGen<chess::movegen::MoveGenType::ALL>(inner_board, depth);

        for (int i = 0; i < root_moves.size(); i++)
            smg.set_score(root_moves[i]);

        std::stable_sort(root_moves.begin(), root_moves.end(),
            [](const chess::Move a, const chess::Move b){ return a.score() > b.score(); });

        for (chess::Move& m: root_moves)
            m.setScore(-INFINITE_VALUE);
    }
    
    std::vector<chess::Move> temp_root_moves(root_moves.begin(), root_moves.end());

    bool in_check = inner_board.inCheck();

    int pos_eval;
    int move_count = 0;
    for (chess::Move& move: root_moves){
        move_count++;
        bool is_capture = inner_board.isCapture(move);

        inner_board.update_state(move);
        ss->current_move = move;

        int new_depth = depth-1;
        new_depth += inner_board.inCheck();
        new_depth -= move_count > 2 && depth > 5 && !is_capture && !in_check;

        if (move_count == 1){
            pos_eval = -negamax<true>(new_depth, -beta, -alpha, ss + 1);
        } else {
            pos_eval = -negamax<false>(new_depth, -beta, -alpha, ss + 1);
        }

        inner_board.restore_state(move);

        if (interrupt_flag)
            return temp_root_moves[0];

        move.setScore(pos_eval);

        if (is_mate(pos_eval)) pos_eval = increment_mate_ply(pos_eval);

        if (pos_eval > alpha){
            best_move = move;
            std::rotate(temp_root_moves.begin(),
                temp_root_moves.begin() + move_count  - 1, temp_root_moves.begin() + move_count);
            alpha = pos_eval;
        }
    }
    
    transposition_table.store(zobrist_hash, alpha, NO_VALUE, depth, best_move, TFlag::EXACT, static_cast<uint8_t>(inner_board.fullMoveNumber()));

    std::copy(temp_root_moves.begin(), temp_root_moves.end(), root_moves.begin());

    return best_move;
}

template<bool pv>
int Engine::negamax(int depth, int alpha, int beta, Stack* ss){
    assert(alpha < INFINITE_VALUE && beta > -INFINITE_VALUE);
    assert(depth < ENGINE_MAX_DEPTH);

    nodes++;
    // we check can_return only at depth 5 or higher to avoid doing it at all nodes
    if (interrupt_flag || (depth >= 5 && update_interrupt_flag())){
        return 0; // the value doesn't matter, it won't be used.
    }

    // there are no repetition checks in qsearch as captures can never lead to repetition.
    if (inner_board.isRepetition(2) || inner_board.isHalfMoveDraw() || inner_board.isInsufficientMaterial()){
        return 0;
    }

    // transpositions will be checked inside of qsearch
    // if isRepetition(1), qsearch will not consider the danger of draw as it searches captures.
    if (depth <= 0){
        return qsearch<pv>(alpha, beta, 0, ss + 1);
    }

    int static_eval = NO_VALUE;
    int eval = NO_VALUE;

    const int initial_alpha = alpha;
    uint64_t zobrist_hash = inner_board.hash();

    SortedMoveGen sorted_move_gen = SortedMoveGen<chess::movegen::MoveGenType::ALL>(inner_board, depth);
    bool is_hit;
    TEntry* transposition = transposition_table.probe(is_hit, zobrist_hash);
    if (is_hit){
        if (transposition->eval() != NO_VALUE)
            static_eval = eval = transposition->eval();

        switch (transposition->flag()){
            case TFlag::EXACT:
                if (transposition->depth() >= depth && !inner_board.isRepetition(1)){
                    return transposition->value();
                }
                eval = transposition->value();
                break;
            case TFlag::LOWER_BOUND:
                if (transposition->depth() >= depth && !inner_board.isRepetition(1)){
                    alpha = std::max(alpha, transposition->value());
                }
                eval = transposition->value();
                break;
            case TFlag::UPPER_BOUND:
                if (transposition->depth() >= depth && !inner_board.isRepetition(1)){
                    beta = std::min(beta, transposition->value());
                }
                eval = transposition->value();
                break;
            default:
                break;
        }
        // we need to do a cutoff check because we updated alpha/beta.
        if (beta <= alpha){
            // no need to store in transposition table as is already there.
            return transposition->value();
        }

        if (transposition->best_move != NO_MOVE) sorted_move_gen.set_tt_move(transposition->best_move);
    }

    bool in_check = inner_board.inCheck();

    if (static_eval == NO_VALUE)
        static_eval = inner_board.evaluate();
    if (eval == NO_VALUE)
        eval = static_eval;

    // pruning
    if (!pv && !in_check){

        // razoring
        if (eval + 100*depth*depth + 250 < alpha){ 
            eval = qsearch<false>(alpha, beta, 0, ss + 1); // we update static eval to the better qsearch eval. //? tweak depth?
            if (eval <= alpha) return eval;
        }

        // reverse futility pruning
        if (depth < 6 && eval - depth*150 - 281 >= beta){
            return eval;
        }

        // null move pruning
        // maybe check for zugzwang?
        int null_move_eval;
        if (!inner_board.last_move_null() 
            && eval > beta - depth*90
            && beta != INFINITE_VALUE
            && !is_loss(beta))
        {
            int R = 2 + (eval >= beta) + depth / 4;
            inner_board.makeNullMove();
            ss->current_move = chess::Move::NULL_MOVE;
            null_move_eval = -negamax<false>(depth - R, -beta, -beta+1, ss + 1);
            inner_board.unmakeNullMove();
            if (null_move_eval >= beta) return null_move_eval;
        }
    }

    int max_eval = -INFINITE_VALUE;

    chess::Move best_move;
    chess::Move move;
    int pos_eval;
    bool tt_capture = is_hit && transposition->best_move != NO_MOVE && inner_board.isCapture(transposition->best_move);
    while (sorted_move_gen.next(move)){
        bool is_capture = inner_board.isCapture(move);

        // lmp
        if (!pv && !in_check && !is_capture && sorted_move_gen.index() > 3 + depth && !is_hit && eval < alpha) continue;

        bool is_killer = SortedMoveGen<chess::movegen::MoveGenType::ALL>::killer_moves[depth].in_buffer(move);

        inner_board.update_state(move);
        ss->current_move = move;

        bool gives_check = inner_board.inCheck();

        // late move reductions
        int new_depth = depth-1;
        new_depth += gives_check;
        new_depth -= sorted_move_gen.index() > 1 && !is_capture && !gives_check && !is_killer;
        new_depth -= depth > 5 && !is_hit && !is_killer; // IIR
        new_depth -= tt_capture && !is_capture;
        new_depth -= sorted_move_gen.index() > 10;

        new_depth = std::min(new_depth, ENGINE_MAX_DEPTH);

        if (pv && (sorted_move_gen.index() == 0)){
            pos_eval = -negamax<true>(new_depth, -beta, -alpha, ss + 1);
        } else {
            pos_eval = -negamax<false>(new_depth, -beta, -alpha, ss + 1);
            if ((new_depth < depth-1) && (pos_eval > alpha)){
                pos_eval = -negamax<false>(depth-1, -beta, -alpha, ss + 1);
            }
        }

        inner_board.restore_state(move);

        if (interrupt_flag) return 0;

        if (pos_eval > max_eval){
            max_eval = pos_eval;
            best_move = move;
        }

        alpha = std::max(alpha, pos_eval);
        if (beta <= alpha){
            if (!is_capture)
                sorted_move_gen.update_history(move, depth, 
                    (inner_board.sideToMove() == chess::Color::WHITE) ? 1: -1);
            SortedMoveGen<chess::movegen::MoveGenType::ALL>::killer_moves[depth].add_move(move);
            break;
        }
    }

    if (sorted_move_gen.tt_move == NO_MOVE && sorted_move_gen.generated_moves_count == 0){ // avoid calling expensive try_outcome_eval function
        // If board is in check, it is checkmate
        // if there are no legal moves and it's not check, it is stalemate so eval is 0
        max_eval = inner_board.inCheck() ? -MATE_VALUE : 0;
        // we know this eval is exact at any depth, but 
        // we also don't want this eval to pollute the transposition table.
        // the full move number will make sure it is replaced at some point.
        transposition_table.store(zobrist_hash, max_eval, NO_VALUE, 255, NO_MOVE, TFlag::EXACT, static_cast<uint8_t>(inner_board.fullMoveNumber()));
        return max_eval;
    }

    // if max eval was obtained because of a threefold repetition,
    // the eval should not be stored in the transposition table, as
    // this is a history dependent evaluation.
    // what if max eval was not obtained because of a threefold? in this case we loose
    // one TT entry which is completely fine.
    // what about evaluations higher in the tree where inner_board.isRepetition(1) is false?
    // these evals are not history dependent as they mean that one side can force a draw.
    if ((max_eval == 0 && inner_board.isRepetition(1)) || inner_board.halfMoveClock() + depth >= 100){
        return max_eval;
        // we fall through without storing the eval in the TT.
    };

    TFlag node_type;
    if (beta <= alpha){ // this means there was a cutoff. So true eval is equal or higher
        node_type = TFlag::LOWER_BOUND;
    } else if (max_eval <= initial_alpha){
        // this means that the evals were all lower than best option for us
        // So eval is equal or lower
        node_type = TFlag::UPPER_BOUND;
    } else {
        // eval is between initial alpha and beta. so search was completed without cutoffs, and is exact
        node_type = TFlag::EXACT;
    }
    if (is_mate(max_eval)) max_eval = increment_mate_ply(max_eval);
    transposition_table.store(zobrist_hash, max_eval, static_eval, depth, best_move, node_type, static_cast<uint8_t>(inner_board.fullMoveNumber()));

    return max_eval;
}

template<bool pv>
int Engine::qsearch(int alpha, int beta, int depth, Stack* ss){
    // assert(pv || ((alpha == (beta-1)) && (alpha == (beta-1))));
    nodes++;

    int stand_pat = NO_VALUE;

    // tablebase probe
    if (inner_board.probe_wdl(stand_pat))
        return stand_pat;

    if (inner_board.isHalfMoveDraw()) 
        return 0;

    // this is recomputed when qsearch is called the first time. Performance loss is probably low. 
    uint64_t zobrist_hash = inner_board.hash();

    // first we check for transposition. If it is outcome, it should have already been stored
    // with an exact flag, so the stand pat will be correct anyways.
    SortedMoveGen sorted_capture_gen = SortedMoveGen<chess::movegen::MoveGenType::CAPTURE>(inner_board);
    bool is_hit;
    TEntry* transposition = transposition_table.probe(is_hit, zobrist_hash);
    if (is_hit){
        if (transposition->eval() != NO_VALUE){
            stand_pat = transposition->eval();
        }

        switch (transposition->flag()){
            case TFlag::EXACT:
                return transposition->value();
            case TFlag::LOWER_BOUND:
                if (!pv) alpha = std::max(alpha, transposition->value());
                stand_pat = std::max(stand_pat, transposition->value());
                break;
            case TFlag::UPPER_BOUND:
                if (!pv) beta = std::min(beta, transposition->value());
                stand_pat = std::min(stand_pat, transposition->value());
                break;
            default:
                break;
        }

        if (beta <= alpha){
            // no need to store in transposition table as is already there.
            return transposition->value();
        }
        if (stand_pat >= beta)
            return stand_pat;
        if (transposition->best_move != NO_MOVE) sorted_capture_gen.set_tt_move(transposition->best_move);
    }

    if (stand_pat == NO_VALUE){
        stand_pat = inner_board.evaluate();
        if (stand_pat >= beta){
            transposition_table.store(zobrist_hash, stand_pat, NO_VALUE, DEPTH_QSEARCH, NO_MOVE, TFlag::EXACT, static_cast<uint8_t>(inner_board.fullMoveNumber()));
            return stand_pat;
        }
    }

    if (depth == -QSEARCH_MAX_DEPTH){
        return stand_pat;
    }

    alpha = std::max(alpha, stand_pat);

    chess::Square previous_to_square = ((ss - 1)->current_move).to();

    int max_value = stand_pat;
    int pos_eval;
    chess::Move move;
    chess::Move best_move = NO_MOVE;
    while (sorted_capture_gen.next(move)){
        // delta pruning
        // move.score() is calculated with set_capture_score which is material difference.
        // 1500 is a safety margin
        if (move.typeOf() != chess::Move::PROMOTION && move.to() != previous_to_square){
            if (stand_pat + move.score()*150 + 1500 < alpha) continue; // multiplication by 150 is to convert from pawn to "engine centipawns".

            // SEE pruning
            if (!SEE::evaluate(inner_board, move, (alpha-stand_pat)/150 - 2)) continue;
    
            if (!pv && sorted_capture_gen.index() > 7
                && stand_pat + 1000 < alpha && !SEE::evaluate(inner_board, move, -1)) continue;
        }

        inner_board.update_state(move);
        ss->current_move = move;
        pos_eval = -qsearch<pv>(-beta, -alpha, depth-1, ss + 1);
        inner_board.restore_state(move);

        if (pos_eval > max_value){
            max_value = pos_eval;
            best_move = move;
        }
        alpha = std::max(alpha, pos_eval);
        if (alpha >= beta){ // only check for cutoffs when alpha gets updated.
            break;
        }
    }

    if (sorted_capture_gen.tt_move == NO_MOVE && sorted_capture_gen.generated_moves_count == 0 
        && inner_board.try_outcome_eval(stand_pat)){
        transposition_table.store(zobrist_hash, stand_pat, NO_VALUE, DEPTH_QSEARCH, NO_MOVE, TFlag::EXACT, static_cast<uint8_t>(inner_board.fullMoveNumber()));
        return stand_pat;
    }

    if (inner_board.halfMoveClock() + (depth + QSEARCH_MAX_DEPTH) >= 100)
        return max_value; // avoid storing history dependant evals.

    if (depth == 0){
        transposition_table.store(zobrist_hash, max_value,
            stand_pat,
            DEPTH_QSEARCH, best_move,
            max_value >= beta ? TFlag::LOWER_BOUND : TFlag::UPPER_BOUND,
            static_cast<uint8_t>(inner_board.fullMoveNumber()));
    }

    if (is_mate(max_value))
        max_value = increment_mate_ply(max_value);
    return max_value;
}