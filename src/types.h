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

#ifndef TYPES_H_INCLUDED
    #define TYPES_H_INCLUDED

// When compiling with provided Makefile (e.g. for Linux and OSX), configuration
// is done automatically. To get started type 'make help'.
//
// When Makefile is not used (e.g. with Microsoft Visual Studio) some switches
// need to be set manually:
//
// -DNDEBUG      | Disable debugging mode. Always use this for release.
//
// -DNO_PREFETCH | Disable use of prefetch asm-instruction. You may need this to
//               | run on some very old machines.
//
// -DUSE_POPCNT  | Add runtime support for use of popcnt asm-instruction. Works
//               | only in 64-bit mode and requires hardware with popcnt support.
//
// -DUSE_PEXT    | Add runtime support for use of pext asm-instruction. Works
//               | only in 64-bit mode and requires hardware with pext support.

    #include <cassert>
    #include <cstdint>

    #if defined(_MSC_VER)
        // Disable some silly and noisy warnings from MSVC compiler
        #pragma warning(disable: 4127)  // Conditional expression is constant
        #pragma warning(disable: 4146)  // Unary minus operator applied to unsigned type
        #pragma warning(disable: 4800)  // Forcing value to bool 'true' or 'false'
    #ifdef FAIRY_STOCKFISH
        #pragma comment(linker, "/STACK:8000000") // Use 8 MB stack size for MSVC
        #pragma comment(lib, "advapi32.lib") // Fix linker error
    #endif
    #endif


// Predefined macros hell:
//
// __GNUC__                Compiler is GCC, Clang or ICX
// __clang__               Compiler is Clang or ICX
// __INTEL_LLVM_COMPILER   Compiler is ICX
// _MSC_VER                Compiler is MSVC
// _WIN32                  Building on Windows (any)
// _WIN64                  Building on Windows 64 bit

    #if defined(__GNUC__) && (__GNUC__ < 9 || (__GNUC__ == 9 && __GNUC_MINOR__ <= 2)) \
      && defined(_WIN32) && !defined(__clang__)
        #define ALIGNAS_ON_STACK_VARIABLES_BROKEN
    #endif

    #define ASSERT_ALIGNED(ptr, alignment) assert(reinterpret_cast<uintptr_t>(ptr) % alignment == 0)

    #if defined(_WIN64) && defined(_MSC_VER)  // No Makefile used
        #include <intrin.h>                   // Microsoft header for _BitScanForward64()
        #define IS_64BIT
    #endif

    #if defined(USE_POPCNT) && defined(_MSC_VER)
        #include <nmmintrin.h>  // Microsoft header for _mm_popcnt_u64()
    #endif

    #if !defined(NO_PREFETCH) && defined(_MSC_VER)
        #include <xmmintrin.h>  // Microsoft header for _mm_prefetch()
    #endif

    #if defined(USE_PEXT)
        #include <immintrin.h>  // Header for _pext_u64() intrinsic
    #ifndef FAIRY_STOCKFISH
        #define pext(b, m) _pext_u64(b, m)
    #else
        #ifdef LARGEBOARDS
            #define pext(b, m) (_pext_u64(b, m) ^ (_pext_u64(b >> 64, m >> 64) << popcount((m << 64) >> 64)))
        #else
            #define pext(b, m) _pext_u64(b, m)
        #endif
    #endif
    #else
        #define pext(b, m) 0
    #endif
namespace Stockfish {

    #ifdef USE_POPCNT
constexpr bool HasPopCnt = true;
    #else
constexpr bool HasPopCnt = false;
    #endif

    #ifdef USE_PEXT
constexpr bool HasPext = true;
    #else
constexpr bool HasPext = false;
    #endif

    #ifdef IS_64BIT
constexpr bool Is64Bit = true;
    #else
constexpr bool Is64Bit = false;
    #endif

using Key      = uint64_t;
#ifndef FAIRY_STOCKFISH
using Bitboard = uint64_t;

constexpr int MAX_MOVES = 256;
constexpr int MAX_PLY   = 246;
#else
#ifdef LARGEBOARDS
#if defined(__GNUC__) && defined(IS_64BIT)
typedef unsigned __int128 Bitboard;
#else
struct Bitboard {
    uint64_t b64[2];

    constexpr Bitboard() : b64 {0, 0} {}
    constexpr Bitboard(uint64_t i) : b64 {0, i} {}
    constexpr Bitboard(uint64_t hi, uint64_t lo) : b64 {hi, lo} {};

    constexpr operator bool() const {
        return b64[0] || b64[1];
    }

    constexpr operator long long unsigned () const {
        return b64[1];
    }

    constexpr operator unsigned() const {
        return b64[1];
    }

    constexpr Bitboard operator << (const unsigned int bits) const {
        return Bitboard(  bits >= 64 ? b64[1] << (bits - 64)
                        : bits == 0  ? b64[0]
                        : ((b64[0] << bits) | (b64[1] >> (64 - bits))),
                        bits >= 64 ? 0 : b64[1] << bits);
    }

    constexpr Bitboard operator >> (const unsigned int bits) const {
        return Bitboard(bits >= 64 ? 0 : b64[0] >> bits,
                          bits >= 64 ? b64[0] >> (bits - 64)
                        : bits == 0  ? b64[1]
                        : ((b64[1] >> bits) | (b64[0] << (64 - bits))));
    }

    constexpr Bitboard operator << (const int bits) const {
        return *this << unsigned(bits);
    }

