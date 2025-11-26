#include "core.hpp"


UNACTIVE_TUNEABLE(r_1, int, 164, 0, 500, 20, 0.002);
UNACTIVE_TUNEABLE(r_2, int, 279, -100, 1000, 25, 0.002);
UNACTIVE_TUNEABLE(rfp_1, int, 159, 0, 500, 25, 0.002);
UNACTIVE_TUNEABLE(rfp_2, int, 50, 0, 1000, 50, 0.002);
UNACTIVE_TUNEABLE(rfp_3, int, 53, 0, 1000, 50, 0.002);
UNACTIVE_TUNEABLE(rfp_4, int, 144, -100, 1000, 20, 0.002);
UNACTIVE_TUNEABLE(nmp_1, int, 84, -50, 250, 10, 0.002);
UNACTIVE_TUNEABLE(nmp_2, int, 24, -300, 300, 15, 0.002);
UNACTIVE_TUNEABLE(lmp_1, int, 117, -100, 500, 20, 0.002);
UNACTIVE_TUNEABLE(see_1, int, 89, -100, 1000, 25, 0.002);
UNACTIVE_TUNEABLE(see_2, int, 10, 0, 100, 0.5, 0.002);
UNACTIVE_TUNEABLE(se_1, int, 8, -100, 100, 1, 0.002);
UNACTIVE_TUNEABLE(se_2, int, 1, -100, 100, 1, 0.002);
UNACTIVE_TUNEABLE(lmr_1, int, 9, 0, 23, 0.5, 0.002);
UNACTIVE_TUNEABLE(cont_1, int, 945, 0, 3000, 70, 0.002);
UNACTIVE_TUNEABLE(cont_2, int, 131, 0, 1500, 35, 0.002);
UNACTIVE_TUNEABLE(qs_fp_1, int, 1681, 0, 3000, 70, 0.002);
UNACTIVE_TUNEABLE(qs_see_1, int, 286, 0, 900, 50, 0.002);
UNACTIVE_TUNEABLE(qs_p_1, int, 1029, 0, 5000, 70, 0.002);
UNACTIVE_TUNEABLE(cthis_1, int, 8479, 0, 16000, 250, 0.002);
UNACTIVE_TUNEABLE(cthis_2, int, 578, 0, 3000, 60, 0.002);
UNACTIVE_TUNEABLE(qs_p_idx, int, 7, 0, 20, 1, 0.002);
UNACTIVE_TUNEABLE(his_1, int, 28, 0, 300, 7, 0.002);
UNACTIVE_TUNEABLE(his_2, int, 26, 0, 300, 5, 0.002);
UNACTIVE_TUNEABLE(his_3, int, 1003, 0, 5000, 200, 0.002);
UNACTIVE_TUNEABLE(asp_1, int, 100, 0, 5000, 20, 0.002);
UNACTIVE_TUNEABLE(asp_2, int, 341, 0, 5000, 60, 0.002);

TUNEABLE(red_1, int, 1231, 0, 10000, 200, 0.002);
TUNEABLE(red_2, int, 1834, 0, 10000, 200, 0.002);
TUNEABLE(red_3, int, 723, 0, 10000, 200, 0.002);
TUNEABLE(red_4, int, 1620, 0, 10000, 200, 0.002);
TUNEABLE(red_5, int, 1112, 0, 10000, 200, 0.002);

int nnue_evaluate(NnueBoard& pos){
    return pos.evaluate();
}

