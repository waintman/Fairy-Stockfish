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

#include "movegen.h"

#include <cassert>
#include <initializer_list>

#include "bitboard.h"
#include "position.h"

namespace Stockfish {

namespace {

#ifdef FAIRY_STOCKFISH
template<MoveType T>
ExtMove* make_move_and_gating(const Position& pos, ExtMove* moveList, Color us, Square from, Square to, PieceType pt = NO_PIECE_TYPE) {
    *moveList++ = make<T>(from, to, pt);

    // Gating moves
    if (pos.seirawan_gating() && (pos.gates(us) & from))
        for (PieceSet ps = pos.piece_types(); ps;)
        {
            PieceType pt_gating = pop_lsb(ps);
            if (pos.can_drop(us, pt_gating) && (pos.drop_region(us, pt_gating) & from))
                *moveList++ = make_gating<T>(from, to, pt_gating, from);
        }
    if (pos.seirawan_gating() && T == CASTLING && (pos.gates(us) & to))
        for (PieceSet ps = pos.piece_types(); ps;)
        {
            PieceType pt_gating = pop_lsb(ps);
            if (pos.can_drop(us, pt_gating) && (pos.drop_region(us, pt_gating) & to))
                *moveList++ = make_gating<T>(from, to, pt_gating, to);
        }

    return moveList;
}
#endif
#ifndef FAIRY_STOCKFISH
template<GenType Type, Direction D, bool Enemy>

ExtMove* make_promotions(ExtMove* moveList, [[maybe_unused]] Square to) {
#else
template<Color c, GenType Type, Direction D, bool Enemy>
ExtMove* make_promotions(const Position& pos, ExtMove* moveList, Square to) {
#endif

    constexpr bool all = Type == EVASIONS || Type == NON_EVASIONS;

    if constexpr (Type == CAPTURES || all)
#ifndef FAIRY_STOCKFISH
        *moveList++ = Move::make<PROMOTION>(to - D, to, QUEEN);

    if constexpr ((Type == CAPTURES && Enemy) || (Type == QUIETS && !Enemy) || all)
    {
        *moveList++ = Move::make<PROMOTION>(to - D, to, ROOK);
        *moveList++ = Move::make<PROMOTION>(to - D, to, BISHOP);
        *moveList++ = Move::make<PROMOTION>(to - D, to, KNIGHT);
    }

#else
    {
        for (PieceSet promotions = pos.promotion_piece_types(c); promotions;)
        {
            PieceType pt = pop_msb(promotions);
            if (!pos.promotion_limit(pt) || pos.promotion_limit(pt) > pos.count(c, pt))
                moveList = make_move_and_gating<PROMOTION>(pos, moveList, pos.side_to_move(), to - D, to, pt);
        }
        PieceType pt = pos.promoted_piece_type(PAWN);
        if (pt && !(pos.piece_promotion_on_capture() && pos.empty(to)))
            moveList = make_move_and_gating<PIECE_PROMOTION>(pos, moveList, pos.side_to_move(), to - D, to);
    }
#endif
    return moveList;
}

#ifdef FAIRY_STOCKFISH
template<Color Us, GenType Type>
ExtMove* generate_drops(const Position& pos, ExtMove* moveList, PieceType pt, Bitboard b) {
    assert(Type != CAPTURES);
    // Do not generate virtual drops for perft and at root
    if (pos.can_drop(Us, pt))
    {
        // Restrict to valid target
        b &= pos.drop_region(Us, pt);

        // Add to move list
        if (pos.drop_promoted() && pos.promoted_piece_type(pt))
        {
            Bitboard b2 = b;
            if (Type == QUIET_CHECKS)
                b2 &= pos.check_squares(pos.promoted_piece_type(pt));
            while (b2)
                *moveList++ = make_drop(pop_lsb(b2), pt, pos.promoted_piece_type(pt));
        }
        if (Type == QUIET_CHECKS || !pos.can_drop(Us, pt))
            b &= pos.check_squares(pt);
        while (b)
            *moveList++ = make_drop(pop_lsb(b), pt, pt);
    }

    return moveList;
}
#endif

template<Color Us, GenType Type>
ExtMove* generate_pawn_moves(const Position& pos, ExtMove* moveList, Bitboard target) {

#ifdef FAIRY_STOCKFISH

    if (!pos.pieces(Us, PAWN))
        return moveList;
#endif
    constexpr Color     Them     = ~Us;
#ifndef FAIRY_STOCKFISH
    constexpr Bitboard  TRank7BB = (Us == WHITE ? Rank7BB : Rank2BB);
    constexpr Bitboard  TRank3BB = (Us == WHITE ? Rank3BB : Rank6BB);
#endif
    constexpr Direction Up       = pawn_push(Us);
    constexpr Direction UpRight  = (Us == WHITE ? NORTH_EAST : SOUTH_WEST);
    constexpr Direction UpLeft   = (Us == WHITE ? NORTH_WEST : SOUTH_EAST);

#ifndef FAIRY_STOCKFISH
    const Bitboard emptySquares = ~pos.pieces();
    const Bitboard enemies      = Type == EVASIONS ? pos.checkers() : pos.pieces(Them);

    Bitboard pawnsOn7    = pos.pieces(Us, PAWN) & TRank7BB;
    Bitboard pawnsNotOn7 = pos.pieces(Us, PAWN) & ~TRank7BB;
#else
    const Bitboard promotionZone = pos.promotion_zone(Us);
    const Bitboard standardPromotionZone = pos.sittuyin_promotion() ? Bitboard(0) : promotionZone;
    const Bitboard doubleStepRegion = pos.double_step_region(Us);
    const Bitboard tripleStepRegion = pos.triple_step_region(Us);

    const Bitboard pawns      = pos.pieces(Us, PAWN);
    const Bitboard movable    = pos.board_bb(Us, PAWN) & ~pos.pieces();
    const Bitboard capturable = pos.board_bb(Us, PAWN) &  pos.pieces(Them);

    target = Type == EVASIONS ? target : AllSquares;

    // Define single and double push, left and right capture, as well as respective promotion moves
    Bitboard b1 = shift<Up>(pawns) & movable & target;
    Bitboard b2 = shift<Up>(shift<Up>(pawns & doubleStepRegion) & movable) & movable & target;
    Bitboard b3 = shift<Up>(shift<Up>(shift<Up>(pawns & tripleStepRegion) & movable) & movable) & movable & target;
    Bitboard brc = shift<UpRight>(pawns) & capturable & target;
    Bitboard blc = shift<UpLeft >(pawns) & capturable & target;

    Bitboard b1p = b1 & standardPromotionZone;
    Bitboard b2p = b2 & standardPromotionZone;
    Bitboard b3p = b3 & standardPromotionZone;
    Bitboard brcp = brc & standardPromotionZone;
    Bitboard blcp = blc & standardPromotionZone;

    // Restrict regions based on rules and move generation type
    if (pos.mandatory_pawn_promotion())
    {
        b1 &= ~standardPromotionZone;
        b2 &= ~standardPromotionZone;
        b3 &= ~standardPromotionZone;
        brc &= ~standardPromotionZone;
        blc &= ~standardPromotionZone;
    }

    if (Type == QUIET_CHECKS && pos.count<KING>(Them))
    {
        // To make a quiet check, you either make a direct check by pushing a pawn
        // or push a blocker pawn that is not on the same file as the enemy king.
        // Discovered check promotion has been already generated amongst the captures.
        Square ksq = pos.square<KING>(Them);
        Bitboard dcCandidatePawns = pos.blockers_for_king(Them) & ~file_bb(ksq);
        b1 &= pawn_attacks_bb(Them, ksq) | shift<   Up>(dcCandidatePawns);
        b2 &= pawn_attacks_bb(Them, ksq) | shift<Up+Up>(dcCandidatePawns);
    }
#endif

    // Single and double pawn pushes, no promotions
    if constexpr (Type != CAPTURES)
    {
#ifndef FAIRY_STOCKFISH
        Bitboard b1 = shift<Up>(pawnsNotOn7) & emptySquares;
        Bitboard b2 = shift<Up>(b1 & TRank3BB) & emptySquares;

        if constexpr (Type == EVASIONS)  // Consider only blocking squares
        {
            b1 &= target;
            b2 &= target;
        }

        if constexpr (Type == QUIET_CHECKS)
        {
            // To make a quiet check, you either make a direct check by pushing a pawn
            // or push a blocker pawn that is not on the same file as the enemy king.
            // Discovered check promotion has been already generated amongst the captures.
            Square   ksq              = pos.square<KING>(Them);
            Bitboard dcCandidatePawns = pos.blockers_for_king(Them) & ~file_bb(ksq);
            b1 &= pawn_attacks_bb(Them, ksq) | shift<Up>(dcCandidatePawns);
            b2 &= pawn_attacks_bb(Them, ksq) | shift<Up + Up>(dcCandidatePawns);
        }

#endif
        while (b1)
        {
            Square to   = pop_lsb(b1);
#ifndef FAIRY_STOCKFISH
            *moveList++ = Move(to - Up, to);
#else
            moveList = make_move_and_gating<NORMAL>(pos, moveList, Us, to - Up, to);
#endif
        }

        while (b2)
        {
            Square to   = pop_lsb(b2);
#ifndef FAIRY_STOCKFISH
            *moveList++ = Move(to - Up - Up, to);
        }
#else
            moveList = make_move_and_gating<NORMAL>(pos, moveList, Us, to - Up - Up, to);
        }

        while (b3)
        {
            Square to = pop_lsb(b3);
            moveList = make_move_and_gating<NORMAL>(pos, moveList, Us, to - Up - Up - Up, to);
        }
#endif
    }

    // Promotions and underpromotions
#ifndef FAIRY_STOCKFISH
    if (pawnsOn7)
    {
        Bitboard b1 = shift<UpRight>(pawnsOn7) & enemies;
        Bitboard b2 = shift<UpLeft>(pawnsOn7) & enemies;
        Bitboard b3 = shift<Up>(pawnsOn7) & emptySquares;

        if constexpr (Type == EVASIONS)
            b3 &= target;

        while (b1)
            moveList = make_promotions<Type, UpRight, true>(moveList, pop_lsb(b1));

        while (b2)
            moveList = make_promotions<Type, UpLeft, true>(moveList, pop_lsb(b2));

        while (b3)
            moveList = make_promotions<Type, Up, false>(moveList, pop_lsb(b3));
    }
#else
    while (brcp)
        moveList = make_promotions<Us, Type, UpRight, true>(pos, moveList, pop_lsb(brcp));

    while (blcp)
        moveList = make_promotions<Us, Type, UpLeft, true>(pos, moveList, pop_lsb(blcp));

    while (b1p)
        moveList = make_promotions<Us, Type, Up,     false>(pos, moveList, pop_lsb(b1p));

    while (b2p)
        moveList = make_promotions<Us, Type, Up+Up,  true>(pos, moveList, pop_lsb(b2p));

    while (b3p)
        moveList = make_promotions<Us, Type, Up+Up+Up, true>(pos, moveList, pop_lsb(b3p));

    // Sittuyin promotions
    if (pos.sittuyin_promotion() && (Type == CAPTURES || Type == EVASIONS || Type == NON_EVASIONS))
    {
        // Pawns need to be in promotion zone if there is more than one pawn
        Bitboard promotionPawns = pos.count<PAWN>(Us) > 1 ? pawns & promotionZone : pawns;
        while (promotionPawns)
        {
            Square from = pop_lsb(promotionPawns);
            for (PieceSet ps = pos.promotion_piece_types(Us); ps;)
            {
                PieceType pt = pop_msb(ps);
                if (pos.promotion_limit(pt) && pos.promotion_limit(pt) <= pos.count(Us, pt))
                    continue;
                Bitboard b = ((pos.attacks_from(Us, pt, from) & ~pos.pieces()) | from) & target;
                while (b)
                {
                    Square to = pop_lsb(b);
                    if (!(attacks_bb(Us, pt, to, pos.pieces() ^ from) & pos.pieces(Them)))
                        *moveList++ = make<PROMOTION>(from, to, pt);
                }
            }
        }
    }
#endif

    // Standard and en passant captures
    if constexpr (Type == CAPTURES || Type == EVASIONS || Type == NON_EVASIONS)
    {
#ifndef FAIRY_STOCKFISH
        Bitboard b1 = shift<UpRight>(pawnsNotOn7) & enemies;
        Bitboard b2 = shift<UpLeft>(pawnsNotOn7) & enemies;

        while (b1)
        {
            Square to   = pop_lsb(b1);
            *moveList++ = Move(to - UpRight, to);
        }

        while (b2)
        {
            Square to   = pop_lsb(b2);
            *moveList++ = Move(to - UpLeft, to);
        }

        if (pos.ep_square() != SQ_NONE)
        {
            assert(rank_of(pos.ep_square()) == relative_rank(Us, RANK_6));

            // An en passant capture cannot resolve a discovered check
            if (Type == EVASIONS && (target & (pos.ep_square() + Up)))
                return moveList;

            b1 = pawnsNotOn7 & pawn_attacks_bb(Them, pos.ep_square());

            assert(b1);

            while (b1)
                *moveList++ = Move::make<EN_PASSANT>(pop_lsb(b1), pos.ep_square());
        }
#else
        while (brc)
        {
            Square to = pop_lsb(brc);
            moveList = make_move_and_gating<NORMAL>(pos, moveList, Us, to - UpRight, to);
        }

        while (blc)
        {
            Square to = pop_lsb(blc);
            moveList = make_move_and_gating<NORMAL>(pos, moveList, Us, to - UpLeft, to);
        }

        for (Bitboard epSquares = pos.ep_squares() & ~pos.pieces(); epSquares; )
        {
            Square epSquare = pop_lsb(epSquares);

            // An en passant capture cannot resolve a discovered check (unless there non-sliding riders)
            if (Type == EVASIONS && (target & (epSquare + Up)) && !pos.non_sliding_riders())
                return moveList;

            Bitboard b = pawns & pawn_attacks_bb(Them, epSquare);

            // En passant square is already disabled for non-fairy variants if there is no attacker
            assert(b || !pos.variant()->fastAttacks);

            while (b)
                moveList = make_move_and_gating<EN_PASSANT>(pos, moveList, Us, pop_lsb(b), epSquare);
        }
#endif
    }

    return moveList;
}


#ifndef FAIRY_STOCKFISH
template<Color Us, PieceType Pt, bool Checks>
ExtMove* generate_moves(const Position& pos, ExtMove* moveList, Bitboard target) {

    static_assert(Pt != KING && Pt != PAWN, "Unsupported piece type in generate_moves()");
#else
template<Color Us, GenType Type>
ExtMove* generate_moves(const Position& pos, ExtMove* moveList, PieceType Pt, Bitboard target) {

    assert(Pt != KING && Pt != PAWN);
#endif

    Bitboard bb = pos.pieces(Us, Pt);

    while (bb)
    {
        Square   from = pop_lsb(bb);
#ifndef FAIRY_STOCKFISH
        Bitboard b    = attacks_bb<Pt>(from, pos.pieces()) & target;

        // To check, you either move freely a blocker or make a direct check.
        if (Checks && (Pt == QUEEN || !(pos.blockers_for_king(~Us) & from)))
            b &= pos.check_squares(Pt);

        while (b)
            *moveList++ = Move(from, pop_lsb(b));
#else

        Bitboard attacks = pos.attacks_from(Us, Pt, from);
        Bitboard quiets = pos.moves_from(Us, Pt, from);
        Bitboard b = (  (attacks & pos.pieces())
                       | (quiets & ~pos.pieces()));
        Bitboard b1 = b & target;
        Bitboard promotion_zone = pos.promotion_zone(Us);
        PieceType promPt = pos.promoted_piece_type(Pt);
        Bitboard b2 = promPt && (!pos.promotion_limit(promPt) || pos.promotion_limit(promPt) > pos.count(Us, promPt)) ? b1 : Bitboard(0);
        Bitboard b3 = pos.piece_demotion() && pos.is_promoted(from) ? b1 : Bitboard(0);
        Bitboard pawnPromotions = pos.variant()->promotionPawnTypes[Us] & Pt ? b & (Type == EVASIONS ? target : ~pos.pieces(Us)) & promotion_zone : Bitboard(0);
        Bitboard epSquares = pos.variant()->enPassantTypes[Us] & Pt ? attacks & ~quiets & pos.ep_squares() & ~pos.pieces() : Bitboard(0);

        // target squares considering pawn promotions
        if (pawnPromotions && pos.mandatory_pawn_promotion())
            b1 &= ~pawnPromotions;

        // Restrict target squares considering promotion zone
        if (b2 | b3)
        {
            if (pos.mandatory_piece_promotion())
                b1 &= (promotion_zone & from ? Bitboard(0) : ~promotion_zone) | (pos.piece_promotion_on_capture() ? ~pos.pieces() : Bitboard(0));
            // Exclude quiet promotions/demotions
            if (pos.piece_promotion_on_capture())
            {
                b2 &= pos.pieces();
                b3 &= pos.pieces();
            }
            // Consider promotions/demotions into promotion zone
            if (!(promotion_zone & from))
            {
                b2 &= promotion_zone;
                b3 &= promotion_zone;
            }
        }

        if (Type == QUIET_CHECKS)
        {
            b1 &= pos.check_squares(Pt);
            if (b2)
                b2 &= pos.check_squares(pos.promoted_piece_type(Pt));
            if (b3)
                b3 &= pos.check_squares(type_of(pos.unpromoted_piece_on(from)));
        }

        while (b1)
            moveList = make_move_and_gating<NORMAL>(pos, moveList, Us, from, pop_lsb(b1));

        // Shogi-style piece promotions
        while (b2)
            *moveList++ = make<PIECE_PROMOTION>(from, pop_lsb(b2));

        // Piece demotions
        while (b3)
            *moveList++ = make<PIECE_DEMOTION>(from, pop_lsb(b3));

        // Pawn-style promotions
        if ((Type == CAPTURES || Type == EVASIONS || Type == NON_EVASIONS) && pawnPromotions)
            for (PieceSet ps = pos.promotion_piece_types(Us); ps;)
            {
                PieceType ptP = pop_msb(ps);
                if (!pos.promotion_limit(ptP) || pos.promotion_limit(ptP) > pos.count(Us, ptP))
                    for (Bitboard promotions = pawnPromotions; promotions; )
                        moveList = make_move_and_gating<PROMOTION>(pos, moveList, pos.side_to_move(), from, pop_lsb(promotions), ptP);
            }

        // En passant captures
        if (Type == CAPTURES || Type == EVASIONS || Type == NON_EVASIONS)
            while (epSquares)
                moveList = make_move_and_gating<EN_PASSANT>(pos, moveList, Us, from, pop_lsb(epSquares));
#endif
    }

    return moveList;
}


template<Color Us, GenType Type>
ExtMove* generate_all(const Position& pos, ExtMove* moveList) {

    static_assert(Type != LEGAL, "Unsupported type in generate_all()");

    constexpr bool Checks = Type == QUIET_CHECKS;  // Reduce template instantiations
#ifndef FAIRY_STOCKFISH
    const Square   ksq    = pos.square<KING>(Us);
#else
    const Square ksq = pos.count<KING>(Us) ? pos.square<KING>(Us) : SQ_NONE;
#endif
    Bitboard       target;

    // Skip generating non-king moves when in double check
#ifndef FAIRY_STOCKFISH
    if (Type != EVASIONS || !more_than_one(pos.checkers()))
    {
        target = Type == EVASIONS     ? between_bb(ksq, lsb(pos.checkers()))
               : Type == NON_EVASIONS ? ~pos.pieces(Us)
               : Type == CAPTURES     ? pos.pieces(~Us)
                                      : ~pos.pieces();  // QUIETS || QUIET_CHECKS

        moveList = generate_pawn_moves<Us, Type>(pos, moveList, target);
        moveList = generate_moves<Us, KNIGHT, Checks>(pos, moveList, target);
        moveList = generate_moves<Us, BISHOP, Checks>(pos, moveList, target);
        moveList = generate_moves<Us, ROOK, Checks>(pos, moveList, target);
        moveList = generate_moves<Us, QUEEN, Checks>(pos, moveList, target);
    }

#else
    if (Type != EVASIONS || !more_than_one(pos.checkers() & ~pos.non_sliding_riders()))
    {
        target = Type == EVASIONS     ?  between_bb(ksq, lsb(pos.checkers()))
               : Type == NON_EVASIONS ? ~pos.pieces( Us)
               : Type == CAPTURES     ?  pos.pieces(~Us)
                                      : ~pos.pieces(   ); // QUIETS || QUIET_CHECKS

        if (Type == EVASIONS)
        {
            if (pos.checkers() & pos.non_sliding_riders())
                target = ~pos.pieces(Us);
            // Leaper attacks can not be blocked
            Square checksq = lsb(pos.checkers());
            if (LeaperAttacks[~Us][type_of(pos.piece_on(checksq))][checksq] & pos.square<KING>(Us))
                target = pos.checkers();
        }

        // Remove inaccessible squares (outside board + wall squares)
        target &= pos.board_bb();

        moveList = generate_pawn_moves<Us, Type>(pos, moveList, target);
        for (PieceSet ps = pos.piece_types() & ~(piece_set(PAWN) | KING); ps;)
            moveList = generate_moves<Us, Type>(pos, moveList, pop_lsb(ps), target);
        // generate drops
        if (pos.piece_drops() && Type != CAPTURES && pos.can_drop(Us, ALL_PIECES))
            for (PieceSet ps = pos.piece_types(); ps;)
                moveList = generate_drops<Us, Type>(pos, moveList, pop_lsb(ps), target & ~pos.pieces(~Us));

        // Castling with non-king piece
        if (!pos.count<KING>(Us) && Type != CAPTURES && pos.can_castle(Us & ANY_CASTLING))
        {
            Square from = pos.castling_king_square(Us);
            for(CastlingRights cr : { Us & KING_SIDE, Us & QUEEN_SIDE } )
                if (!pos.castling_impeded(cr) && pos.can_castle(cr))
                    moveList = make_move_and_gating<CASTLING>(pos, moveList, Us, from, pos.castling_rook_square(cr));
        }

        // Special moves
        if (pos.cambodian_moves() && pos.gates(Us) && Type != CAPTURES)
        {
            if (Type != EVASIONS && (pos.pieces(Us, KING) & pos.gates(Us)))
            {
                Square from = pos.square<KING>(Us);
                Bitboard b = PseudoAttacks[WHITE][KNIGHT][from] & rank_bb(rank_of(from + (Us == WHITE ? NORTH : SOUTH)))
                    & target & ~pos.pieces();
                while (b)
                    moveList = make_move_and_gating<SPECIAL>(pos, moveList, Us, from, pop_lsb(b));
            }

            Bitboard b = pos.pieces(Us, FERS) & pos.gates(Us);
            while (b)
            {
                Square from = pop_lsb(b);
                Square to = from + 2 * (Us == WHITE ? NORTH : SOUTH);
                if (is_ok(to) && (target & to & ~pos.pieces()))
                    moveList = make_move_and_gating<SPECIAL>(pos, moveList, Us, from, to);
            }
        }

        // Workaround for passing: Execute a non-move with any piece
        if (pos.pass(Us) && !pos.count<KING>(Us) && pos.pieces(Us))
            *moveList++ = make<SPECIAL>(lsb(pos.pieces(Us)), lsb(pos.pieces(Us)));

    }
#endif

#ifndef FAIRY_STOCKFISH
    if (!Checks || pos.blockers_for_king(~Us) & ksq)
    {
        Bitboard b = attacks_bb<KING>(ksq) & (Type == EVASIONS ? ~pos.pieces(Us) : target);
        if (Checks)
            b &= ~attacks_bb<QUEEN>(pos.square<KING>(~Us));

#else
    // King moves
    if (pos.count<KING>(Us) && (!Checks || pos.blockers_for_king(~Us) & ksq))
    {
        Bitboard b = (  (pos.attacks_from(Us, KING, ksq) & pos.pieces())
                      | (pos.moves_from(Us, KING, ksq) & ~pos.pieces())) & (Type == EVASIONS ? ~pos.pieces(Us) : target);
#endif
        while (b)
#ifndef FAIRY_STOCKFISH
            *moveList++ = Move(ksq, pop_lsb(b));
#else
            moveList = make_move_and_gating<NORMAL>(pos, moveList, Us, ksq, pop_lsb(b));

        // Passing move by king
        if (pos.pass(Us))
            *moveList++ = make<SPECIAL>(ksq, ksq);
#endif

        if ((Type == QUIETS || Type == NON_EVASIONS) && pos.can_castle(Us & ANY_CASTLING))
            for (CastlingRights cr : {Us & KING_SIDE, Us & QUEEN_SIDE})
                if (!pos.castling_impeded(cr) && pos.can_castle(cr))
#ifndef FAIRY_STOCKFISH
                    *moveList++ = Move::make<CASTLING>(ksq, pos.castling_rook_square(cr));
#else
                    moveList = make_move_and_gating<CASTLING>(pos, moveList, Us,ksq, pos.castling_rook_square(cr));
#endif
    }

    return moveList;
}

}  // namespace


// <CAPTURES>     Generates all pseudo-legal captures plus queen promotions
// <QUIETS>       Generates all pseudo-legal non-captures and underpromotions
// <EVASIONS>     Generates all pseudo-legal check evasions
// <NON_EVASIONS> Generates all pseudo-legal captures and non-captures
// <QUIET_CHECKS> Generates all pseudo-legal non-captures giving check,
//                except castling and promotions
//
// Returns a pointer to the end of the move list.
template<GenType Type>
ExtMove* generate(const Position& pos, ExtMove* moveList) {

    static_assert(Type != LEGAL, "Unsupported type in generate()");
    assert((Type == EVASIONS) == bool(pos.checkers()));

    Color us = pos.side_to_move();

    return us == WHITE ? generate_all<WHITE, Type>(pos, moveList)
                       : generate_all<BLACK, Type>(pos, moveList);
}

// Explicit template instantiations
template ExtMove* generate<CAPTURES>(const Position&, ExtMove*);
template ExtMove* generate<QUIETS>(const Position&, ExtMove*);
template ExtMove* generate<EVASIONS>(const Position&, ExtMove*);
template ExtMove* generate<QUIET_CHECKS>(const Position&, ExtMove*);
template ExtMove* generate<NON_EVASIONS>(const Position&, ExtMove*);


// generate<LEGAL> generates all the legal moves in the given position

template<>
ExtMove* generate<LEGAL>(const Position& pos, ExtMove* moveList) {

#ifndef FAIRY_STOCKFISH
    Color    us     = pos.side_to_move();
    Bitboard pinned = pos.blockers_for_king(us) & pos.pieces(us);
    Square   ksq    = pos.square<KING>(us);
#else
  if (pos.is_immediate_game_end())
      return moveList;

#endif
    ExtMove* cur    = moveList;

    moveList =
      pos.checkers() ? generate<EVASIONS>(pos, moveList) : generate<NON_EVASIONS>(pos, moveList);
    while (cur != moveList)
#ifndef FAIRY_STOCKFISH
        if (((pinned & cur->from_sq()) || cur->from_sq() == ksq || cur->type_of() == EN_PASSANT)
            && !pos.legal(*cur))
#else
      if (!pos.legal(*cur) || pos.virtual_drop(*cur))
#endif
            *cur = *(--moveList);
        else
            ++cur;

    return moveList;
}

}  // namespace Stockfish