    constexpr Bitboard operator >> (const int bits) const {
        return *this >> unsigned(bits);
    }

    constexpr bool operator == (const Bitboard y) const {
        return (b64[0] == y.b64[0]) && (b64[1] == y.b64[1]);
    }

    constexpr bool operator != (const Bitboard y) const {
        return !(*this == y);
    }

    inline Bitboard& operator |=(const Bitboard x) {
        b64[0] |= x.b64[0];
        b64[1] |= x.b64[1];
        return *this;
    }
    inline Bitboard& operator &=(const Bitboard x) {
        b64[0] &= x.b64[0];
        b64[1] &= x.b64[1];
        return *this;
    }
    inline Bitboard& operator ^=(const Bitboard x) {
        b64[0] ^= x.b64[0];
        b64[1] ^= x.b64[1];
        return *this;
    }

    constexpr Bitboard operator ~ () const {
        return Bitboard(~b64[0], ~b64[1]);
    }

    constexpr Bitboard operator - () const {
        return Bitboard(-b64[0] - (b64[1] > 0), -b64[1]);
    }

    constexpr Bitboard operator | (const Bitboard x) const {
        return Bitboard(b64[0] | x.b64[0], b64[1] | x.b64[1]);
    }

    constexpr Bitboard operator & (const Bitboard x) const {
        return Bitboard(b64[0] & x.b64[0], b64[1] & x.b64[1]);
    }

    constexpr Bitboard operator ^ (const Bitboard x) const {
        return Bitboard(b64[0] ^ x.b64[0], b64[1] ^ x.b64[1]);
    }

    constexpr Bitboard operator - (const Bitboard x) const {
        return Bitboard(b64[0] - x.b64[0] - (b64[1] < x.b64[1]), b64[1] - x.b64[1]);
    }

    constexpr Bitboard operator - (const int x) const {
        return *this - Bitboard(x);
    }

    inline Bitboard operator * (const Bitboard x) const {
        uint64_t a_lo = (uint32_t)b64[1];
        uint64_t a_hi = b64[1] >> 32;
        uint64_t b_lo = (uint32_t)x.b64[1];
        uint64_t b_hi = x.b64[1] >> 32;

        uint64_t t1 = (a_hi * b_lo) + ((a_lo * b_lo) >> 32);
        uint64_t t2 = (a_lo * b_hi) + (t1 & 0xFFFFFFFF);

        return Bitboard(b64[0] * x.b64[1] + b64[1] * x.b64[0] + (a_hi * b_hi) + (t1 >> 32) + (t2 >> 32),
                        (t2 << 32) + (a_lo * b_lo & 0xFFFFFFFF));
   }
};
#endif
constexpr int SQUARE_BITS = 7;
#else
typedef uint64_t Bitboard;
constexpr int SQUARE_BITS = 6;
#endif

//When defined, move list will be stored in heap. Delete this if you want to use stack to store move list. Using stack can cause overflow (Segmentation Fault) when the search is too deep.
#define USE_HEAP_INSTEAD_OF_STACK_FOR_MOVE_LIST

#ifdef ALLVARS
constexpr int MAX_MOVES = 8192;
#ifdef USE_HEAP_INSTEAD_OF_STACK_FOR_MOVE_LIST
constexpr int MAX_PLY = 246;
#else
constexpr int MAX_PLY = 60;
#endif
/// endif USE_HEAP_INSTEAD_OF_STACK_FOR_MOVE_LIST
#else
constexpr int MAX_MOVES = 1024;
constexpr int MAX_PLY = 246;
#endif
/// endif ALLVARS
#endif

#ifdef FAIRY_STOCKFISH
constexpr int MOVE_TYPE_BITS = 4;
#endif

enum Color {
    WHITE,
    BLACK,
    COLOR_NB = 2
};

enum CastlingRights {
    NO_CASTLING,
    WHITE_OO,
    WHITE_OOO = WHITE_OO << 1,
    BLACK_OO  = WHITE_OO << 2,
    BLACK_OOO = WHITE_OO << 3,

    KING_SIDE      = WHITE_OO | BLACK_OO,
    QUEEN_SIDE     = WHITE_OOO | BLACK_OOO,
    WHITE_CASTLING = WHITE_OO | WHITE_OOO,
    BLACK_CASTLING = BLACK_OO | BLACK_OOO,
    ANY_CASTLING   = WHITE_CASTLING | BLACK_CASTLING,