int Engine::get_think_time(float time_left, int num_moves_out_of_book, int num_moves_until_time_control=0, int increment=0){
    float target = num_moves_until_time_control == 0
        ? time_left / 20
        : time_left / (num_moves_until_time_control+5);

    return static_cast<int>(target + 0.9F*increment);
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

void Engine::clear_state(){
    transposition_table.clear();
    SortedMoveGen<movegen::MoveGenType::ALL>::history.clear();
    SortedMoveGen<movegen::MoveGenType::ALL>::cont_history.clear();
    SortedMoveGen<movegen::MoveGenType::ALL>::killer_moves.clear();
}

void Engine::save_state(std::string file){
    std::ofstream ofs(file, std::ios::binary | std::ios::out);
    if (!ofs) {
        std::cout << "Could not open file for writing." << std::endl;
        return;
    }

    transposition_table.save_to_stream(ofs);
    SortedMoveGen<movegen::MoveGenType::ALL>::history.save_to_stream(ofs);
    SortedMoveGen<movegen::MoveGenType::ALL>::cont_history.save_to_stream(ofs);
    SortedMoveGen<movegen::MoveGenType::ALL>::killer_moves.save_to_stream(ofs);

    ofs.close();
}

void Engine::load_state(std::string file){
    std::ifstream ifs(file, std::ios::binary | std::ios::in);
    if (!ifs) {
        std::cout << "Could not open file for reading." << std::endl;
        return;
    }

    transposition_table.load_from_stream(ifs);
    SortedMoveGen<movegen::MoveGenType::ALL>::history.load_from_stream(ifs);
    SortedMoveGen<movegen::MoveGenType::ALL>::cont_history.load_from_stream(ifs);
    SortedMoveGen<movegen::MoveGenType::ALL>::killer_moves.load_from_stream(ifs);

    ifs.close();
}

void Engine::update_run_time(){
    run_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now() - start_time
    ).count() + 1; // add 1 to avoid divisions by 0
};

std::pair<std::string, std::string> Engine::get_pv_pmove(){
    std::string pv = "";
    std::string ponder_move = "";

    Board pv_visitor = pos;

    for (int i = 0; i < current_depth; i++){
        bool is_hit;
        TTData transposition = transposition_table.probe(is_hit, pv_visitor.hash());
        if (transposition.move == Move::NO_MOVE || pv_visitor.isRepetition(2)
            || pv_visitor.isHalfMoveDraw() || pv_visitor.isInsufficientMaterial())
            break;

        if (i == 1)
            ponder_move = uci::moveToUci(transposition.move);

        pv += " " + uci::moveToUci(transposition.move);
        pv_visitor.makeMove(transposition.move);
    }
    return std::pair(pv, ponder_move);
}

Move Engine::search(std::string fen, SearchLimit limit){
    pos.setFen(fen);
    return iterative_deepening(limit);
};

Move Engine::search(SearchLimit limit){
    return iterative_deepening(limit);
};

