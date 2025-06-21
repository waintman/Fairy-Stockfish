/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2024 The Stockfish developers (see AUTHORS file)

  Stockfish is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Stockfish is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef POSITION_H_INCLUDED
#define POSITION_H_INCLUDED

#include <cassert>
#include <deque>
#include <iosfwd>
#include <memory>
#include <string>

#ifdef FAIRY_STOCKFISH
#include <functional>
#include "variant.h"
#include "movegen.h"
#endif
#include "bitboard.h"
#include "nnue/nnue_accumulator.h"
#include "nnue/nnue_architecture.h"
#include "types.h"

namespace Stockfish {

class TranspositionTable;

// StateInfo struct stores information needed to restore a Position object to
// its previous state when we retract a move. Whenever a move is made on the
// board (by calling Position::do_move), a StateInfo object must be passed.

struct StateInfo {

    // Copied when making a move
    Key    materialKey;
    Key    pawnKey;
    Value  nonPawnMaterial[COLOR_NB];
    int    castlingRights;
    int    rule50;
    int    pliesFromNull;
    Square epSquare;
#ifdef FAIRY_STOCKFISH
    int    countingPly;
    CheckCount checksRemaining[COLOR_NB];
    Bitboard epSquares;
    Square castlingKingSquare[COLOR_NB];
    Bitboard wallSquares;
    Bitboard gatesBB[COLOR_NB];
    Piece      unpromotedCapturedPiece;
    Piece      unpromotedBycatch[SQUARE_NB];
    Bitboard   promotedBycatch;
    Bitboard   demotedBycatch;
#endif

    // Not copied when making a move (will be recomputed anyhow)
    Key        key;
    Bitboard   checkersBB;
    StateInfo* previous;
    Bitboard   blockersForKing[COLOR_NB];
    Bitboard   pinners[COLOR_NB];
    Bitboard   checkSquares[PIECE_TYPE_NB];
    Piece      capturedPiece;
#ifdef FAIRY_STOCKFISH
    Square     captureSquare; // when != to_sq, e.g., en passant
    Piece      promotionPawn;
    Bitboard   nonSlidingRiders;
    Bitboard   flippedPieces;
    Bitboard   pseudoRoyalCandidates;
    Bitboard   pseudoRoyals;
    OptBool    legalCapture;
    bool       capturedpromoted;
    bool       shak;
    bool       bikjang;
    bool       pass;
    Move       move;
#endif
    int        repetition;

    // Used by NNUE
    Eval::NNUE::Accumulator<Eval::NNUE::TransformedFeatureDimensionsBig>   accumulatorBig;
    Eval::NNUE::Accumulator<Eval::NNUE::TransformedFeatureDimensionsSmall> accumulatorSmall;
    DirtyPiece                                                             dirtyPiece;
};


// A list to keep track of the position states along the setup moves (from the
// start position to the position just before the search starts). Needed by
// 'draw by repetition' detection. Use a std::deque because pointers to
// elements are not invalidated upon list resizing.
using StateListPtr = std::unique_ptr<std::deque<StateInfo>>;


// Position class stores information regarding the board representation as
// pieces, side to move, hash keys, castling info, etc. Important methods are
// do_move() and undo_move(), used by the search to update node info when
// traversing the search tree.
class Position {
   public:
    static void init();

    Position()                           = default;
    Position(const Position&)            = delete;
    Position& operator=(const Position&) = delete;

    // FEN string input/output
#ifndef FAIRY_STOCKFISH
    Position&   set(const std::string& fenStr, bool isChess960, StateInfo* si);
#else
    Position& set(const Variant* v, const std::string& fenStr, bool isChess960, StateInfo* si);
#endif
    Position&   set(const std::string& code, Color c, StateInfo* si);
    std::string fen() const;
#ifdef FAIRY_STOCKFISH

    // Variant rule properties
    const Variant* variant() const;
    Rank max_rank() const;
    File max_file() const;
    int ranks() const;
    int files() const;
    Bitboard board_bb() const;
    Bitboard board_bb(Color c, PieceType pt) const;
    PieceSet piece_types() const;
    const std::string& piece_to_char() const;
    const std::string& piece_to_char_synonyms() const;
    PieceType promotion_pawn_type(Color c) const;
    PieceSet promotion_piece_types(Color c) const;
    int promotion_limit(PieceType pt) const;
    bool mandatory_pawn_promotion() const;
    EndgameEval endgame_eval() const;
    Rank castling_rank(Color c) const;
    PieceType castling_king_piece(Color c) const;
    PieceSet castling_rook_pieces(Color c) const;
    PieceType king_type() const;
    PieceType nnue_king() const;
    Square nnue_king_square(Color c) const;
    bool nnue_use_pockets() const;
    bool nnue_applicable() const;
    bool has_capture() const;
    bool must_drop() const;
    bool drop_loop() const;
    bool first_rank_pawn_drops() const;
    bool can_drop(Color c, PieceType pt) const;
    EnclosingRule enclosing_drop() const;
    bool sittuyin_rook_drop() const;
    bool drop_opposite_colored_bishop() const;
    PieceType drop_no_doubled() const;
    bool cambodian_moves() const;
    Bitboard diagonal_lines() const;
    bool pass(Color c) const;
    bool pass_on_stalemate(Color c) const;
    Bitboard promoted_soldiers(Color c) const;
    bool makpong() const;
    EnclosingRule flip_enclosed_pieces() const;
    // winning conditions
    int n_move_rule() const;
    int n_fold_rule() const;
    Value stalemate_value(int ply = 0) const;
    Value checkmate_value(int ply = 0) const;
    Value extinction_value(int ply = 0) const;
    bool extinction_claim() const;
    int extinction_opponent_piece_count() const;
    PieceType flag_piece(Color c) const;
    Bitboard flag_region(Color c) const;
    bool flag_move() const;
    bool flag_reached(Color c) const;
    bool check_counting() const;
    int connect_n() const;
    PieceSet connect_piece_types() const;
    bool connect_horizontal() const;
    bool connect_vertical() const;
    bool connect_diagonal() const;
    const std::vector<Direction>& getConnectDirections() const;
    int connect_nxn() const;
    int collinear_n() const;