    CASTLING_RIGHT_NB = 16
};
#ifdef FAIRY_STOCKFISH

enum CheckCount : int {
    CHECKS_0 = 0, CHECKS_NB = 11
};

enum MaterialCounting {
    NO_MATERIAL_COUNTING, JANGGI_MATERIAL, UNWEIGHTED_MATERIAL, WHITE_DRAW_ODDS, BLACK_DRAW_ODDS
};

enum CountingRule {
    NO_COUNTING, MAKRUK_COUNTING, CAMBODIAN_COUNTING, ASEAN_COUNTING
};

enum ChasingRule {
    NO_CHASING, AXF_CHASING
};

enum EnclosingRule {
    NO_ENCLOSING, REVERSI, ATAXX, QUADWRANGLE, SNORT, ANYSIDE, TOP
};

enum WallingRule {
    NO_WALLING, ARROW, DUCK, EDGE, PAST, STATIC
};

enum EndgameEval {
    NO_EG_EVAL, EG_EVAL_CHESS, EG_EVAL_ANTI, EG_EVAL_ATOMIC, EG_EVAL_DUCK, EG_EVAL_MISERE, EG_EVAL_RK, EG_EVAL_NB
};

enum OptBool {
    NO_VALUE, VALUE_FALSE, VALUE_TRUE
};
#endif

enum Bound {
    BOUND_NONE,
    BOUND_UPPER,
    BOUND_LOWER,
    BOUND_EXACT = BOUND_UPPER | BOUND_LOWER
};

// Value is used as an alias for int16_t, this is done to differentiate between
// a search value and any other integer value. The values used in search are always
// supposed to be in the range (-VALUE_NONE, VALUE_NONE] and should not exceed this range.
using Value = int;

constexpr Value VALUE_ZERO     = 0;
constexpr Value VALUE_DRAW     = 0;
#ifdef FAIRY_STOCKFISH
constexpr Value XBOARD_VALUE_MATE = 200000;
constexpr Value VALUE_VIRTUAL_MATE = 3000;
constexpr Value VALUE_VIRTUAL_MATE_IN_MAX_PLY = VALUE_VIRTUAL_MATE - MAX_PLY;
#endif
constexpr Value VALUE_NONE     = 32002;
constexpr Value VALUE_INFINITE = 32001;

#ifdef FAIRY_STOCKFISH

constexpr int PIECE_TYPE_BITS = 6; // PIECE_TYPE_NB = pow(2, PIECE_TYPE_BITS)
#endif
constexpr Value VALUE_MATE             = 32000;
constexpr Value VALUE_MATE_IN_MAX_PLY  = VALUE_MATE - MAX_PLY;
constexpr Value VALUE_MATED_IN_MAX_PLY = -VALUE_MATE_IN_MAX_PLY;

constexpr Value VALUE_TB                 = VALUE_MATE_IN_MAX_PLY - 1;
constexpr Value VALUE_TB_WIN_IN_MAX_PLY  = VALUE_TB - MAX_PLY;
constexpr Value VALUE_TB_LOSS_IN_MAX_PLY = -VALUE_TB_WIN_IN_MAX_PLY;

// In the code, we make the assumption that these values
// are such that non_pawn_material() can be used to uniquely
// identify the material on the board.
constexpr Value PawnValue   = 208;
constexpr Value KnightValue = 781;
constexpr Value BishopValue = 825;
constexpr Value RookValue   = 1276;
constexpr Value QueenValue  = 2538;

#ifdef FAIRY_STOCKFISH
constexpr Value FersValue               = 420;
constexpr Value AlfilValue              = 350;
constexpr Value FersAlfilValue          = 700;
constexpr Value SilverValue             = 660;
constexpr Value AiwokValue              = 2300;
constexpr Value BersValueMg             = 1800;
constexpr Value ArchbishopValue         = 2200;
constexpr Value ChancellorValue         = 2300;
constexpr Value AmazonValue             = 2700;
constexpr Value KnibisValue             = 1100;
constexpr Value BiskniValue             = 750;
constexpr Value KnirooValue             = 1050;
constexpr Value RookniValue             = 800;
constexpr Value ShogiPawnValue          = 90;
constexpr Value LanceValue              = 400;
constexpr Value ShogiKnightValue        = 420;
constexpr Value GoldValue               = 720;
constexpr Value DragonHorseValue        = 1550;
constexpr Value ClobberPieceValue       = 300;
constexpr Value BreakthroughPieceValue  = 300;
constexpr Value ImmobilePieceValue      = 50;
constexpr Value CannonPieceValue        = 800;
constexpr Value JanggiCannonPieceValue  = 800;
constexpr Value SoldierValue            = 200;
constexpr Value HorseValue              = 520;
constexpr Value ElephantValue           = 300;
constexpr Value JanggiElephantValue     = 340;
constexpr Value BannerValue             = 3400;
constexpr Value WazirValue              = 400;
constexpr Value CommonerValue           = 700;
constexpr Value CentaurValueMg          = 1800;
#endif

// clang-format off
enum PieceType {
#ifndef FAIRY_STOCKFISH
    NO_PIECE_TYPE, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING,
#else
    NO_PIECE_TYPE, PAWN, KNIGHT, BISHOP, ROOK, QUEEN,
    FERS, MET = FERS, ALFIL, FERS_ALFIL, SILVER, KHON = SILVER, AIWOK, BERS, DRAGON = BERS,
    ARCHBISHOP, CHANCELLOR, AMAZON, KNIBIS, BISKNI, KNIROO, ROOKNI,
    SHOGI_PAWN, LANCE, SHOGI_KNIGHT, GOLD, DRAGON_HORSE,
    CLOBBER_PIECE, BREAKTHROUGH_PIECE, IMMOBILE_PIECE, CANNON, JANGGI_CANNON,
    SOLDIER, HORSE, ELEPHANT, JANGGI_ELEPHANT, BANNER,
    WAZIR, COMMONER, CENTAUR,

    CUSTOM_PIECE_1, CUSTOM_PIECE_2, CUSTOM_PIECE_3, CUSTOM_PIECE_4,
    CUSTOM_PIECE_5, CUSTOM_PIECE_6, CUSTOM_PIECE_7, CUSTOM_PIECE_8,

    PIECE_TYPE_NB = 1 << PIECE_TYPE_BITS,
    KING = PIECE_TYPE_NB - 1,