Move Engine::iterative_deepening(SearchLimit limit){
    assert(is_nonsense || nonsense_stage == Nonsense::STANDARD);

    if (is_nonsense){
        srand((unsigned int)time(NULL));
        if (pos.fullMoveNumber() < 3){
            Move move = Nonsense::play_bongcloud(pos);
            if (move != Move::NO_MOVE)
                return move;
        }
    }

    if (nonsense_stage >= Nonsense::PROMOTE && limit.type == LimitType::Time)
        this->limit = SearchLimit(LimitType::Time, std::min(limit.value, 200)); // move quickly
    else
        this->limit = limit;

    start_time = std::chrono::high_resolution_clock::now();

    std::string pv;
    std::string ponder_move = "";

    Move best_move = Move::NO_MOVE;
    engine_color = pos.sideToMove();

    SortedMoveGen<movegen::MoveGenType::ALL>::killer_moves.clear();

    nodes = 0;
    current_depth = 0;

    root_moves.clear();

    // initialize stack
    for (int i = 0; i < MAX_PLY + STACK_PADDING_SIZE; i++){
        stack[i] = Stack();
    }

    bool root_tb_hit = tablebase_loaded && TB::probe_root_dtz(pos, best_move, root_moves, is_nonsense);
    if (root_tb_hit && !(is_nonsense && best_move.score() == TB_VALUE && !Nonsense::only_knight_bishop(pos))){
        update_run_time();
        std::cout << "info depth 0";
        std::cout << " score cp " << best_move.score();
        std::cout << " nodes 0 nps 0";
        std::cout << " time " << run_time;
        std::cout << " hashfull " << transposition_table.hashfull();
        std::cout << " pv " << uci::moveToUci(best_move) << std::endl;
        std::cout << "bestmove " << uci::moveToUci(best_move) << std::endl;
        return best_move;
    };

    switch (nonsense_stage){
        case Nonsense::TAKE_PIECES:
            if (!Nonsense::enough_material_for_nonsense(pos)){
                nonsense_stage = Nonsense::STANDARD;
                break;
            }

            if (root_tb_hit || pos.them(engine_color).count() == 1){
                clear_state();
                evaluate = Nonsense::evaluate;
                nonsense_stage = Nonsense::PROMOTE;
            }
            break;

        case Nonsense::PROMOTE:
            if (!Nonsense::enough_material_for_nonsense(pos)){
                nonsense_stage = Nonsense::STANDARD;
                break;
            }

            if (Nonsense::only_knight_bishop(pos)){
                clear_state();
                evaluate = nnue_evaluate;
                nonsense_stage = Nonsense::CHECKMATE;
            }
            break;

        default:
            break;
    }

    while (true){
        current_depth++;

        int asp_alpha;
        int asp_beta;
        if (current_depth <= 6){
            asp_alpha = -INFINITE_VALUE;
            asp_beta = INFINITE_VALUE;
        } else {
            int margin = asp_1 + asp_2*std::abs(best_move.score())/1024;
            asp_alpha = std::clamp(best_move.score() - margin, -INFINITE_VALUE, INFINITE_VALUE);
            asp_beta = std::clamp(best_move.score() + margin, -INFINITE_VALUE, INFINITE_VALUE);
        }

        while (true){
            negamax<true>(current_depth, asp_alpha, asp_beta, root_ss, false);
            best_move = root_moves[0];
            
            if (best_move.score() <= asp_alpha)
                asp_alpha -= asp_beta - asp_alpha;
            else if (best_move.score() >= asp_beta)
                asp_beta += asp_beta - asp_alpha;
            else
                break;

            asp_alpha = std::clamp(asp_alpha, -INFINITE_VALUE, INFINITE_VALUE);
            asp_beta = std::clamp(asp_beta, -INFINITE_VALUE, INFINITE_VALUE);
        }

        std::pair<std::string, std::string> pv_pmove = get_pv_pmove();
        pv = pv_pmove.first;
        if (pv_pmove.second.size() > 0)
            ponder_move = pv_pmove.second;

        update_run_time();

        // do not count interrupted searches in depth
        std::cout << "info depth " << current_depth - interrupt_flag;
        if (is_mate(best_move.score()))
            std::cout << " score mate " << get_mate_in_moves(best_move.score()); 
        else
            std::cout << " score cp " << best_move.score();

        std::cout << " nodes " << nodes;
        std::cout << " nps " << nodes * 1000 / run_time;
        std::cout << " time " << run_time;
        std::cout << " hashfull " << transposition_table.hashfull();
        std::cout << " pv" << pv << std::endl;
        
        // should the search really stop if there is a mate for the oponent?
        if (interrupt_flag
            || is_mate(best_move.score())
            || (limit.type == LimitType::Depth && current_depth == limit.value)
            || current_depth >= ENGINE_MAX_DEPTH)
            break;
    }

    if (is_nonsense){
        Nonsense::display_info();
        if (nonsense_stage == Nonsense::STANDARD){
            const int queen_value = piece_value[static_cast<int>(PieceType::QUEEN)];
            if (Nonsense::enough_material_for_nonsense(pos)
                && (pos.them(engine_color).count() == 1 
                    || (Nonsense::material_evaluate(pos) > queen_value
                        && best_move.score() > 3 * queen_value / 2)))
            {
                clear_state();
                nonsense_stage = Nonsense::TAKE_PIECES;
            }
        }
    }

    std::cout << "bestmove " << uci::moveToUci(best_move);
    if (ponder_move.size() > 0)
        std::cout << " ponder " << ponder_move;
    std::cout << std::endl;

    interrupt_flag = false;
    return best_move;
}

