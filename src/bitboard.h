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

#ifndef BITBOARD_H_INCLUDED
#define BITBOARD_H_INCLUDED

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <string>

#include "types.h"

namespace Stockfish {

namespace Bitboards {

#ifdef FAIRY_STOCKFISH
void init_pieces();
#endif
void        init();
std::string pretty(Bitboard b);

}  // namespace Stockfish::Bitboards

#ifdef LARGEBOARDS
constexpr Bitboard AllSquares = ((~Bitboard(0)) >> 8);
constexpr Bitboard DarkSquares = (Bitboard(0xAAA555AAA555AAULL) << 64) ^ Bitboard(0xA555AAA555AAA555ULL);
constexpr Bitboard FileABB = (Bitboard(0x00100100100100ULL) << 64) ^ Bitboard(0x1001001001001001ULL);
#else
constexpr Bitboard FileABB = 0x0101010101010101ULL;
#endif

constexpr Bitboard FileBBB = FileABB << 1;
constexpr Bitboard FileCBB = FileABB << 2;
constexpr Bitboard FileDBB = FileABB << 3;
constexpr Bitboard FileEBB = FileABB << 4;
constexpr Bitboard FileFBB = FileABB << 5;
constexpr Bitboard FileGBB = FileABB << 6;
constexpr Bitboard FileHBB = FileABB << 7;

#ifdef LARGEBOARDS
constexpr Bitboard FileIBB = FileABB << 8;
constexpr Bitboard FileJBB = FileABB << 9;
constexpr Bitboard FileKBB = FileABB << 10;
constexpr Bitboard FileLBB = FileABB << 11;
#endif


#ifdef LARGEBOARDS
constexpr Bitboard Rank1BB = 0xFFF;
#else
constexpr Bitboard Rank1BB = 0xFF;
#endif
#ifndef FAIRY_STOCKFISH
constexpr Bitboard Rank2BB = Rank1BB << (8 * 1);
constexpr Bitboard Rank3BB = Rank1BB << (8 * 2);
constexpr Bitboard Rank4BB = Rank1BB << (8 * 3);
constexpr Bitboard Rank5BB = Rank1BB << (8 * 4);
constexpr Bitboard Rank6BB = Rank1BB << (8 * 5);
constexpr Bitboard Rank7BB = Rank1BB << (8 * 6);
constexpr Bitboard Rank8BB = Rank1BB << (8 * 7);
#else
constexpr Bitboard Rank2BB = Rank1BB << (FILE_NB * 1);
constexpr Bitboard Rank3BB = Rank1BB << (FILE_NB * 2);
constexpr Bitboard Rank4BB = Rank1BB << (FILE_NB * 3);
constexpr Bitboard Rank5BB = Rank1BB << (FILE_NB * 4);
constexpr Bitboard Rank6BB = Rank1BB << (FILE_NB * 5);
constexpr Bitboard Rank7BB = Rank1BB << (FILE_NB * 6);
constexpr Bitboard Rank8BB = Rank1BB << (FILE_NB * 7);
#endif
#ifdef LARGEBOARDS
constexpr Bitboard Rank9BB = Rank1BB << (FILE_NB * 8);
constexpr Bitboard Rank10BB = Rank1BB << (FILE_NB * 9);
#endif

extern uint8_t PopCnt16[1 << 16];
extern uint8_t SquareDistance[SQUARE_NB][SQUARE_NB];

extern Bitboard BetweenBB[SQUARE_NB][SQUARE_NB];
extern Bitboard LineBB[SQUARE_NB][SQUARE_NB];
#ifndef FAIRY_STOCKFISH
extern Bitboard PseudoAttacks[PIECE_TYPE_NB][SQUARE_NB];
extern Bitboard PawnAttacks[COLOR_NB][SQUARE_NB];

#else
extern Bitboard PseudoAttacks[COLOR_NB][PIECE_TYPE_NB][SQUARE_NB];
extern Bitboard PseudoMoves[2][COLOR_NB][PIECE_TYPE_NB][SQUARE_NB];
extern Bitboard LeaperAttacks[COLOR_NB][PIECE_TYPE_NB][SQUARE_NB];
extern Bitboard LeaperMoves[2][COLOR_NB][PIECE_TYPE_NB][SQUARE_NB];
extern Bitboard SquareBB[SQUARE_NB];
extern Bitboard BoardSizeBB[FILE_NB][RANK_NB];
extern RiderType AttackRiderTypes[PIECE_TYPE_NB];
extern RiderType MoveRiderTypes[2][PIECE_TYPE_NB];
#endif

#ifdef LARGEBOARDS
int popcount(Bitboard b); // required for 128 bit pext
#endif

// Magic holds all magic bitboards relevant data for a single square
struct Magic {
    Bitboard  mask;
    Bitboard  magic;
    Bitboard* attacks;
    unsigned  shift;