    // Aliases
    CUSTOM_PIECES = CUSTOM_PIECE_1,
    CUSTOM_PIECES_END = KING - 1,
    CUSTOM_PIECES_ROYAL = CUSTOM_PIECES_END,
    CUSTOM_PIECES_NB = CUSTOM_PIECES_END - CUSTOM_PIECES + 1,
    FAIRY_PIECES = QUEEN + 1,
    FAIRY_PIECES_END = CUSTOM_PIECES - 1,
#endif
    ALL_PIECES = 0,
#ifndef FAIRY_STOCKFISH
    PIECE_TYPE_NB = 8
#endif
};
#ifdef FAIRY_STOCKFISH
static_assert(KING < PIECE_TYPE_NB, "KING exceeds PIECE_TYPE_NB.");
static_assert(PIECE_TYPE_BITS <= 6, "PIECE_TYPE uses more than 6 bit");
static_assert(!(PIECE_TYPE_NB & (PIECE_TYPE_NB - 1)), "PIECE_TYPE_NB is not a power of 2");

static_assert(2 * SQUARE_BITS + MOVE_TYPE_BITS + 2 * PIECE_TYPE_BITS <= 32, "Move encoding uses more than 32 bits");
#endif

enum Piece {
#ifndef FAIRY_STOCKFISH
    NO_PIECE,
    W_PAWN = PAWN,     W_KNIGHT, W_BISHOP, W_ROOK, W_QUEEN, W_KING,
    B_PAWN = PAWN + 8, B_KNIGHT, B_BISHOP, B_ROOK, B_QUEEN, B_KING,
    PIECE_NB = 16
#else
    NO_PIECE,
    W_PAWN = PAWN,                 W_KNIGHT, W_BISHOP, W_ROOK, W_QUEEN, W_KING = KING,
    B_PAWN = PAWN + PIECE_TYPE_NB, B_KNIGHT, B_BISHOP, B_ROOK, B_QUEEN, B_KING = KING + PIECE_TYPE_NB,
    PIECE_NB = 2 * PIECE_TYPE_NB
#endif
};
// clang-format on

#ifndef FAIRY_STOCKFISH
constexpr Value PieceValue[PIECE_NB] = {
  VALUE_ZERO, PawnValue, KnightValue, BishopValue, RookValue, QueenValue, VALUE_ZERO, VALUE_ZERO,
  VALUE_ZERO, PawnValue, KnightValue, BishopValue, RookValue, QueenValue, VALUE_ZERO, VALUE_ZERO};
#else
enum PieceSet : uint64_t {
    NO_PIECE_SET = 0,
    CHESS_PIECES = (1ULL << PAWN) | (1ULL << KNIGHT) | (1ULL << BISHOP) | (1ULL << ROOK) | (1ULL << QUEEN) | (1ULL << KING),
    COMMON_FAIRY_PIECES = (1ULL << IMMOBILE_PIECE) | (1ULL << COMMONER) | (1ULL << ARCHBISHOP) | (1ULL << CHANCELLOR),
    SHOGI_PIECES = (1ULL << SHOGI_PAWN) | (1ULL << GOLD) | (1ULL << SILVER) | (1ULL << SHOGI_KNIGHT) | (1ULL << LANCE)
                  | (1ULL << DRAGON)| (1ULL << DRAGON_HORSE) | (1ULL << KING),
    COMMON_STEP_PIECES = (1ULL << COMMONER) | (1ULL << FERS) | (1ULL << WAZIR) | (1ULL << BREAKTHROUGH_PIECE),
};

enum RiderType : int {
    NO_RIDER = 0,
    RIDER_BISHOP = 1 << 0,
    RIDER_ROOK_H = 1 << 1,
    RIDER_ROOK_V = 1 << 2,
    RIDER_CANNON_H = 1 << 3,
    RIDER_CANNON_V = 1 << 4,
    RIDER_LAME_DABBABA = 1 << 5,
    RIDER_HORSE = 1 << 6,
    RIDER_ELEPHANT = 1 << 7,
    RIDER_JANGGI_ELEPHANT = 1 << 8,
    RIDER_CANNON_DIAG = 1 << 9,
    RIDER_NIGHTRIDER = 1 << 10,
    RIDER_GRASSHOPPER_H = 1 << 11,
    RIDER_GRASSHOPPER_V = 1 << 12,
    RIDER_GRASSHOPPER_D = 1 << 13,
    HOPPING_RIDERS =  RIDER_CANNON_H | RIDER_CANNON_V | RIDER_CANNON_DIAG
                    | RIDER_GRASSHOPPER_H | RIDER_GRASSHOPPER_V | RIDER_GRASSHOPPER_D,
    LAME_LEAPERS = RIDER_LAME_DABBABA | RIDER_HORSE | RIDER_ELEPHANT | RIDER_JANGGI_ELEPHANT,
    ASYMMETRICAL_RIDERS =  RIDER_HORSE | RIDER_JANGGI_ELEPHANT
                        | RIDER_GRASSHOPPER_H | RIDER_GRASSHOPPER_V | RIDER_GRASSHOPPER_D,
    NON_SLIDING_RIDERS = HOPPING_RIDERS | LAME_LEAPERS | RIDER_NIGHTRIDER,
};

extern Value PieceValue[PIECE_NB];
extern Value EvalPieceValue[PIECE_NB]; // variant piece values for evaluation
extern Value CapturePieceValue[PIECE_NB]; // variant piece values for captures/search
#endif
using Depth = int;

enum : int {
    DEPTH_QS_CHECKS    = 0,
    DEPTH_QS_NO_CHECKS = -1,

    DEPTH_NONE = -6,