template<bool pv>
int Engine::negamax(int depth, int alpha, int beta, Stack* ss, bool cutnode){
    assert(alpha < INFINITE_VALUE && beta > -INFINITE_VALUE);
    assert(depth <= ENGINE_MAX_DEPTH);
    assert(alpha < beta);
    assert(pv || (alpha == beta - 1));

    nodes++;

    const bool root_node = ss == root_ss;
    assert(!root_node || pos.isGameOver().second == GameResult::NONE);

    const int ply = ss - root_ss;
    assert(ply < MAX_PLY); // avoid stack overflow

    if (root_node)
        pos.synchronize();

    // a stalemate will be processed after the move generation
    else if (pos.isRepetition(1))
        return 0;

    if (pos.isRepetition(2) || pos.isHalfMoveDraw() || pos.isInsufficientMaterial())
        return 0;

    // we check can_return only at depth 5 or higher to avoid doing it at all nodes
    if (interrupt_flag || (depth >= 5 && update_interrupt_flag()))
        return NO_VALUE; // the value doesn't matter, it won't be used.

    if (ply >= MAX_PLY - 1)
        return evaluate(pos);

    // transpositions will be checked inside of qsearch
    // if isRepetition(1), qsearch will not consider the danger of draw as it searches captures.
    if (depth <= 0)
        return qsearch<pv>(alpha, beta, 0, ss + 1);

    int static_eval, eval;

    // tablebase probe
    if (tablebase_loaded && TB::probe_wdl(pos, eval)){
        if (nonsense_stage == Nonsense::STANDARD
            || nonsense_stage == Nonsense::CHECKMATE
            || eval == 0)
            return eval;
    }

    // mate distance pruning
    if (!root_node){
        alpha = std::max(pos_to_root_mate_value(-MATE_VALUE, ply), alpha);
        beta  = std::min(pos_to_root_mate_value(MATE_VALUE, ply), beta);
        if (alpha >= beta)
            return alpha;
    }

    int max_value = -INFINITE_VALUE;
    Move best_move = Move::NO_MOVE;
    Move move;
    int value;
    Piece prev_piece = (ss - 1)->moved_piece;
    Square prev_to = (ss - 1)->current_move.to();

    const int initial_alpha = alpha;
    uint64_t zobrist_hash = pos.hash();

    SortedMoveGen move_gen = SortedMoveGen<movegen::MoveGenType::ALL>(
        root_node ? &root_moves : NULL, prev_piece, prev_to, pos, depth
    );

    if (root_node && root_moves.empty()){
        movegen::legalmoves(root_moves, pos);
        assert(!root_moves.empty());

        move_gen.prepare_pos_data();
        for (int i = 0; i < root_moves.size(); i++)
            move_gen.set_score(root_moves[i]);

        std::stable_sort(root_moves.begin(), root_moves.end(),
            [](const Move a, const Move b){ return a.score() > b.score(); });

        for (Move& m: root_moves)
            m.setScore(NO_VALUE);
    }

    bool is_hit;
    TTData transposition = transposition_table.probe(is_hit, zobrist_hash);
    if (is_mate(transposition.value))
        transposition.value = pos_to_root_mate_value(transposition.value, ply);

    static_eval = eval = transposition.static_eval;
    eval = transposition.value;
    move_gen.set_tt_move(transposition.move);
    
    chess::Move excluded_move = ss->excluded_move;
    if (!root_node && !pv && transposition.depth >= depth && excluded_move == Move::NO_MOVE){
        switch (transposition.flag){
            case TFlag::EXACT:
                return transposition.value;
            case TFlag::LOWER_BOUND:
                alpha = std::max(alpha, transposition.value);
                break;
            case TFlag::UPPER_BOUND:
                beta = std::min(beta, transposition.value);
                break;
            default:
                break;
        }
    }

    // we need to do a cutoff check because we updated alpha/beta.
    // no need to store in transposition table as is already there.
    if (beta <= alpha)
        return transposition.value;

    bool in_check = pos.inCheck();

    if (static_eval == NO_VALUE)
        static_eval = evaluate(pos);
    if (eval == NO_VALUE)
        eval = static_eval;

    ss->static_eval = static_eval;

    bool improving = is_valid(ss->static_eval) && is_valid((ss - 2)->static_eval)
        && ss->static_eval > (ss - 2)->static_eval;

    bool tt_capture = transposition.move != Move::NO_MOVE && pos.isCapture(transposition.move);

    // pruning
    if (!root_node && !pv && !in_check){

        // razoring
        if (eval + r_1*depth*depth + r_2 < alpha){ 
            eval = qsearch<false>(alpha, beta, 0, ss + 1); // we update static eval to the better qsearch eval.
            if (eval <= alpha)
                return eval;
        }

        // reverse futility pruning
        if (depth < 6 && eval - depth * (rfp_1 - rfp_2*cutnode) - rfp_3 + rfp_4*improving >= beta)
            return eval;

        // null move pruning
        // maybe check for zugzwang?
        int null_move_eval;
        if (cutnode && (ss - 1)->current_move != Move::NULL_MOVE && excluded_move == Move::NO_MOVE
            && eval > beta - depth*nmp_1 + nmp_2 && is_regular_eval(beta)){

            int R = 2 + (eval >= beta) + depth / 4 + tt_capture;
            ss->moved_piece = Piece::NONE;
            ss->current_move = Move::NULL_MOVE;
            pos.makeNullMove();
            null_move_eval = -negamax<false>(depth - R, -beta, -beta + 1, ss + 1, false);
            pos.unmakeNullMove();
            if (null_move_eval >= beta && !is_win(null_move_eval))
                return null_move_eval;
        }
    }

    while (move_gen.next(move)){
        bool is_capture = pos.isCapture(move);

        if (move == excluded_move)
            continue;

        bool is_killer = SortedMoveGen<movegen::MoveGenType::ALL>::killer_moves.in_buffer(depth, move);

        if (!root_node && is_valid(max_value) && !is_loss(max_value)){
            // lmp
            if (!in_check && !is_capture && move_gen.index() > 2 + depth + improving 
                && !is_hit && eval - lmp_1 * !improving < alpha)
                continue;

            // SEE pruning
            if (!in_check && move_gen.index() > 5 + depth / 2
                && depth < 5 && !SEE::evaluate(pos, move, alpha - static_eval - see_1 - see_2*depth))
                continue;

            // continuation history pruning
            if (!is_capture && !in_check
                && prev_piece != int(Piece::NONE)
                && prev_to != int(Square::underlying::NO_SQ)
                && move_gen.cont_history.get(prev_piece, prev_to, pos.at(move.from()), move.to()) < -cthis_1 - cthis_2*depth)
                continue;
        }

        int new_depth = depth-1;
    
        // singular extensions
        // we need to be careful regarding stack variables as they can get modified by the singular search
        // as it uses the same stack element
        int extension = 0;
        if (!root_node && is_regular_eval(transposition.value)
            && move == transposition.move && excluded_move == Move::NO_MOVE
            && depth >= 5 && (transposition.flag == TFlag::LOWER_BOUND || transposition.flag == TFlag::EXACT)
            && transposition.depth >= depth - 1)
        {
            int singular_beta = transposition.value - se_1 - se_2*depth;

            if (is_regular_eval(singular_beta)){
                ss->excluded_move = move;
                value = negamax<false>(new_depth / 2, singular_beta - 1, singular_beta, ss, cutnode);
                ss->excluded_move = Move::NO_MOVE;
    
                if (interrupt_flag)
                    return NO_VALUE;
    
                if (value < singular_beta)
                    extension = 1;
                else if (value >= beta && !is_decisive(value))
                    return value;
            }
        }

        new_depth += extension;

        ss->moved_piece = pos.at(move.from());
        ss->current_move = move;
        pos.update_state(move, transposition_table);

        bool gives_check = pos.inCheck();

        new_depth -= depth > 5 && !is_hit && !is_killer; // IIR
        new_depth = std::min(new_depth, ENGINE_MAX_DEPTH);

        int reduction = 0;

        reduction -= red_1 * (gives_check && !root_node);
        reduction += red_2 * (move_gen.index() > 1 && !is_capture);
        reduction += red_3 * (tt_capture && !is_capture);
        reduction += red_4 * (move_gen.index() > lmr_1);
        reduction += red_5 * (cutnode && depth > 7);
        reduction += 800 * (depth > 3 && !improving);

        int reduced_depth = std::min(new_depth - reduction / 1024, ENGINE_MAX_DEPTH);

        if (move_gen.index() > 0 && depth >= 2){
            value = -negamax<false>(reduced_depth, -alpha - 1, -alpha, ss + 1, true);

            if (value > alpha && reduced_depth < new_depth){
                value = -negamax<false>(new_depth, -alpha - 1, -alpha, ss + 1, !cutnode);
                if (!is_capture)
                    move_gen.update_cont_history(ss->moved_piece, move.to().index(), cont_1);
            } else if (value <= alpha && !is_capture)
                move_gen.update_cont_history(ss->moved_piece, move.to().index(), -cont_2);

        } else if (!pv || move_gen.index() > 0){
            value = -negamax<false>(new_depth, -alpha - 1, -alpha, ss + 1, !cutnode);
        }

        if (pv && (move_gen.index() == 0 || value > alpha)){
            value = -negamax<true>(new_depth, -beta, -alpha, ss + 1, false);
        }

        pos.restore_state(move);

        if (interrupt_flag)
            return NO_VALUE;

        if (root_node)
            root_moves[move_gen.index()].setScore(value);

        if (value > max_value){
            max_value = value;
            if (value > alpha)
                best_move = move;
            if (root_node){
                // ! This preserves the order of the array after the current move.
                // ! Rotate invalidates root_moves[move_gen.index()].
                std::rotate(root_moves.begin(), root_moves.begin() + move_gen.index(),
                    root_moves.begin() + move_gen.index() + 1);
            }
        }

        alpha = std::max(alpha, value);
        if (beta <= alpha){
            if (!is_capture)
                move_gen.update_history(move, std::min(depth*depth*his_1 + his_2, his_3));
            SortedMoveGen<movegen::MoveGenType::ALL>::killer_moves.add_move(depth, move);
            break;
        }
    }

    if (!is_valid(max_value)){
        assert(!root_node);
        if (move_gen.tt_move == Move::NO_MOVE && move_gen.empty()){
            // If board is in check, it is checkmate
            // if there are no legal moves and it's not check, it is stalemate so eval is 0
            max_value = pos.inCheck() ? pos_to_root_mate_value(-MATE_VALUE, ply) : 0;

            if (nonsense_stage == Nonsense::TAKE_PIECES
                && pos.them(engine_color).count() != 1
                && pos.sideToMove() != engine_color
                && is_loss(max_value))
                max_value = TB_VALUE;

            // if it should be checkmate, but there are not only bishops and knights, then say the position is winning
            if (nonsense_stage == Nonsense::PROMOTE
                && !Nonsense::only_knight_bishop(pos)
                && is_loss(max_value)){
                assert(!Nonsense::is_winning_side(pos));
                max_value = TB_VALUE;
            }

            return max_value;
        }

        // if the move gen is not empty, the only legal move must be the excluded move
        assert(excluded_move != Move::NO_MOVE);
        assert(move_gen.tt_move == excluded_move);
        assert(move_gen.index() == 1);

        // we return a fail low to extend the search by 1
        return alpha;
    }

    // early return without storing the eval in the TT
    if (!root_node && pos.halfMoveClock() + depth >= 100)
        return max_value;

    TFlag node_type;
    if (beta <= alpha){ // this means that there was a cutoff
        node_type = TFlag::LOWER_BOUND;
    } else if (max_value <= initial_alpha){
        // this means that the evals were all lower than best option for us
        node_type = TFlag::UPPER_BOUND;
    } else {
        // value is between initial alpha and beta, so search was completed without cutoffs, and is exact
        node_type = TFlag::EXACT;
    }

    assert(is_valid(max_value));

    transposition_table.store(zobrist_hash, to_tt(max_value, ply), static_eval, depth, best_move,
        node_type, static_cast<uint8_t>(pos.fullMoveNumber()), pv);

    return max_value;
}