    CheckCount checks_remaining(Color c) const;
    MaterialCounting material_counting() const;

    // Variant-specific properties
    int count_in_hand(PieceType pt) const;
    int count_in_hand(Color c, PieceType pt) const;
    int count_with_hand(Color c, PieceType pt) const;
    bool bikjang() const;
    bool allow_virtual_drop(Color c, PieceType pt) const;
#endif

    // Position representation
    Bitboard pieces(PieceType pt = ALL_PIECES) const;
    template<typename... PieceTypes>
    Bitboard pieces(PieceType pt, PieceTypes... pts) const;
    Bitboard pieces(Color c) const;
    template<typename... PieceTypes>
    Bitboard pieces(Color c, PieceTypes... pts) const;
#ifdef FAIRY_STOCKFISH
    Bitboard major_pieces(Color c) const;
    Bitboard non_sliding_riders() const;
#endif
    Piece    piece_on(Square s) const;
#ifdef FAIRY_STOCKFISH
    Piece unpromoted_piece_on(Square s) const;
#endif
    Square   ep_square() const;
#ifdef FAIRY_STOCKFISH
    Bitboard ep_squares() const;
    Square castling_king_square(Color c) const;
    Bitboard gates(Color c) const;
#endif
    bool     empty(Square s) const;
#ifdef FAIRY_STOCKFISH
    int count(Color c, PieceType pt) const;
#endif
    template<PieceType Pt>
    int count(Color c) const;
    template<PieceType Pt>
    int count() const;
    template<PieceType Pt>
    Square square(Color c) const;
#ifdef FAIRY_STOCKFISH
    Square square(Color c, PieceType pt) const;
#endif

    // Castling
    CastlingRights castling_rights(Color c) const;
    bool           can_castle(CastlingRights cr) const;
    bool           castling_impeded(CastlingRights cr) const;
    Square         castling_rook_square(CastlingRights cr) const;

    // Checking
    Bitboard checkers() const;
    Bitboard blockers_for_king(Color c) const;
    Bitboard check_squares(PieceType pt) const;
    Bitboard pinners(Color c) const;

    // Attacks to/from a given square
    Bitboard attackers_to(Square s) const;
#ifdef FAIRY_STOCKFISH
    Bitboard attackers_to(Square s, Color c) const;
    Bitboard attackers_to(Square s, Bitboard occupied, Color c) const;
    Bitboard attackers_to(Square s, Bitboard occupied, Color c, Bitboard janggiCannons) const;
    Bitboard attacks_from(Color c, PieceType pt, Square s) const;
    Bitboard moves_from(Color c, PieceType pt, Square s) const;
#endif
    Bitboard attackers_to(Square s, Bitboard occupied) const;
    void     update_slider_blockers(Color c) const;
#ifdef FAIRY_STOCKFISH
    void     update_slider_blockers(Bitboard sliders, Square s, Bitboard& pinners, Color c) const;
#endif
    template<PieceType Pt>
    Bitboard attacks_by(Color c) const;

    // Properties of moves
    bool  legal(Move m) const;
    bool  pseudo_legal(const Move m) const;
    bool  capture(Move m) const;
#ifdef FAIRY_STOCKFISH
    Square capture_square(Square to) const;
#endif
    bool  capture_stage(Move m) const;
    bool  gives_check(Move m) const;
    Piece moved_piece(Move m) const;
    Piece captured_piece() const;
#ifdef FAIRY_STOCKFISH
    const std::string piece_to_partner() const;
    bool is_promoted(Square s) const;
#endif

    // Doing and undoing moves
    void do_move(Move m, StateInfo& newSt);
    void do_move(Move m, StateInfo& newSt, bool givesCheck);
    void undo_move(Move m);
    void do_null_move(StateInfo& newSt, TranspositionTable& tt);
    void undo_null_move();