    DEPTH_OFFSET = -7  // value used only for TT entry occupancy check
};

// clang-format off
enum Square : int {
#ifdef LARGEBOARDS
    SQ_A1, SQ_B1, SQ_C1, SQ_D1, SQ_E1, SQ_F1, SQ_G1, SQ_H1, SQ_I1, SQ_J1, SQ_K1, SQ_L1,
    SQ_A2, SQ_B2, SQ_C2, SQ_D2, SQ_E2, SQ_F2, SQ_G2, SQ_H2, SQ_I2, SQ_J2, SQ_K2, SQ_L2,
    SQ_A3, SQ_B3, SQ_C3, SQ_D3, SQ_E3, SQ_F3, SQ_G3, SQ_H3, SQ_I3, SQ_J3, SQ_K3, SQ_L3,
    SQ_A4, SQ_B4, SQ_C4, SQ_D4, SQ_E4, SQ_F4, SQ_G4, SQ_H4, SQ_I4, SQ_J4, SQ_K4, SQ_L4,
    SQ_A5, SQ_B5, SQ_C5, SQ_D5, SQ_E5, SQ_F5, SQ_G5, SQ_H5, SQ_I5, SQ_J5, SQ_K5, SQ_L5,
    SQ_A6, SQ_B6, SQ_C6, SQ_D6, SQ_E6, SQ_F6, SQ_G6, SQ_H6, SQ_I6, SQ_J6, SQ_K6, SQ_L6,
    SQ_A7, SQ_B7, SQ_C7, SQ_D7, SQ_E7, SQ_F7, SQ_G7, SQ_H7, SQ_I7, SQ_J7, SQ_K7, SQ_L7,
    SQ_A8, SQ_B8, SQ_C8, SQ_D8, SQ_E8, SQ_F8, SQ_G8, SQ_H8, SQ_I8, SQ_J8, SQ_K8, SQ_L8,
    SQ_A9, SQ_B9, SQ_C9, SQ_D9, SQ_E9, SQ_F9, SQ_G9, SQ_H9, SQ_I9, SQ_J9, SQ_K9, SQ_L9,
    SQ_A10, SQ_B10, SQ_C10, SQ_D10, SQ_E10, SQ_F10, SQ_G10, SQ_H10, SQ_I10, SQ_J10, SQ_K10, SQ_L10,
#else
    SQ_A1, SQ_B1, SQ_C1, SQ_D1, SQ_E1, SQ_F1, SQ_G1, SQ_H1,
    SQ_A2, SQ_B2, SQ_C2, SQ_D2, SQ_E2, SQ_F2, SQ_G2, SQ_H2,
    SQ_A3, SQ_B3, SQ_C3, SQ_D3, SQ_E3, SQ_F3, SQ_G3, SQ_H3,
    SQ_A4, SQ_B4, SQ_C4, SQ_D4, SQ_E4, SQ_F4, SQ_G4, SQ_H4,
    SQ_A5, SQ_B5, SQ_C5, SQ_D5, SQ_E5, SQ_F5, SQ_G5, SQ_H5,
    SQ_A6, SQ_B6, SQ_C6, SQ_D6, SQ_E6, SQ_F6, SQ_G6, SQ_H6,
    SQ_A7, SQ_B7, SQ_C7, SQ_D7, SQ_E7, SQ_F7, SQ_G7, SQ_H7,
    SQ_A8, SQ_B8, SQ_C8, SQ_D8, SQ_E8, SQ_F8, SQ_G8, SQ_H8,
#endif
    SQ_NONE,

    SQUARE_ZERO = 0,
#ifndef FAIRY_STOCKFISH
    SQUARE_NB   = 64
#else
#ifdef LARGEBOARDS
    SQUARE_NB = 120,
    SQUARE_BIT_MASK = 127,
#else
    SQUARE_NB = 64,
    SQUARE_BIT_MASK = 63,
#endif
    SQ_MAX = SQUARE_NB - 1,
    SQUARE_NB_CHESS = 64,
    SQUARE_NB_SHOGI = 81,
#endif
};
// clang-format on

enum Direction : int {
#ifdef LARGEBOARDS
    NORTH =  12,
#else
    NORTH = 8,
#endif
    EAST  = 1,
    SOUTH = -NORTH,
    WEST  = -EAST,

    NORTH_EAST = NORTH + EAST,
    SOUTH_EAST = SOUTH + EAST,
    SOUTH_WEST = SOUTH + WEST,
    NORTH_WEST = NORTH + WEST
};

enum File : int {
#ifndef FAIRY_STOCKFISH
    FILE_A,
    FILE_B,
    FILE_C,
    FILE_D,
    FILE_E,
    FILE_F,
    FILE_G,
    FILE_H,
    FILE_NB
#else
#ifdef LARGEBOARDS
    FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H, FILE_I, FILE_J, FILE_K, FILE_L,
#else
    FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H,
#endif
    FILE_NB,
    FILE_MAX = FILE_NB - 1
#endif
};

enum Rank : int {
#ifndef FAIRY_STOCKFISH
    RANK_1,
    RANK_2,
    RANK_3,
    RANK_4,
    RANK_5,
    RANK_6,
    RANK_7,
    RANK_8,
    RANK_NB
#else
#ifdef LARGEBOARDS
    RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, RANK_9, RANK_10,
#else
    RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8,
#endif
    RANK_NB,
    RANK_MAX = RANK_NB - 1
#endif
};

// Keep track of what a move changes on the board (used by NNUE)
struct DirtyPiece {