    // Compute the attack's index using the 'magic bitboards' approach
    unsigned index(Bitboard occupied) const {

        if (HasPext)
            return unsigned(pext(occupied, mask));

#ifdef LARGEBOARDS
        return unsigned(((occupied & mask) * magic) >> shift);
#else
        if (Is64Bit)
            return unsigned(((occupied & mask) * magic) >> shift);
#endif

        unsigned lo = unsigned(occupied) & unsigned(mask);
        unsigned hi = unsigned(occupied >> 32) & unsigned(mask >> 32);
        return (lo * unsigned(magic) ^ hi * unsigned(magic >> 32)) >> shift;
    }
};

#ifndef FAIRY_STOCKFISH
extern Magic RookMagics[SQUARE_NB];
extern Magic BishopMagics[SQUARE_NB];
#else
extern Magic RookMagicsH[SQUARE_NB];
extern Magic RookMagicsV[SQUARE_NB];
extern Magic BishopMagics[SQUARE_NB];
extern Magic CannonMagicsH[SQUARE_NB];
extern Magic CannonMagicsV[SQUARE_NB];
extern Magic LameDabbabaMagics[SQUARE_NB];
extern Magic HorseMagics[SQUARE_NB];
extern Magic ElephantMagics[SQUARE_NB];
extern Magic JanggiElephantMagics[SQUARE_NB];
extern Magic CannonDiagMagics[SQUARE_NB];
extern Magic NightriderMagics[SQUARE_NB];
extern Magic GrasshopperMagicsH[SQUARE_NB];
extern Magic GrasshopperMagicsV[SQUARE_NB];
extern Magic GrasshopperMagicsD[SQUARE_NB];

extern Magic* magics[];

constexpr Bitboard make_bitboard() { return 0; }

template<typename ...Squares>
constexpr Bitboard make_bitboard(Square s, Squares... squares) {
  return (Bitboard(1) << s) | make_bitboard(squares...);
}
#endif

constexpr Bitboard square_bb(Square s) {
    assert(is_ok(s));
#ifndef FAIRY_STOCKFISH
    return (1ULL << s);
#else
    return SquareBB[s];
#endif
}


// Overloads of bitwise operators between a Bitboard and a Square for testing
// whether a given bit is set in a bitboard, and for setting and clearing bits.

inline Bitboard  operator&(Bitboard b, Square s) { return b & square_bb(s); }
inline Bitboard  operator|(Bitboard b, Square s) { return b | square_bb(s); }
inline Bitboard  operator^(Bitboard b, Square s) { return b ^ square_bb(s); }
inline Bitboard& operator|=(Bitboard& b, Square s) { return b |= square_bb(s); }
inline Bitboard& operator^=(Bitboard& b, Square s) { return b ^= square_bb(s); }

#ifdef FAIRY_STOCKFISH
inline Bitboard  operator-( Bitboard  b, Square s) { return b & ~square_bb(s); }
inline Bitboard& operator-=(Bitboard& b, Square s) { return b &= ~square_bb(s); }
#endif
inline Bitboard operator&(Square s, Bitboard b) { return b & s; }
inline Bitboard operator|(Square s, Bitboard b) { return b | s; }
inline Bitboard operator^(Square s, Bitboard b) { return b ^ s; }

inline Bitboard operator|(Square s1, Square s2) { return square_bb(s1) | s2; }

constexpr bool more_than_one(Bitboard b) { return b & (b - 1); }


#ifdef FAIRY_STOCKFISH
inline Bitboard undo_move_board(Bitboard b, Move m) {
  return (from_sq(m) != SQ_NONE && (b & to_sq(m))) ? (b ^ to_sq(m)) | from_sq(m) : b;
}

/// board_size_bb() returns a bitboard representing all the squares
/// on a board with given size.

inline Bitboard board_size_bb(File f, Rank r) {
  return BoardSizeBB[f][r];
}
#endif

// rank_bb() and file_bb() return a bitboard representing all the squares on
// the given file or rank.

#ifndef FAIRY_STOCKFISH
constexpr Bitboard rank_bb(Rank r) { return Rank1BB << (8 * r); }

#else
constexpr Bitboard rank_bb(Rank r) { return Rank1BB << (FILE_NB * r); }
#endif
constexpr Bitboard rank_bb(Square s) { return rank_bb(rank_of(s)); }

constexpr Bitboard file_bb(File f) { return FileABB << f; }

constexpr Bitboard file_bb(Square s) { return file_bb(file_of(s)); }


// Moves a bitboard one or two steps as specified by the direction D
template<Direction D>
constexpr Bitboard shift(Bitboard b) {
#ifndef FAIRY_STOCKFISH
    return D == NORTH         ? b << 8
         : D == SOUTH         ? b >> 8
         : D == NORTH + NORTH ? b << 16
         : D == SOUTH + SOUTH ? b >> 16
         : D == EAST          ? (b & ~FileHBB) << 1
         : D == WEST          ? (b & ~FileABB) >> 1
         : D == NORTH_EAST    ? (b & ~FileHBB) << 9
         : D == NORTH_WEST    ? (b & ~FileABB) << 7
         : D == SOUTH_EAST    ? (b & ~FileHBB) >> 7
         : D == SOUTH_WEST    ? (b & ~FileABB) >> 9
                              : 0;
#else
    return D == NORTH         ? b << NORTH
         : D == SOUTH         ? b >> NORTH
         : D == NORTH + NORTH ? b << (2 * NORTH)
         : D == SOUTH + SOUTH ? b >> (2 * NORTH)
         : D == EAST          ? (b & ~file_bb(FILE_MAX)) << EAST
         : D == WEST          ? (b & ~FileABB) >> EAST
         : D == NORTH_EAST    ? (b & ~file_bb(FILE_MAX)) << NORTH_EAST
         : D == NORTH_WEST    ? (b & ~FileABB) << NORTH_WEST
         : D == SOUTH_EAST    ? (b & ~file_bb(FILE_MAX)) >> NORTH_WEST
         : D == SOUTH_WEST    ? (b & ~FileABB) >> NORTH_EAST
                              : Bitboard(0);
#endif
}

#ifdef FAIRY_STOCKFISH
/// shift() moves a bitboard one step along direction D (mainly for pawns)

constexpr Bitboard shift(Direction D, Bitboard b) {
  return  D == NORTH      ?  b                       << NORTH      : D == SOUTH      ?  b             >> NORTH
        : D == NORTH+NORTH?  b                       <<(2 * NORTH) : D == SOUTH+SOUTH?  b             >> (2 * NORTH)
        : D == EAST       ? (b & ~file_bb(FILE_MAX)) << EAST       : D == WEST       ? (b & ~FileABB) >> EAST
        : D == NORTH_EAST ? (b & ~file_bb(FILE_MAX)) << NORTH_EAST : D == NORTH_WEST ? (b & ~FileABB) << NORTH_WEST
        : D == SOUTH_EAST ? (b & ~file_bb(FILE_MAX)) >> NORTH_WEST : D == SOUTH_WEST ? (b & ~FileABB) >> NORTH_EAST
        : Bitboard(0);
}

#endif

// Returns the squares attacked by pawns of the given color
// from the squares in the given bitboard.
template<Color C>
constexpr Bitboard pawn_attacks_bb(Bitboard b) {
    return C == WHITE ? shift<NORTH_WEST>(b) | shift<NORTH_EAST>(b)
                      : shift<SOUTH_WEST>(b) | shift<SOUTH_EAST>(b);
}

inline Bitboard pawn_attacks_bb(Color c, Square s) {

    assert(is_ok(s));
#ifndef FAIRY_STOCKFISH
    return PawnAttacks[c][s];
#else
  return PseudoAttacks[c][PAWN][s];
#endif
}

// Returns a bitboard representing an entire line (from board edge
// to board edge) that intersects the two given squares. If the given squares
// are not on a same file/rank/diagonal, the function returns 0. For instance,
// line_bb(SQ_C4, SQ_F7) will return a bitboard with the A2-G8 diagonal.
inline Bitboard line_bb(Square s1, Square s2) {

    assert(is_ok(s1) && is_ok(s2));
    return LineBB[s1][s2];
}


// Returns a bitboard representing the squares in the semi-open
// segment between the squares s1 and s2 (excluding s1 but including s2). If the
// given squares are not on a same file/rank/diagonal, it returns s2. For instance,
// between_bb(SQ_C4, SQ_F7) will return a bitboard with squares D5, E6 and F7, but
// between_bb(SQ_E6, SQ_F8) will return a bitboard with the square F8. This trick
// allows to generate non-king evasion moves faster: the defending piece must either
// interpose itself to cover the check or capture the checking piece.
inline Bitboard between_bb(Square s1, Square s2) {

    assert(is_ok(s1) && is_ok(s2));
    return BetweenBB[s1][s2];
}

#ifdef FAIRY_STOCKFISH
inline Bitboard between_bb(Square s1, Square s2, PieceType pt) {
  if (pt == HORSE)
      return PseudoAttacks[WHITE][WAZIR][s2] & PseudoAttacks[WHITE][FERS][s1];
  else if (pt == JANGGI_ELEPHANT)
      return  (PseudoAttacks[WHITE][WAZIR][s2] & PseudoAttacks[WHITE][ALFIL][s1])
            | (PseudoAttacks[WHITE][KNIGHT][s2] & PseudoAttacks[WHITE][FERS][s1]);
  else
      return between_bb(s1, s2);
}

#endif
// Returns true if the squares s1, s2 and s3 are aligned either on a
// straight or on a diagonal line.
inline bool aligned(Square s1, Square s2, Square s3) { return line_bb(s1, s2) & s3; }

#ifdef FAIRY_STOCKFISH
constexpr Bitboard forward_ranks_bb(Color c, Square s) {
  return c == WHITE ? (AllSquares ^ Rank1BB) << FILE_NB * relative_rank(WHITE, s, RANK_MAX)
                    : (AllSquares ^ rank_bb(RANK_MAX)) >> FILE_NB * relative_rank(BLACK, s, RANK_MAX);
}

constexpr Bitboard forward_ranks_bb(Color c, Rank r) {
  return c == WHITE ? (AllSquares ^ Rank1BB) << FILE_NB * (r - RANK_1)
                    : (AllSquares ^ rank_bb(RANK_MAX)) >> FILE_NB * (RANK_MAX - r);
}


/// zone_bb() returns a bitboard representing the squares on all the ranks
/// in front of and on the given relative rank, from the point of view of the given color.
/// For instance, zone_bb(BLACK, RANK_7) will return the 16 squares on ranks 1 and 2.

inline Bitboard zone_bb(Color c, Rank r, Rank maxRank) {
  return forward_ranks_bb(c, relative_rank(c, r, maxRank)) | rank_bb(relative_rank(c, r, maxRank));
}

/// forward_file_bb() returns a bitboard representing all the squares along the
/// line in front of the given one, from the point of view of the given color.

constexpr Bitboard forward_file_bb(Color c, Square s) {
  return forward_ranks_bb(c, s) & file_bb(s);
}
#endif

// distance() functions return the distance between x and y, defined as the
// number of steps for a king in x to reach y.

template<typename T1 = Square>
inline int distance(Square x, Square y);

template<>
inline int distance<File>(Square x, Square y) {
    return std::abs(file_of(x) - file_of(y));
}

template<>
inline int distance<Rank>(Square x, Square y) {
    return std::abs(rank_of(x) - rank_of(y));
}

template<>
inline int distance<Square>(Square x, Square y) {
    return SquareDistance[x][y];
}

#ifdef FAIRY_STOCKFISH
template<RiderType R>
inline Bitboard rider_attacks_bb(Square s, Bitboard occupied) {

  static_assert(R != NO_RIDER && !(R & (R - 1))); // exactly one bit
  const Magic& m =  R == RIDER_ROOK_H ? RookMagicsH[s]
                  : R == RIDER_ROOK_V ? RookMagicsV[s]
                  : R == RIDER_CANNON_H ? CannonMagicsH[s]
                  : R == RIDER_CANNON_V ? CannonMagicsV[s]
                  : R == RIDER_LAME_DABBABA ? LameDabbabaMagics[s]
                  : R == RIDER_HORSE ? HorseMagics[s]
                  : R == RIDER_ELEPHANT ? ElephantMagics[s]
                  : R == RIDER_JANGGI_ELEPHANT ? JanggiElephantMagics[s]
                  : R == RIDER_CANNON_DIAG ? CannonDiagMagics[s]
                  : R == RIDER_NIGHTRIDER ? NightriderMagics[s]
                  : R == RIDER_GRASSHOPPER_H ? GrasshopperMagicsH[s]
                  : R == RIDER_GRASSHOPPER_V ? GrasshopperMagicsV[s]
                  : R == RIDER_GRASSHOPPER_D ? GrasshopperMagicsD[s]
                  : BishopMagics[s];
  return m.attacks[m.index(occupied)];
}

inline Square lsb(Bitboard b);

inline Bitboard rider_attacks_bb(RiderType R, Square s, Bitboard occupied) {

  assert(R != NO_RIDER && !(R & (R - 1))); // exactly one bit
  const Magic& m = magics[lsb(R)][s]; // re-use Bitboard lsb for riders
  return m.attacks[m.index(occupied)];
}
#endif

/// attacks_bb(Square) returns the pseudo attacks of the give piece type
/// assuming an empty board.
#ifndef FAIRY_STOCKFISH
inline int edge_distance(File f) { return std::min(f, File(FILE_H - f)); }
#else
inline int edge_distance(File f, File maxFile = FILE_H) { return std::min(f, File(maxFile - f)); }
#endif

// Returns the pseudo attacks of the given piece type
// assuming an empty board.
template<PieceType Pt>
inline Bitboard attacks_bb(Square s) {

    assert((Pt != PAWN) && (is_ok(s)));
#ifndef FAIRY_STOCKFISH
    return PseudoAttacks[Pt][s];
#else
  return PseudoAttacks[WHITE][Pt][s];
#endif
}


// Returns the attacks by the given piece
// assuming the board is occupied according to the passed Bitboard.
// Sliding piece attacks do not continue passed an occupied square.
template<PieceType Pt>
inline Bitboard attacks_bb(Square s, Bitboard occupied) {

    assert((Pt != PAWN) && (is_ok(s)));

    switch (Pt)
    {
#ifndef FAIRY_STOCKFISH
    case BISHOP :
        return BishopMagics[s].attacks[BishopMagics[s].index(occupied)];
    case ROOK :
        return RookMagics[s].attacks[RookMagics[s].index(occupied)];
    case QUEEN :
        return attacks_bb<BISHOP>(s, occupied) | attacks_bb<ROOK>(s, occupied);
    default :
        return PseudoAttacks[Pt][s];
#else
    case BISHOP:
        return rider_attacks_bb<RIDER_BISHOP>(s, occupied);
    case ROOK :
        return rider_attacks_bb<RIDER_ROOK_H>(s, occupied) | rider_attacks_bb<RIDER_ROOK_V>(s, occupied);
    case QUEEN :
        return attacks_bb<BISHOP>(s, occupied) | attacks_bb<ROOK>(s, occupied);
    default :
        return PseudoAttacks[WHITE][Pt][s];
#endif
    }
}

#ifdef FAIRY_STOCKFISH
/// pop_rider() finds and clears a rider in a (hybrid) rider type

inline RiderType pop_rider(RiderType* r) {
  assert(*r);
  const RiderType r2 = *r & ~(*r - 1);
  *r &= *r - 1;
  return r2;
}
#endif
// Returns the attacks by the given piece
// assuming the board is occupied according to the passed Bitboard.
// Sliding piece attacks do not continue passed an occupied square.
#ifndef FAIRY_STOCKFISH
inline Bitboard attacks_bb(PieceType pt, Square s, Bitboard occupied) {

    assert((pt != PAWN) && (is_ok(s)));

    switch (pt)
    {
    case BISHOP :
        return attacks_bb<BISHOP>(s, occupied);
    case ROOK :
        return attacks_bb<ROOK>(s, occupied);
    case QUEEN :
        return attacks_bb<BISHOP>(s, occupied) | attacks_bb<ROOK>(s, occupied);
    default :
        return PseudoAttacks[pt][s];
    }
}

#else
inline Bitboard attacks_bb(Color c, PieceType pt, Square s, Bitboard occupied) {
  Bitboard b = LeaperAttacks[c][pt][s];
  RiderType r = AttackRiderTypes[pt];
  while (r)
      b |= rider_attacks_bb(pop_rider(&r), s, occupied);
  return b & PseudoAttacks[c][pt][s];
}


template <bool Initial=false>
inline Bitboard moves_bb(Color c, PieceType pt, Square s, Bitboard occupied) {
  Bitboard b = LeaperMoves[Initial][c][pt][s];
  RiderType r = MoveRiderTypes[Initial][pt];
  while (r)
      b |= rider_attacks_bb(pop_rider(&r), s, occupied);
  return b & PseudoMoves[Initial][c][pt][s];
}

#endif

// Counts the number of non-zero bits in a bitboard.
inline int popcount(Bitboard b) {

#ifndef USE_POPCNT

#ifdef LARGEBOARDS
  union { Bitboard bb; uint16_t u[8]; } v = { b };
  return  PopCnt16[v.u[0]] + PopCnt16[v.u[1]] + PopCnt16[v.u[2]] + PopCnt16[v.u[3]]
        + PopCnt16[v.u[4]] + PopCnt16[v.u[5]] + PopCnt16[v.u[6]] + PopCnt16[v.u[7]];
#else
    union {
        Bitboard bb;
        uint16_t u[4];
    } v = {b};
    return PopCnt16[v.u[0]] + PopCnt16[v.u[1]] + PopCnt16[v.u[2]] + PopCnt16[v.u[3]];
#endif

#elif defined(_MSC_VER)

#ifdef LARGEBOARDS
  return (int)_mm_popcnt_u64(uint64_t(b >> 64)) + (int)_mm_popcnt_u64(uint64_t(b));
#else
    return int(_mm_popcnt_u64(b));
#endif

#else  // Assumed gcc or compatible compiler

#ifdef LARGEBOARDS
  return __builtin_popcountll(b >> 64) + __builtin_popcountll(b);
#else
    return __builtin_popcountll(b);
#endif

#endif
}

// Returns the least significant bit in a non-zero bitboard.
inline Square lsb(Bitboard b) {
    assert(b);

#if defined(__GNUC__)  // GCC, Clang, ICX

#ifdef LARGEBOARDS
  if (!(b << 64))
    return Square(__builtin_ctzll(b >> 64) + 64);
#endif
    return Square(__builtin_ctzll(b));

#elif defined(_MSC_VER)
    #ifdef _WIN64  // MSVC, WIN64

    unsigned long idx;
    #ifdef LARGEBOARDS
    if (uint64_t(b))
    {
        _BitScanForward64(&idx, uint64_t(b));
        return Square(idx);
    }
    else
    {
        _BitScanForward64(&idx, uint64_t(b >> 64));
        return Square(idx + 64);
    }
    #else
    _BitScanForward64(&idx, b);
    return Square(idx);

    #endif
    #else  // MSVC, WIN32
    unsigned long idx;

#ifdef LARGEBOARDS
    if (b << 96) {
        _BitScanForward(&idx, uint32_t(b));
        return Square(idx);
    } else if (b << 64) {
        _BitScanForward(&idx, uint32_t(b >> 32));
        return Square(idx + 32);
    } else if (b << 32) {
        _BitScanForward(&idx, uint32_t(b >> 64));
        return Square(idx + 64);
    } else {
        _BitScanForward(&idx, uint32_t(b >> 96));
        return Square(idx + 96);
    }
#else
    if (b & 0xffffffff)
    {
#ifndef FAIRY_STOCKFISH
        _BitScanForward(&idx, int32_t(b));
#else
        _BitScanForward(&idx, uint32_t(b));
#endif
        return Square(idx);
    }
    else
    {
#ifndef FAIRY_STOCKFISH
        _BitScanForward(&idx, int32_t(b >> 32));
#else
        _BitScanForward(&idx, uint32_t(b >> 32));
#endif
        return Square(idx + 32);
    }
#endif
    #endif
#else  // Compiler is neither GCC nor MSVC compatible
    #error "Compiler not supported."
#endif
}

// Returns the most significant bit in a non-zero bitboard.
inline Square msb(Bitboard b) {
    assert(b);

#if defined(__GNUC__)  // GCC, Clang, ICX

    return Square(63 ^ __builtin_clzll(b));

#elif defined(_MSC_VER)
    #ifdef _WIN64  // MSVC, WIN64

    unsigned long idx;
    _BitScanReverse64(&idx, b);
    return Square(idx);

    #else  // MSVC, WIN32

    unsigned long idx;

#ifdef LARGEBOARDS
    if (b >> 96) {
        _BitScanReverse(&idx, uint32_t(b >> 96));
        return Square(idx + 96);
    } else if (b >> 64) {
        _BitScanReverse(&idx, uint32_t(b >> 64));
        return Square(idx + 64);
    } else
#endif
    if (b >> 32)
    {
#ifndef FAIRY_STOCKFISH
        _BitScanReverse(&idx, int32_t(b >> 32));
#else
        _BitScanReverse(&idx, uint32_t(b >> 32));
#endif
        return Square(idx + 32);
    }
    else
    {
#ifndef FAIRY_STOCKFISH
        _BitScanReverse(&idx, int32_t(b));
#else
        _BitScanReverse(&idx, uint32_t(b));
#endif
        return Square(idx);
    }
    #endif
#else  // Compiler is neither GCC nor MSVC compatible
    #error "Compiler not supported."
#endif
}

// Returns the bitboard of the least significant
// square of a non-zero bitboard. It is equivalent to square_bb(lsb(bb)).
inline Bitboard least_significant_square_bb(Bitboard b) {
    assert(b);
    return b & -b;
}

// Finds and clears the least significant bit in a non-zero bitboard.
inline Square pop_lsb(Bitboard& b) {
    assert(b);
    const Square s = lsb(b);
    b &= b - 1;
    return s;
}

#ifdef FAIRY_STOCKFISH
/// popcount() counts the number of non-zero bits in a piece set

inline int popcount(PieceSet ps) {

#ifndef USE_POPCNT

  union { uint64_t bb; uint16_t u[4]; } v = { (uint64_t)ps };
  return PopCnt16[v.u[0]] + PopCnt16[v.u[1]] + PopCnt16[v.u[2]] + PopCnt16[v.u[3]];

#elif defined(_MSC_VER) || defined(__INTEL_COMPILER)

  return (int)_mm_popcnt_u64(ps);

#else // Assumed gcc or compatible compiler

  return __builtin_popcountll(ps);

#endif
}

/// lsb() and msb() return the least/most significant bit in a non-zero piece set

#if defined(__GNUC__)  // GCC, Clang, ICC

inline PieceType lsb(PieceSet ps) {
  assert(ps);
  return PieceType(__builtin_ctzll(ps));
}

inline PieceType msb(PieceSet ps) {
  assert(ps);
  return PieceType((PIECE_TYPE_NB - 1) ^ __builtin_clzll(ps));
}

#elif defined(_MSC_VER)  // MSVC

#ifdef _WIN64  // MSVC, WIN64

inline PieceType lsb(PieceSet ps) {
  assert(ps);
  unsigned long idx;
  _BitScanForward64(&idx, ps);
  return (PieceType) idx;
}

inline PieceType msb(PieceSet ps) {
  assert(ps);
  unsigned long idx;
  _BitScanReverse64(&idx, ps);
  return (PieceType) idx;
}

#else  // MSVC, WIN32

inline PieceType lsb(PieceSet ps) {
  assert(ps);
  unsigned long idx;

  if (ps & 0xffffffff) {
      _BitScanForward(&idx, uint32_t(ps));
      return PieceType(idx);
  } else {
      _BitScanForward(&idx, uint32_t(ps >> 32));
      return PieceType(idx + 32);
  }
}

inline PieceType msb(PieceSet ps) {
  assert(ps);
  unsigned long idx;
  if (ps >> 32) {
      _BitScanReverse(&idx, uint32_t(ps >> 32));
      return PieceType(idx + 32);
  } else {
      _BitScanReverse(&idx, uint32_t(ps));
      return PieceType(idx);
  }
}

#endif

#else  // Compiler is neither GCC nor MSVC compatible

#error "Compiler not supported."

#endif

/// pop_lsb() and pop_msb() find and clear the least/most significant bit in a non-zero piece set

inline PieceType pop_lsb(PieceSet& ps) {
  assert(ps);
  const PieceType pt = lsb(ps);
  ps &= PieceSet(ps - 1);
  return pt;
}

inline PieceType pop_msb(PieceSet& ps) {
  assert(ps);
  const PieceType pt = msb(ps);
  ps &= ~piece_set(pt);
  return pt;
}

#endif
}  // namespace Stockfish

#endif  // #ifndef BITBOARD_H_INCLUDED