    // Static Exchange Evaluation
    bool see_ge(Move m, int threshold = 0) const;

    // Accessing hash keys
    Key key() const;
    Key key_after(Move m) const;
#ifndef FAIRY_STOCKFISH
    Key material_key() const;
#else
    Key material_key(EndgameEval e = EG_EVAL_CHESS) const;
#endif
    Key pawn_key() const;

    // Other properties of the position
    Color side_to_move() const;
    int   game_ply() const;
    bool  is_chess960() const;
#ifdef FAIRY_STOCKFISH
    bool is_immediate_game_end() const;
    bool is_immediate_game_end(Value& result, int ply = 0) const;
    bool is_optional_game_end() const;
    bool is_optional_game_end(Value& result, int ply = 0) const;
    bool is_game_end(Value& result, int ply = 0) const;
    Value material_counting_result() const;
#endif
    bool  is_draw(int ply) const;
    bool  has_game_cycle(int ply) const;
    bool  has_repeated() const;

    int   rule50_count() const;
    Value non_pawn_material(Color c) const;
    Value non_pawn_material() const;

    // Position consistency check, for debugging
    bool pos_is_ok() const;
    void flip();
#ifdef FAIRY_STOCKFISH
    Bitboard fog_area() const;
#endif

    // Used by NNUE
    StateInfo* state() const;

#ifndef FAIRY_STOCKFISH
    void put_piece(Piece pc, Square s);
#else
    void put_piece(Piece pc, Square s, bool isPromoted = false, Piece unpromotedPc = NO_PIECE);
#endif
    void remove_piece(Square s);

   private:
    // Initialization helpers (used while setting up a position)
    void set_castling_right(Color c, Square rfrom);
    void set_state() const;
    void set_check_info() const;

    // Other helpers
    void move_piece(Square from, Square to);
    template<bool Do>
    void do_castling(Color us, Square from, Square& to, Square& rfrom, Square& rto);
    template<bool AfterMove>
    Key adjust_key50(Key k) const;