    // Number of changed pieces
    int dirty_num;

    // Max 3 pieces can change in one move. A promotion with capture moves
    // both the pawn and the captured piece to SQ_NONE and the piece promoted
    // to from SQ_NONE to the capture square.
#ifndef FAIRY_STOCKFISH
    Piece piece[3];

#else
    Piece piece[12];
    Piece handPiece[12];
    int handCount[12];
#endif
    // From and to squares, which may be SQ_NONE
#ifndef FAIRY_STOCKFISH
    Square from[3];
    Square to[3];
#else
    Square from[12];
    Square to[12];
#endif
};

    #define ENABLE_INCR_OPERATORS_ON(T) \
        inline T& operator++(T& d) { return d = T(int(d) + 1); } \
        inline T& operator--(T& d) { return d = T(int(d) - 1); }

#ifdef FAIRY_STOCKFISH

#define ENABLE_BIT_OPERATORS_ON(T)                                        \
constexpr T operator~ (T d) { return (T)~(int)d; }                        \
constexpr T operator| (T d1, T d2) { return (T)((int)d1 | (int)d2); }     \
constexpr T operator& (T d1, T d2) { return (T)((int)d1 & (int)d2); }     \
constexpr T operator^ (T d1, T d2) { return (T)((int)d1 ^ (int)d2); }     \
inline T& operator|= (T& d1, T d2) { return (T&)((int&)d1 |= (int)d2); }  \
inline T& operator&= (T& d1, T d2) { return (T&)((int&)d1 &= (int)d2); }  \
inline T& operator^= (T& d1, T d2) { return (T&)((int&)d1 ^= (int)d2); }


#define ENABLE_BASE_OPERATORS_ON(T)                                \
constexpr T operator+(T d1, int d2) { return T(int(d1) + d2); }    \
constexpr T operator-(T d1, int d2) { return T(int(d1) - d2); }    \
constexpr T operator-(T d) { return T(-int(d)); }                  \
inline T& operator+=(T& d1, int d2) { return d1 = d1 + d2; }       \
inline T& operator-=(T& d1, int d2) { return d1 = d1 - d2; }
#endif

ENABLE_INCR_OPERATORS_ON(PieceType)
ENABLE_INCR_OPERATORS_ON(Square)
ENABLE_INCR_OPERATORS_ON(File)
ENABLE_INCR_OPERATORS_ON(Rank)
#ifdef FAIRY_STOCKFISH
ENABLE_INCR_OPERATORS_ON(CheckCount)
ENABLE_BASE_OPERATORS_ON(PieceType)
ENABLE_BIT_OPERATORS_ON(RiderType)
ENABLE_BASE_OPERATORS_ON(RiderType)

#undef ENABLE_BIT_OPERATORS_ON
#undef ENABLE_BASE_OPERATORS_ON

constexpr PieceSet piece_set(PieceType pt) {
  return PieceSet(1ULL << pt);
}

constexpr PieceSet operator~ (PieceSet ps) { return (PieceSet)~(uint64_t)ps; }
constexpr PieceSet operator| (PieceSet ps1, PieceSet ps2) { return (PieceSet)((uint64_t)ps1 | (uint64_t)ps2); }
constexpr PieceSet operator| (PieceSet ps, PieceType pt) { return ps | piece_set(pt); }
constexpr PieceSet operator& (PieceSet ps1, PieceSet ps2) { return (PieceSet)((uint64_t)ps1 & (uint64_t)ps2); }
constexpr PieceSet operator& (PieceSet ps, PieceType pt) { return ps & piece_set(pt); }
constexpr PieceSet operator^ (PieceSet ps1, PieceSet ps2) { return (PieceSet)((uint64_t)ps1 ^ (uint64_t)ps2); }
constexpr PieceSet operator^ (PieceSet ps, PieceType pt) { return ps ^ piece_set(pt); }
inline PieceSet& operator|= (PieceSet& ps1, PieceSet ps2) { return (PieceSet&)((uint64_t&)ps1 |= (uint64_t)ps2); }
inline PieceSet& operator|= (PieceSet& ps, PieceType pt) { return ps |= piece_set(pt); }
inline PieceSet& operator&= (PieceSet& ps1, PieceSet ps2) { return (PieceSet&)((uint64_t&)ps1 &= (uint64_t)ps2); }
//inline PieceSet& operator&= (PieceSet& ps, PieceType pt) does not make sense
inline PieceSet& operator^= (PieceSet& ps1, PieceSet ps2) { return (PieceSet&)((uint64_t&)ps1 ^= (uint64_t)ps2); }
inline PieceSet& operator^= (PieceSet& ps, PieceType pt) { return ps ^= piece_set(pt); }

static_assert(piece_set(PAWN) & PAWN);
static_assert(piece_set(KING) & KING);
#endif
    #undef ENABLE_INCR_OPERATORS_ON

constexpr Direction operator+(Direction d1, Direction d2) { return Direction(int(d1) + int(d2)); }
constexpr Direction operator*(int i, Direction d) { return Direction(i * int(d)); }

// Additional operators to add a Direction to a Square
constexpr Square operator+(Square s, Direction d) { return Square(int(s) + int(d)); }
constexpr Square operator-(Square s, Direction d) { return Square(int(s) - int(d)); }
inline Square&   operator+=(Square& s, Direction d) { return s = s + d; }
inline Square&   operator-=(Square& s, Direction d) { return s = s - d; }

// Toggle color
constexpr Color operator~(Color c) { return Color(c ^ BLACK); }

// Swap A1 <-> A8
constexpr Square flip_rank(Square s) { return Square(s ^ SQ_A8); }

// Swap A1 <-> H1
constexpr Square flip_file(Square s) { return Square(s ^ SQ_H1); }

// Swap color of piece B_KNIGHT <-> W_KNIGHT
constexpr Piece operator~(Piece pc) {
#ifndef FAIRY_STOCKFISH
    return Piece(pc ^ 8);
#else
    return Piece(pc ^ PIECE_TYPE_NB);  // Swap color of piece B_KNIGHT <-> W_KNIGHT
#endif
}

constexpr CastlingRights operator&(Color c, CastlingRights cr) {
    return CastlingRights((c == WHITE ? WHITE_CASTLING : BLACK_CASTLING) & cr);
}


constexpr Value mate_in(int ply) { return VALUE_MATE - ply; }

constexpr Value mated_in(int ply) { return -VALUE_MATE + ply; }

#ifdef FAIRY_STOCKFISH
constexpr Value convert_mate_value(Value v, int ply) {
    return  v ==  VALUE_MATE ? mate_in(ply)
        : v == -VALUE_MATE ? mated_in(ply)
        : v;
}
#endif

constexpr Square make_square(File f, Rank r) {
#ifndef FAIRY_STOCKFISH
    return Square((r << 3) + f);
#else
    return Square(r * FILE_NB + f);
#endif
}

constexpr Piece make_piece(Color c, PieceType pt) {
#ifndef FAIRY_STOCKFISH
    return Piece((c << 3) + pt);
#else
    return Piece((c << PIECE_TYPE_BITS) + pt);
#endif
}

constexpr PieceType type_of(Piece pc) {
#ifndef FAIRY_STOCKFISH
    return PieceType(pc & 7);
#else
    return PieceType(pc & (PIECE_TYPE_NB - 1));
#endif
}

inline Color color_of(Piece pc) {
    assert(pc != NO_PIECE);
#ifndef FAIRY_STOCKFISH
    return Color(pc >> 3);
#else
    return Color(pc >> PIECE_TYPE_BITS);
#endif
}

constexpr bool is_ok(Square s) {
#ifndef FAIRY_STOCKFISH
    return s >= SQ_A1 && s <= SQ_H8;
#else
    return s >= SQ_A1 && s <= SQ_MAX;
#endif
}

constexpr File file_of(Square s) {
#ifndef FAIRY_STOCKFISH
    return File(s & 7);
#else
    return File(s % FILE_NB);
#endif
}

constexpr Rank rank_of(Square s) {
#ifndef FAIRY_STOCKFISH
    return Rank(s >> 3);
#else
    return Rank(s / FILE_NB);
#endif
}
#ifndef FAIRY_STOCKFISH
constexpr Square relative_square(Color c, Square s) { return Square(s ^ (c * 56)); }

constexpr Rank relative_rank(Color c, Rank r) { return Rank(r ^ (c * 7)); }
#else
constexpr Rank relative_rank(Color c, Rank r, Rank maxRank = RANK_8) {
  return Rank(c == WHITE ? r : maxRank - r);
}
#endif

#ifndef FAIRY_STOCKFISH
constexpr Rank relative_rank(Color c, Square s) { return relative_rank(c, rank_of(s)); }
#else
constexpr Rank relative_rank(Color c, Square s, Rank maxRank = RANK_8) {
  return relative_rank(c, rank_of(s), maxRank);
}
constexpr Square relative_square(Color c, Square s, Rank maxRank = RANK_8) {
  return make_square(file_of(s), relative_rank(c, s, maxRank));
}
#endif

constexpr Direction pawn_push(Color c) { return c == WHITE ? NORTH : SOUTH; }

// Based on a congruential pseudo-random number generator
constexpr Key make_key(uint64_t seed) {
    return seed * 6364136223846793005ULL + 1442695040888963407ULL;
}

#ifndef FAIRY_STOCKFISH
enum MoveType {
#else
enum MoveType : int {
#endif
    NORMAL,
#ifndef FAIRY_STOCKFISH
    PROMOTION  = 1 << 14,
    EN_PASSANT = 2 << 14,
    CASTLING   = 3 << 14
};
#else
    EN_PASSANT          = 1 << (2 * SQUARE_BITS),
    CASTLING           = 2 << (2 * SQUARE_BITS),
    PROMOTION          = 3 << (2 * SQUARE_BITS),
    DROP               = 4 << (2 * SQUARE_BITS),
    PIECE_PROMOTION    = 5 << (2 * SQUARE_BITS),
    PIECE_DEMOTION     = 6 << (2 * SQUARE_BITS),
    SPECIAL            = 7 << (2 * SQUARE_BITS),
};
#endif

// A move needs 16 bits to be stored
//
// bit  0- 5: destination square (from 0 to 63)
// bit  6-11: origin square (from 0 to 63)
// bit 12-13: promotion piece type - 2 (from KNIGHT-2 to QUEEN-2)
// bit 14-15: special move flag: promotion (1), en passant (2), castling (3)
// NOTE: en passant bit is set only when a pawn can be captured
//
// Special cases are Move::none() and Move::null(). We can sneak these in because in
// any normal move destination square is always different from origin square
// while Move::none() and Move::null() have the same origin and destination square.
class Move {
   public:
    Move() = default;
    constexpr explicit Move(std::uint16_t d) :
        data(d) {}