template<bool pv>
int Engine::qsearch(int alpha, int beta, int depth, Stack* ss){
    assert(pv || (alpha == beta - 1));
    assert(alpha < INFINITE_VALUE && beta > -INFINITE_VALUE);
    assert(alpha < beta);

    nodes++;

    const int ply = ss - root_ss;
    assert(ply < MAX_PLY); // avoid stack overflow

    int stand_pat = NO_VALUE;

    // tablebase probe
    if (tablebase_loaded && TB::probe_wdl(pos, stand_pat))
        if (evaluate != Nonsense::evaluate || stand_pat == 0)
            return stand_pat;

    if (pos.isHalfMoveDraw() || pos.isInsufficientMaterial() || pos.isRepetition(1) || pos.isRepetition(2)) 
        return 0;

    if (ply >= MAX_PLY - 1)
        return evaluate(pos);

    int value;
    Move move;
    Move best_move = Move::NO_MOVE;

    // this is recomputed when qsearch is called the first time. Performance loss is probably low. 
    uint64_t zobrist_hash = pos.hash();

    SortedMoveGen capture_gen = SortedMoveGen<movegen::MoveGenType::CAPTURE>(
        (ss - 1)->moved_piece, (ss - 1)->current_move.to().index(), pos
    );

    bool is_hit;
    TTData transposition = transposition_table.probe(is_hit, zobrist_hash);
    if (is_mate(transposition.value))
        transposition.value = pos_to_root_mate_value(transposition.value, ply);

    if (!pv && is_valid(transposition.value)){
        switch (transposition.flag){
            case TFlag::EXACT:
                return transposition.value;
            case TFlag::LOWER_BOUND:
                alpha = std::max(alpha, transposition.value);
                break;
            case TFlag::UPPER_BOUND:
                beta = std::min(beta, transposition.value);
                break;
            default:
                break;
        }

        if (beta <= alpha)
            return transposition.value;
    }

    bool in_check = pos.inCheck();

    if (!in_check){
        stand_pat = transposition.static_eval;
    
        if (!is_valid(stand_pat))
            stand_pat = evaluate(pos);
    
        assert(is_regular_eval(stand_pat, false));
    
        if (is_valid(transposition.value) && !is_decisive(transposition.value)
            && (
                transposition.flag == TFlag::EXACT 
                || (transposition.flag == TFlag::LOWER_BOUND && transposition.value >= stand_pat)
                || (transposition.flag == TFlag::UPPER_BOUND && transposition.value <= stand_pat)
                ))
                stand_pat = transposition.value;
    
        if (stand_pat >= beta){
            if (!is_hit)
                transposition_table.store(zobrist_hash, to_tt(stand_pat, ply), stand_pat,
                    DEPTH_QSEARCH, Move::NO_MOVE, TFlag::EXACT, static_cast<uint8_t>(pos.fullMoveNumber()), pv);
            return stand_pat;
        }

        alpha = std::max(alpha, stand_pat);

        if (depth == -QSEARCH_MAX_DEPTH)
            return stand_pat;
    }

    capture_gen.set_tt_move(transposition.move);

    int max_value = in_check ? -INFINITE_VALUE : stand_pat;

    Square previous_to_square = ((ss - 1)->current_move).to();

    while (capture_gen.next(move)){
        Piece captured_piece = pos.at(move.to());
        Piece moved_piece = pos.at(move.from());

        if (!in_check && move.typeOf() != Move::PROMOTION && move.to() != previous_to_square){
            if (stand_pat 
                + piece_value[static_cast<int>(captured_piece.type())]
                - piece_value[static_cast<int>(moved_piece.type())]
                + qs_fp_1 < alpha)
                continue;

            // SEE pruning
            if (move != transposition.move && !SEE::evaluate(pos, move, alpha - stand_pat - qs_see_1))
                continue;

            // move count pruning
            if (capture_gen.index() > qs_p_idx && stand_pat + qs_p_1 < alpha)
                continue;
        }

        ss->moved_piece = moved_piece;
        ss->current_move = move;
        pos.update_state(move, transposition_table);
        value = -qsearch<pv>(-beta, -alpha, depth-1, ss + 1);
        pos.restore_state(move);

        if (value > max_value){
            max_value = value;
            if (value > alpha)
                best_move = move;
        }

        alpha = std::max(alpha, value);
        if (alpha >= beta)
            break;
    }

    assert(!pos.isInsufficientMaterial());
    if (capture_gen.tt_move == Move::NO_MOVE && capture_gen.empty() 
        && (in_check || pos.is_stalemate())){

        if (in_check)
            stand_pat = pos_to_root_mate_value(-MATE_VALUE, ply);
        else
            stand_pat = 0;

        if (nonsense_stage == Nonsense::TAKE_PIECES
            && pos.them(engine_color).count() != 1
            && pos.sideToMove() != engine_color
            && is_loss(stand_pat))
            stand_pat = TB_VALUE;

        // if it should be checkmate, but there are not only bishops and knights, then say the position is winning
        if (nonsense_stage == Nonsense::PROMOTE
            && !Nonsense::only_knight_bishop(pos)
            && is_loss(stand_pat)){
            assert(!Nonsense::is_winning_side(pos));
            stand_pat = TB_VALUE;
        }

        transposition_table.store(zobrist_hash, to_tt(stand_pat, ply), NO_VALUE, DEPTH_QSEARCH,
            Move::NO_MOVE, TFlag::EXACT, static_cast<uint8_t>(pos.fullMoveNumber()), pv);
        return stand_pat;
    }

    // assert(pos.isGameOver().second == GameResult::NONE);
    assert(is_valid(max_value));

    // avoid storing history dependant values
    if (pos.halfMoveClock() + depth + QSEARCH_MAX_DEPTH >= 100)
        return max_value;

    if (depth == 0 || depth == -1)
        transposition_table.store(zobrist_hash, to_tt(max_value, ply),
            stand_pat, DEPTH_QSEARCH, best_move,
            max_value >= beta ? TFlag::LOWER_BOUND : TFlag::UPPER_BOUND,
            static_cast<uint8_t>(pos.fullMoveNumber()), pv);

    return max_value;
}