    // Data members
    Piece      board[SQUARE_NB];
#ifdef FAIRY_STOCKFISH
    Piece unpromotedBoard[SQUARE_NB];
#endif
    Bitboard   byTypeBB[PIECE_TYPE_NB];
    Bitboard   byColorBB[COLOR_NB];
    int        pieceCount[PIECE_NB];
    int        castlingRightsMask[SQUARE_NB];
    Square     castlingRookSquare[CASTLING_RIGHT_NB];
    Bitboard   castlingPath[CASTLING_RIGHT_NB];
    StateInfo* st;
    int        gamePly;
    Color      sideToMove;
#ifdef FAIRY_STOCKFISH
    // variant-specific
    const Variant* var;
    bool tsumeMode;
#endif
    bool       chess960;
#ifdef FAIRY_STOCKFISH
    int pieceCountInHand[COLOR_NB][PIECE_TYPE_NB];
    int virtualPieces;
    Bitboard promotedPieces;
    void add_to_hand(Piece pc);
    void remove_from_hand(Piece pc);
#endif
};

std::ostream& operator<<(std::ostream& os, const Position& pos);

#ifdef FAIRY_STOCKFISH
inline const Variant* Position::variant() const {
    assert(var != nullptr);
    return var;
}

inline Rank Position::max_rank() const {
    assert(var != nullptr);
    return var->maxRank;
}

inline File Position::max_file() const {
    assert(var != nullptr);
    return var->maxFile;
}

inline int Position::ranks() const {
    assert(var != nullptr);
    return var->maxRank + 1;
}

inline int Position::files() const {
    assert(var != nullptr);
    return var->maxFile + 1;
}

inline Bitboard Position::board_bb() const {
    assert(var != nullptr);
    return board_size_bb(var->maxFile, var->maxRank) & ~st->wallSquares;
}

inline Bitboard Position::board_bb(Color c, PieceType pt) const {
    assert(var != nullptr);
    return var->mobilityRegion[c][pt] ? var->mobilityRegion[c][pt] & board_bb() : board_bb();
}

inline PieceSet Position::piece_types() const {
    assert(var != nullptr);
    return var->pieceTypes;
}

inline const std::string& Position::piece_to_char() const {
    assert(var != nullptr);
    return var->pieceToChar;
}

inline const std::string& Position::piece_to_char_synonyms() const {
    assert(var != nullptr);
    return var->pieceToCharSynonyms;
}

inline bool Position::mandatory_pawn_promotion() const {
    assert(var != nullptr);
    return var->mandatoryPawnPromotion;
}

inline EndgameEval Position::endgame_eval() const {
    assert(var != nullptr);
    return !count_in_hand(ALL_PIECES) && (var->endgameEval != EG_EVAL_CHESS || count<KING>() == 2) ? var->endgameEval : NO_EG_EVAL;
}

inline Rank Position::castling_rank(Color c) const {
    assert(var != nullptr);
    return relative_rank(c, var->castlingRank, max_rank());
}

inline PieceType Position::castling_king_piece(Color c) const {
    assert(var != nullptr);
    return var->castlingKingPiece[c];
}

inline PieceSet Position::castling_rook_pieces(Color c) const {
    assert(var != nullptr);
    return var->castlingRookPieces[c];
}

inline PieceType Position::king_type() const {
    assert(var != nullptr);
    return var->kingType;
}

inline PieceType Position::nnue_king() const {
    assert(var != nullptr);
    return var->nnueKing;
}

inline Square Position::nnue_king_square(Color c) const {
    return nnue_king() ? square(c, nnue_king()) : SQ_NONE;
}

inline bool Position::nnue_use_pockets() const {
    assert(var != nullptr);
    return var->nnueUsePockets;
}

inline bool Position::nnue_applicable() const {
    // Do not use NNUE during setup phases (placement, sittuyin)
    return (!count_in_hand(ALL_PIECES) || nnue_use_pockets() || !must_drop()) && !virtualPieces;
}

inline bool Position::has_capture() const {
    // Check for cached value
    if (st->legalCapture != NO_VALUE)
        return st->legalCapture == VALUE_TRUE;
    if (checkers())
    {
        for (const auto& mevasion : MoveList<EVASIONS>(*this))
            if (capture(mevasion) && legal(mevasion))
            {
                st->legalCapture = VALUE_TRUE;
                return true;
            }
    }
    else
    {
        for (const auto& mcap : MoveList<CAPTURES>(*this))
            if (capture(mcap) && legal(mcap))
            {
                st->legalCapture = VALUE_TRUE;
                return true;
            }
    }
    st->legalCapture = VALUE_FALSE;
    return false;
}

inline bool Position::must_drop() const {
    assert(var != nullptr);
    return var->mustDrop;
}

inline bool Position::drop_loop() const {
    assert(var != nullptr);
    return var->dropLoop;
}

inline bool Position::first_rank_pawn_drops() const {
    assert(var != nullptr);
    return var->firstRankPawnDrops;
}

inline EnclosingRule Position::enclosing_drop() const {
    assert(var != nullptr);
    return var->enclosingDrop;
}

inline bool Position::sittuyin_rook_drop() const {
    assert(var != nullptr);
    return var->sittuyinRookDrop;
}

inline bool Position::drop_opposite_colored_bishop() const {
    assert(var != nullptr);
    return var->dropOppositeColoredBishop;
}

inline PieceType Position::drop_no_doubled() const {
    assert(var != nullptr);
    return var->dropNoDoubled;
}

inline bool Position::cambodian_moves() const {
    assert(var != nullptr);
    return var->cambodianMoves;
}

inline Bitboard Position::diagonal_lines() const {
    assert(var != nullptr);
    return var->diagonalLines;
}

inline bool Position::pass(Color c) const {
    assert(var != nullptr);
    return var->pass[c] || var->passOnStalemate[c];
}

inline bool Position::pass_on_stalemate(Color c) const {
    assert(var != nullptr);
    return var->passOnStalemate[c];
}

inline Bitboard Position::promoted_soldiers(Color c) const {
    assert(var != nullptr);
    return pieces(c, SOLDIER) & zone_bb(c, var->soldierPromotionRank, max_rank());
}

inline bool Position::makpong() const {
    assert(var != nullptr);
    return var->makpongRule;
}

inline int Position::n_move_rule() const {
    assert(var != nullptr);
    return var->nMoveRule;
}

inline int Position::n_fold_rule() const {
    assert(var != nullptr);
    return var->nFoldRule;
}

inline EnclosingRule Position::flip_enclosed_pieces() const {
    assert(var != nullptr);
    return var->flipEnclosedPieces;
}

inline Value Position::stalemate_value(int ply) const {
    assert(var != nullptr);
    if (var->stalematePieceCount)
    {
        int c = count<ALL_PIECES>(sideToMove) - count<ALL_PIECES>(~sideToMove);
        return c == 0 ? VALUE_DRAW : convert_mate_value(c < 0 ? var->stalemateValue : -var->stalemateValue, ply);
    }
    return convert_mate_value(var->stalemateValue, ply);
}

inline Value Position::checkmate_value(int ply) const {
    assert(var != nullptr);
    // Check for illegal mate by shogi pawn drop
    if (    var->shogiPawnDropMateIllegal
        && !(checkers() & ~pieces(SHOGI_PAWN))
        && !st->capturedPiece
        &&  st->pliesFromNull > 0
        && (st->materialKey != st->previous->materialKey))
    {
        return mate_in(ply);
    }
    // Check for shatar mate rule
    if (var->shatarMateRule)
    {
        // Mate by knight is illegal
        if (!(checkers() & ~pieces(KNIGHT)))
            return mate_in(ply);

        StateInfo* stp = st;
        while (stp->checkersBB)
        {
            // Return mate score if there is at least one shak in series of checks
            if (stp->shak)
                return convert_mate_value(var->checkmateValue, ply);

            if (stp->pliesFromNull < 2)
                break;

            stp = stp->previous->previous;
        }
        // Niol
        return VALUE_DRAW;
    }

    // Return mate value
    return convert_mate_value(var->checkmateValue, ply);
}

inline Value Position::extinction_value(int ply) const {
    assert(var != nullptr);
    return convert_mate_value(var->extinctionValue, ply);
}

inline bool Position::extinction_claim() const {
    assert(var != nullptr);
    return var->extinctionClaim;
}

inline int Position::extinction_opponent_piece_count() const {
    assert(var != nullptr);
    return var->extinctionOpponentPieceCount;
}

inline PieceType Position::flag_piece(Color c) const {
    assert(var != nullptr);
    return var->flagPiece[c];
}

inline Bitboard Position::flag_region(Color c) const {
    assert(var != nullptr);
    return var->flagRegion[c];
}

inline bool Position::flag_move() const {
    assert(var != nullptr);
    return var->flagMove;
}

inline bool Position::flag_reached(Color c) const {
    assert(var != nullptr);
    bool simpleResult =
          (flag_region(c) & pieces(c, flag_piece(c)))
          && (   popcount(flag_region(c) & pieces(c, flag_piece(c))) >= var->flagPieceCount
              || (var->flagPieceBlockedWin && !(flag_region(c) & ~pieces())));

    if (simpleResult&&var->flagPieceSafe)
    {
        Bitboard piecesInFlagZone = flag_region(c) & pieces(c, flag_piece(c));
        int potentialPieces = (popcount(piecesInFlagZone));
        /*
        There isn't a variant that uses it, but in the hypothetical game where the rules say I need 3
        pieces in the flag zone and they need to be safe: If I have 3 pieces there, but one is under
        threat, I don't think I can declare victory. If I have 4 there, but one is under threat, I
        think that's victory.
        */
        while (piecesInFlagZone)
        {
            Square sr = pop_lsb(piecesInFlagZone);
            Bitboard flagAttackers = attackers_to(sr, ~c);

            if ((potentialPieces < var->flagPieceCount) || (potentialPieces >= var->flagPieceCount + 1)) break;
            while (flagAttackers)
            {
                Square currentAttack = pop_lsb(flagAttackers);
                if (legal(make_move(currentAttack, sr)))
                {
                    potentialPieces--;
                    break;
                }
            }
        }
        return potentialPieces >= var->flagPieceCount;
    }
    return simpleResult;
}

inline bool Position::check_counting() const {
    assert(var != nullptr);
    return var->checkCounting;
}

inline int Position::connect_n() const {
    assert(var != nullptr);
    return var->connectN;
}

inline PieceSet Position::connect_piece_types() const {
    assert(var != nullptr);
    return var->connectPieceTypesTrimmed;
}

inline bool Position::connect_horizontal() const {
    assert(var != nullptr);
    return var->connectHorizontal;
}
inline bool Position::connect_vertical() const {
    assert(var != nullptr);
    return var->connectVertical;
}
inline bool Position::connect_diagonal() const {
    assert(var != nullptr);
    return var->connectDiagonal;
}

inline const std::vector<Direction>& Position::getConnectDirections() const {
    assert(var != nullptr);
    return var->connect_directions;
}

inline int Position::connect_nxn() const {
    assert(var != nullptr);
    return var->connectNxN;
}

inline int Position::collinear_n() const {
    assert(var != nullptr);
    return var->collinearN;
}

inline CheckCount Position::checks_remaining(Color c) const {
    return st->checksRemaining[c];
}

inline MaterialCounting Position::material_counting() const {
    assert(var != nullptr);
    return var->materialCounting;
}

inline bool Position::is_immediate_game_end() const {
    Value result;
    return is_immediate_game_end(result);
}

inline bool Position::is_optional_game_end() const {
    Value result;
    return is_optional_game_end(result);
}

inline bool Position::is_draw(int ply) const {
    Value result;
    return is_optional_game_end(result, ply);
}

inline bool Position::is_game_end(Value& result, int ply) const {
    return is_immediate_game_end(result, ply) || is_optional_game_end(result, ply);
}
#endif
inline Color Position::side_to_move() const { return sideToMove; }

inline Piece Position::piece_on(Square s) const {
    assert(is_ok(s));
    return board[s];
}

inline bool Position::empty(Square s) const { return piece_on(s) == NO_PIECE; }

inline Piece Position::moved_piece(Move m) const { return piece_on(m.from_sq()); }

inline Bitboard Position::pieces(PieceType pt) const { return byTypeBB[pt]; }

template<typename... PieceTypes>
inline Bitboard Position::pieces(PieceType pt, PieceTypes... pts) const {
    return pieces(pt) | pieces(pts...);
}

#ifdef FAIRY_STOCKFISH
inline Piece Position::unpromoted_piece_on(Square s) const {
    return unpromotedBoard[s];
}
#endif
inline Bitboard Position::pieces(Color c) const { return byColorBB[c]; }

template<typename... PieceTypes>
inline Bitboard Position::pieces(Color c, PieceTypes... pts) const {
    return pieces(c) & pieces(pts...);
}

template<PieceType Pt>
inline int Position::count(Color c) const {
    return pieceCount[make_piece(c, Pt)];
}

template<PieceType Pt>
inline int Position::count() const {
    return count<Pt>(WHITE) + count<Pt>(BLACK);
}

template<PieceType Pt>
inline Square Position::square(Color c) const {
    assert(count<Pt>(c) == 1);
    return lsb(pieces(c, Pt));
}

#ifdef FAIRY_STOCKFISH
inline Bitboard Position::major_pieces(Color c) const {
    return pieces(c) & (pieces(QUEEN) | pieces(AIWOK) | pieces(ARCHBISHOP) | pieces(CHANCELLOR) | pieces(AMAZON));
}

inline Bitboard Position::non_sliding_riders() const {
    return st->nonSlidingRiders;
}

inline int Position::count(Color c, PieceType pt) const {
    return pieceCount[make_piece(c, pt)];
}

inline Square Position::square(Color c, PieceType pt) const {
    assert(count(c, pt) == 1);
    return lsb(pieces(c, pt));
}

#endif
inline Square Position::ep_square() const { return st->epSquare; }
#ifdef FAIRY_STOCKFISH
inline Bitboard Position::ep_squares() const {
    return st->epSquares;
}

inline Square Position::castling_king_square(Color c) const {
    return st->castlingKingSquare[c];
}

inline Bitboard Position::gates(Color c) const {
    assert(var != nullptr);
    return st->gatesBB[c];
}

#endif
inline bool Position::can_castle(CastlingRights cr) const { return st->castlingRights & cr; }

inline CastlingRights Position::castling_rights(Color c) const {
    return c & CastlingRights(st->castlingRights);
}

inline bool Position::castling_impeded(CastlingRights cr) const {
    assert(cr == WHITE_OO || cr == WHITE_OOO || cr == BLACK_OO || cr == BLACK_OOO);
    return pieces() & castlingPath[cr];
}

inline Square Position::castling_rook_square(CastlingRights cr) const {
    assert(cr == WHITE_OO || cr == WHITE_OOO || cr == BLACK_OO || cr == BLACK_OOO);
    return castlingRookSquare[cr];
}

#ifdef FAIRY_STOCKFISH
inline Bitboard Position::attacks_from(Color c, PieceType pt, Square s) const {
    if (var->fastAttacks || var->fastAttacks2)
        return attacks_bb(c, pt, s, byTypeBB[ALL_PIECES]) & board_bb();

    PieceType movePt = pt == KING ? king_type() : pt;
    Bitboard b = attacks_bb(c, movePt, s, byTypeBB[ALL_PIECES]);
    // Xiangqi soldier
    if (pt == SOLDIER && !(promoted_soldiers(c) & s))
        b &= file_bb(file_of(s));
    // Janggi cannon restrictions
    if (pt == JANGGI_CANNON)
    {
        b &= ~pieces(pt);
        b &= attacks_bb(c, pt, s, pieces() ^ pieces(pt));
    }
    // Janggi palace moves
    if (diagonal_lines() & s)
    {
        PieceType diagType = movePt == WAZIR ? FERS : movePt == SOLDIER ? PAWN : movePt == ROOK ? BISHOP : NO_PIECE_TYPE;
        if (diagType)
            b |= attacks_bb(c, diagType, s, pieces()) & diagonal_lines();
        else if (movePt == JANGGI_CANNON)
            b |=  rider_attacks_bb<RIDER_CANNON_DIAG>(s, pieces())
                & rider_attacks_bb<RIDER_CANNON_DIAG>(s, pieces() ^ pieces(pt))
                & ~pieces(pt)
                & diagonal_lines();
    }
    return b & board_bb(c, pt);
}

inline Bitboard Position::moves_from(Color c, PieceType pt, Square s) const {
    if (var->fastAttacks || var->fastAttacks2)
        return moves_bb(c, pt, s, byTypeBB[ALL_PIECES]) & board_bb();

    PieceType movePt = pt == KING ? king_type() : pt;
    Bitboard b = moves_bb(c, movePt, s, byTypeBB[ALL_PIECES]);
    // Xiangqi soldier
    if (pt == SOLDIER && !(promoted_soldiers(c) & s))
        b &= file_bb(file_of(s));
    // Janggi cannon restrictions
    if (pt == JANGGI_CANNON)
    {
        b &= ~pieces(pt);
        b &= attacks_bb(c, pt, s, pieces() ^ pieces(pt));
    }
    // Janggi palace moves
    if (diagonal_lines() & s)
    {
        PieceType diagType = movePt == WAZIR ? FERS : movePt == SOLDIER ? PAWN : movePt == ROOK ? BISHOP : NO_PIECE_TYPE;
        if (diagType)
            b |= attacks_bb(c, diagType, s, pieces()) & diagonal_lines();
        else if (movePt == JANGGI_CANNON)
            b |=  rider_attacks_bb<RIDER_CANNON_DIAG>(s, pieces())
                & rider_attacks_bb<RIDER_CANNON_DIAG>(s, pieces() ^ pieces(pt))
                & ~pieces(pt)
                & diagonal_lines();
    }
    return b & board_bb(c, pt);
}
#endif
inline Bitboard Position::attackers_to(Square s) const { return attackers_to(s, pieces()); }

#ifdef FAIRY_STOCKFISH
inline Bitboard Position::attackers_to(Square s, Color c) const {
    return attackers_to(s, byTypeBB[ALL_PIECES], c);
}

inline Bitboard Position::attackers_to(Square s, Bitboard occupied, Color c) const {
    return attackers_to(s, occupied, c, byTypeBB[JANGGI_CANNON]);
}
#endif

template<PieceType Pt>
inline Bitboard Position::attacks_by(Color c) const {

    if constexpr (Pt == PAWN)
        return c == WHITE ? pawn_attacks_bb<WHITE>(pieces(WHITE, PAWN))
                          : pawn_attacks_bb<BLACK>(pieces(BLACK, PAWN));
    else
    {
        Bitboard threats   = 0;
        Bitboard attackers = pieces(c, Pt);
        while (attackers)
            threats |= attacks_bb<Pt>(pop_lsb(attackers), pieces());
        return threats;
    }
}

inline Bitboard Position::checkers() const { return st->checkersBB; }

inline Bitboard Position::blockers_for_king(Color c) const { return st->blockersForKing[c]; }

inline Bitboard Position::pinners(Color c) const { return st->pinners[c]; }

inline Bitboard Position::check_squares(PieceType pt) const { return st->checkSquares[pt]; }

inline Key Position::key() const { return adjust_key50<false>(st->key); }

template<bool AfterMove>
inline Key Position::adjust_key50(Key k) const {
    return st->rule50 < 14 - AfterMove ? k : k ^ make_key((st->rule50 - (14 - AfterMove)) / 8);
}

inline Key Position::pawn_key() const { return st->pawnKey; }

#ifndef FAIRY_STOCKFISH
inline Key Position::material_key() const { return st->materialKey; }
#endif

inline Value Position::non_pawn_material(Color c) const { return st->nonPawnMaterial[c]; }

inline Value Position::non_pawn_material() const {
    return non_pawn_material(WHITE) + non_pawn_material(BLACK);
}

inline int Position::game_ply() const { return gamePly; }

inline int Position::rule50_count() const { return st->rule50; }

#ifdef FAIRY_STOCKFISH
inline bool Position::is_promoted(Square s) const {
    return promotedPieces & s;
}

inline Square Position::capture_square(Square to) const {
    assert(is_ok(to));
    // The capture square of en passant is either the marked ep piece or the closest piece behind the target square
    Bitboard customEp = ep_squares() & pieces();
    if (customEp)
    {
        // For longer custom en passant paths, we take the frontmost piece
        return sideToMove == WHITE ? lsb(customEp) : msb(customEp);
    }
    else
    {
        // The capture square of normal en passant is the closest piece behind the target square
        Bitboard epCandidates = pieces(~sideToMove) & forward_file_bb(~sideToMove, to);
        return sideToMove == WHITE ? msb(epCandidates) : lsb(epCandidates);
    }
}

#endif

inline bool Position::is_chess960() const { return chess960; }

inline bool Position::capture(Move m) const {
    assert(m.is_ok());
    return (!empty(m.to_sq()) && m.type_of() != CASTLING
#ifdef FAIRY_STOCKFISH
        && m.from_sq() != m.to_sq()
#endif
        ) || m.type_of() == EN_PASSANT;
}

// Returns true if a move is generated from the capture stage, having also
// queen promotions covered, i.e. consistency with the capture stage move generation
// is needed to avoid the generation of duplicate moves.
inline bool Position::capture_stage(Move m) const {
    assert(m.is_ok());
    return capture(m) || m.promotion_type() == QUEEN;
}

#ifdef FAIRY_STOCKFISH

inline Bitboard Position::fog_area() const {
    Bitboard b = board_bb();
    // Our own pieces are visible
    Bitboard visible = pieces(sideToMove);
    // Squares where we can move to are visible as well
    for (const auto& m : MoveList<LEGAL>(*this))
    {
      Square to = m.to_sq();
      visible |= to;
    }
    // Everything else is invisible
    return ~visible & b;
}

inline const std::string Position::piece_to_partner() const {
    if (!st->capturedPiece) return std::string();
    Color color = color_of(st->capturedPiece);
    Piece piece = st->capturedpromoted ?
        (st->unpromotedCapturedPiece ? st->unpromotedCapturedPiece : make_piece(color, promotion_pawn_type(color))) :
        st->capturedPiece;
    return std::string(1, piece_to_char()[piece]);
}
#endif

inline Piece Position::captured_piece() const { return st->capturedPiece; }

#ifndef FAIRY_STOCKFISH
inline void Position::put_piece(Piece pc, Square s) {

#else
inline void Position::put_piece(Piece pc, Square s, bool isPromoted, Piece unpromotedPc) {
#endif
    board[s] = pc;
    byTypeBB[ALL_PIECES] |= byTypeBB[type_of(pc)] |= s;
    byColorBB[color_of(pc)] |= s;
    pieceCount[pc]++;
    pieceCount[make_piece(color_of(pc), ALL_PIECES)]++;
#ifdef FAIRY_STOCKFISH
    if (isPromoted)
        promotedPieces |= s;
    unpromotedBoard[s] = unpromotedPc;
#endif
}

inline void Position::remove_piece(Square s) {

    Piece pc = board[s];
    byTypeBB[ALL_PIECES] ^= s;
    byTypeBB[type_of(pc)] ^= s;
    byColorBB[color_of(pc)] ^= s;
    board[s] = NO_PIECE;
    pieceCount[pc]--;
    pieceCount[make_piece(color_of(pc), ALL_PIECES)]--;
#ifdef FAIRY_STOCKFISH
    promotedPieces -= s;
    unpromotedBoard[s] = NO_PIECE;
#endif
}

inline void Position::move_piece(Square from, Square to) {

    Piece    pc     = board[from];
#ifndef FAIRY_STOCKFISH
    Bitboard fromTo = from | to;
#else
    Bitboard fromTo = square_bb(from) ^ to; // from == to needs to cancel out
#endif
    byTypeBB[ALL_PIECES] ^= fromTo;
    byTypeBB[type_of(pc)] ^= fromTo;
    byColorBB[color_of(pc)] ^= fromTo;
    board[from] = NO_PIECE;
    board[to]   = pc;
#ifdef FAIRY_STOCKFISH
    if (is_promoted(from))
        promotedPieces ^= fromTo;
    unpromotedBoard[to] = unpromotedBoard[from];
    unpromotedBoard[from] = NO_PIECE;
#endif
}

inline void Position::do_move(Move m, StateInfo& newSt) { do_move(m, newSt, gives_check(m)); }

inline StateInfo* Position::state() const { return st; }

#ifdef FAIRY_STOCKFISH
// Variant-specific

inline int Position::count_in_hand(PieceType pt) const {
    return pieceCountInHand[WHITE][pt] + pieceCountInHand[BLACK][pt];
}

inline int Position::count_in_hand(Color c, PieceType pt) const {
    return pieceCountInHand[c][pt];
}

inline int Position::count_with_hand(Color c, PieceType pt) const {
    return pieceCount[make_piece(c, pt)] + pieceCountInHand[c][pt];
}

inline bool Position::bikjang() const {
    return st->bikjang;
}

inline bool Position::allow_virtual_drop(Color c, PieceType pt) const {
    // Do we allow a virtual drop?
    return pt != KING && (   count_in_hand(c, PAWN) >= -(pt == PAWN)
                          && count_in_hand(c, KNIGHT) >= -(pt == PAWN)
                          && count_in_hand(c, BISHOP) >= -(pt == PAWN)
                          && count_in_hand(c, ROOK) >= 0
                          && count_in_hand(c, QUEEN) >= 0);
}

inline Value Position::material_counting_result() const {
    auto weight_count = [this](PieceType pt, int v){ return v * (count(WHITE, pt) - count(BLACK, pt)); };
    int materialCount;
    Value result;
    switch (var->materialCounting)
    {
    case JANGGI_MATERIAL:
        materialCount =  weight_count(ROOK, 13)
                      + weight_count(JANGGI_CANNON, 7)
                      + weight_count(HORSE, 5)
                      + weight_count(JANGGI_ELEPHANT, 3)
                      + weight_count(WAZIR, 3)
                      + weight_count(SOLDIER, 2)
                      - 1;
        result = materialCount > 0 ? VALUE_MATE : -VALUE_MATE;
        break;
    case UNWEIGHTED_MATERIAL:
        result =  count(WHITE, ALL_PIECES) > count(BLACK, ALL_PIECES) ?  VALUE_MATE
                : count(WHITE, ALL_PIECES) < count(BLACK, ALL_PIECES) ? -VALUE_MATE
                                                                      :  VALUE_DRAW;
        break;
    case WHITE_DRAW_ODDS:
        result = VALUE_MATE;
        break;
    case BLACK_DRAW_ODDS:
        result = -VALUE_MATE;
        break;
    default:
        assert(false);
        result = VALUE_DRAW;
    }
    return sideToMove == WHITE ? result : -result;
}

inline void Position::add_to_hand(Piece pc) {
    pieceCountInHand[color_of(pc)][type_of(pc)]++;
    pieceCountInHand[color_of(pc)][ALL_PIECES]++;
    // psq += PSQT::psq[pc][SQ_NONE];
}

inline void Position::remove_from_hand(Piece pc) {
    pieceCountInHand[color_of(pc)][type_of(pc)]--;
    pieceCountInHand[color_of(pc)][ALL_PIECES]--;
    // psq -= PSQT::psq[pc][SQ_NONE];
}

inline bool Position::can_drop(Color c, PieceType pt) const {
    return count_in_hand(c, pt) > 0;
}
#endif

}  // namespace Stockfish

#endif  // #ifndef POSITION_H_INCLUDED