    constexpr Move(Square from, Square to) :
        data((from << 6) + to) {}

    template<MoveType T>
    static constexpr Move make(Square from, Square to, PieceType pt = KNIGHT) {
        return Move(T + ((pt - KNIGHT) << 12) + (from << 6) + to);
    }

    constexpr Square from_sq() const {
        assert(is_ok());
        return Square((data >> 6) & 0x3F);
    }

    constexpr Square to_sq() const {
        assert(is_ok());
        return Square(data & 0x3F);
    }

    constexpr int from_to() const { return data & 0xFFF; }

    constexpr MoveType type_of() const { return MoveType(data & (3 << 14)); }

    constexpr PieceType promotion_type() const { return PieceType(((data >> 12) & 3) + KNIGHT); }

    constexpr bool is_ok() const { return none().data != data && null().data != data; }

    static constexpr Move null() {
#ifndef FAIRY_STOCKFISH
        return Move(65);
#else
        return Move(1 + (1 << SQUARE_BITS));
#endif
    }
    static constexpr Move none() { return Move(0); }

    constexpr bool operator==(const Move& m) const { return data == m.data; }
    constexpr bool operator!=(const Move& m) const { return data != m.data; }

    constexpr explicit operator bool() const { return data != 0; }

    constexpr std::uint16_t raw() const { return data; }

    struct MoveHash {
        std::size_t operator()(const Move& m) const { return make_key(m.data); }
    };

   protected:
    std::uint16_t data;
};
#ifdef FAIRY_STOCKFISH
constexpr MoveType type_of(Move m) {
  return MoveType(m.raw() & (15 << (2 * SQUARE_BITS)));
}

inline bool is_ok(Move m) {
  return (m != Move::none() && m != Move::null()) || type_of(m) == PROMOTION || type_of(m) == SPECIAL; // Catch MOVE_NULL and MOVE_NONE
}

constexpr Square to_sq(Move m) {
  assert(is_ok(m));
  return Square(m.raw() & SQUARE_BIT_MASK);
}

constexpr Square from_sq(Move m) {
  return type_of(m) == DROP ? SQ_NONE : Square((m.raw() >> SQUARE_BITS) & SQUARE_BIT_MASK);
}

inline int from_to(Move m) {
 return to_sq(m) + (from_sq(m) << SQUARE_BITS);
}

inline PieceType promotion_type(Move m) {
  return type_of(m) == PROMOTION ? PieceType((m.raw() >> (2 * SQUARE_BITS + MOVE_TYPE_BITS)) & (PIECE_TYPE_NB - 1)) : NO_PIECE_TYPE;
}

inline PieceType gating_type(Move m) {
  return PieceType((m.raw() >> (2 * SQUARE_BITS + MOVE_TYPE_BITS)) & (PIECE_TYPE_NB - 1));
}

inline Square gating_square(Move m) {
  return Square((m.raw() >> (2 * SQUARE_BITS + MOVE_TYPE_BITS + PIECE_TYPE_BITS)) & SQUARE_BIT_MASK);
}

inline bool is_gating(Move m) {
  return gating_type(m) && (type_of(m) == NORMAL || type_of(m) == CASTLING);
}

inline bool is_pass(Move m) {
  return type_of(m) == SPECIAL && from_sq(m) == to_sq(m);
}

constexpr Move make_move(Square from, Square to) {
  return Move((from << SQUARE_BITS) + to);
}

template<MoveType T>
inline Move make(Square from, Square to, PieceType pt = NO_PIECE_TYPE) {
  return Move((pt << (2 * SQUARE_BITS + MOVE_TYPE_BITS)) + T + (from << SQUARE_BITS) + to);
}

constexpr Move reverse_move(Move m) {
  return make_move(to_sq(m), from_sq(m));
}

constexpr Move make_drop(Square to, PieceType pt_in_hand, PieceType pt_dropped) {
  return Move((pt_in_hand << (2 * SQUARE_BITS + MOVE_TYPE_BITS + PIECE_TYPE_BITS)) + (pt_dropped << (2 * SQUARE_BITS + MOVE_TYPE_BITS)) + DROP + to);
}

template<MoveType T>
constexpr Move make_gating(Square from, Square to, PieceType pt, Square gate) {
  return Move((gate << (2 * SQUARE_BITS + MOVE_TYPE_BITS + PIECE_TYPE_BITS)) + (pt << (2 * SQUARE_BITS + MOVE_TYPE_BITS)) + T + (from << SQUARE_BITS) + to);
}

constexpr PieceType dropped_piece_type(Move m) {
  return PieceType((m.raw() >> (2 * SQUARE_BITS + MOVE_TYPE_BITS)) & (PIECE_TYPE_NB - 1));
}

constexpr PieceType in_hand_piece_type(Move m) {
  return PieceType((m.raw() >> (2 * SQUARE_BITS + MOVE_TYPE_BITS + PIECE_TYPE_BITS)) & (PIECE_TYPE_NB - 1));
}

inline bool is_custom(PieceType pt) {
  return pt >= CUSTOM_PIECES && pt <= CUSTOM_PIECES_END;
}

inline int dist(Direction d) {
  return std::abs(d % NORTH) < NORTH / 2 ? std::max(std::abs(d / NORTH), int(std::abs(d % NORTH)))
      : std::max(std::abs(d / NORTH) + 1, int(NORTH - std::abs(d % NORTH)));
}
#endif
}  // namespace Stockfish

#endif  // #ifndef TYPES_H_INCLUDED

#include "tune.h"  // Global visibility to tuning setup
