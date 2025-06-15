/*
  Fairy-Stockfish, a UCI chess variant playing engine derived from Stockfish
  Copyright (C) 2018-2022 Fabian Fichter

  Fairy-Stockfish is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Fairy-Stockfish is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

#include "parser.h"
#include "piece.h"
#include "variant.h"

using std::string;

namespace Stockfish {

VariantMap variants; // Global object

namespace {
    // Base variant
    Variant* variant_base() {
        Variant* v = new Variant();
        return v;
    }
    // Base for all fairy variants
    Variant* chess_variant_base() {
        Variant* v = variant_base()->init();
        v->pieceToCharTable = "PNBRQ................Kpnbrq................k";
        return v;
    }
    // Standard chess
    // https://en.wikipedia.org/wiki/Chess
    Variant* chess_variant() {
        Variant* v = chess_variant_base()->init();
        v->nnueAlias = "nn-";
        return v;
    }
    // Chess960 aka Fischer random chess
    // https://en.wikipedia.org/wiki/Fischer_random_chess
    Variant* chess960_variant() {
        Variant* v = chess_variant()->init();
        v->chess960 = true;
        v->nnueAlias = "nn-";
        return v;
    }
    // Standard chess without castling
    Variant* nocastle_variant() {
        Variant* v = chess_variant()->init();
        v->startFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - - 0 1";
        //v->castling = false;
        return v;
    }
    // Armageddon Chess
    // https://en.wikipedia.org/wiki/Fast_chess#Armageddon
    Variant* armageddon_variant() {
        Variant* v = chess_variant()->init();
        v->materialCounting = BLACK_DRAW_ODDS;
        return v;
    }
    // Berolina Chess
    // https://www.chessvariants.com/dpieces.dir/berlin.html
    Variant* berolina_variant() {
        Variant* v = chess_variant_base()->init();
        v->remove_piece(PAWN);
        v->add_piece(CUSTOM_PIECE_1, 'p', "mfFcfeWimfnA");
        v->promotionPawnType[WHITE] = v->promotionPawnType[BLACK] = CUSTOM_PIECE_1;
        v->promotionPawnTypes[WHITE] = v->promotionPawnTypes[BLACK] = piece_set(CUSTOM_PIECE_1);
        v->enPassantTypes[WHITE] = v->enPassantTypes[BLACK] = piece_set(CUSTOM_PIECE_1);
        v->nMoveRuleTypes[WHITE] = v->nMoveRuleTypes[BLACK] = piece_set(CUSTOM_PIECE_1);
        return v;
    }
    // Pawnsideways
    // https://arxiv.org/abs/2009.04374
    Variant* pawnsideways_variant() {
        Variant* v = chess_variant_base()->init();
        v->remove_piece(PAWN);
        v->add_piece(CUSTOM_PIECE_1, 'p', "fsmWfceFifmnD");
        v->promotionPawnType[WHITE] = v->promotionPawnType[BLACK] = CUSTOM_PIECE_1;
        v->promotionPawnTypes[WHITE] = v->promotionPawnTypes[BLACK] = piece_set(CUSTOM_PIECE_1);
        v->enPassantTypes[WHITE] = v->enPassantTypes[BLACK] = piece_set(CUSTOM_PIECE_1);
        v->nMoveRuleTypes[WHITE] = v->nMoveRuleTypes[BLACK] = piece_set(CUSTOM_PIECE_1);
        return v;
    }
    // Pawnback
    // https://arxiv.org/abs/2009.04374
    Variant* pawnback_variant() {
        Variant* v = chess_variant_base()->init();
        v->remove_piece(PAWN);
        v->add_piece(CUSTOM_PIECE_1, 'p', "fbmWfceFifmnD");
        v->mobilityRegion[WHITE][CUSTOM_PIECE_1] = (Rank2BB | Rank3BB | Rank4BB | Rank5BB | Rank6BB | Rank7BB | Rank8BB);
        v->mobilityRegion[BLACK][CUSTOM_PIECE_1] = (Rank7BB | Rank6BB | Rank5BB | Rank4BB | Rank3BB | Rank2BB | Rank1BB);
        v->promotionPawnType[WHITE] = v->promotionPawnType[BLACK] = CUSTOM_PIECE_1;
        v->promotionPawnTypes[WHITE] = v->promotionPawnTypes[BLACK] = piece_set(CUSTOM_PIECE_1);
        v->enPassantTypes[WHITE] = v->enPassantTypes[BLACK] = piece_set(CUSTOM_PIECE_1);
        v->nMoveRuleTypes[WHITE] = v->nMoveRuleTypes[BLACK] = NO_PIECE_SET; // backwards pawn moves are reversible
        return v;
    }

    // Pseudo-variant only used for endgame initialization
    Variant* fairy_variant() {
        Variant* v = chess_variant_base()->init();
        v->add_piece(SILVER, 's');
        v->add_piece(FERS, 'f');
        v->add_piece(ARCHBISHOP, 'a');
        v->add_piece(CHANCELLOR, 'c');
        v->add_piece(COMMONER, 'm');
        return v;
    }

    // Amazon chess
    // The queen has the additional power of moving like a knight.
    // https://www.chessvariants.com/diffmove.dir/amazone.html
    Variant* amazon_variant() {
        Variant* v = chess_variant_base()->init();
        v->pieceToCharTable = "PNBR..............AKpnbr..............ak";
        v->remove_piece(QUEEN);
        v->add_piece(AMAZON, 'a');
        v->startFen = "rnbakbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBAKBNR w KQkq - 0 1";
        v->promotionPieceTypes[WHITE] = piece_set(AMAZON) | ROOK | BISHOP | KNIGHT;
        v->promotionPieceTypes[BLACK] = piece_set(AMAZON) | ROOK | BISHOP | KNIGHT;
        return v;
    }
    // Nightrider chess
    // Knights are replaced by nightriders.
    // https://en.wikipedia.org/wiki/Nightrider_(chess)
    Variant* nightrider_variant() {
        Variant* v = chess_variant_base()->init();
        v->remove_piece(KNIGHT);
        v->add_piece(CUSTOM_PIECE_1, 'n', "NN");
        v->promotionPieceTypes[WHITE] = piece_set(QUEEN) | ROOK | BISHOP | CUSTOM_PIECE_1;
        v->promotionPieceTypes[BLACK] = piece_set(QUEEN) | ROOK | BISHOP | CUSTOM_PIECE_1;
        return v;
    }

    // Hoppel-Poppel
    // A variant from Germany where knights capture like bishops and vice versa
    // https://www.chessvariants.com/diffmove.dir/hoppel-poppel.html
    Variant* hoppelpoppel_variant() {
        Variant* v = chess_variant_base()->init();
        v->remove_piece(KNIGHT);
        v->remove_piece(BISHOP);
        v->add_piece(KNIBIS, 'n');
        v->add_piece(BISKNI, 'b');
        v->promotionPieceTypes[WHITE] = piece_set(QUEEN) | ROOK | BISKNI | KNIBIS;
        v->promotionPieceTypes[BLACK] = piece_set(QUEEN) | ROOK | BISKNI | KNIBIS;
        return v;
    }
    // New Zealand
    // Knights capture like rooks and vice versa.
    Variant* newzealand_variant() {
        Variant* v = chess_variant_base()->init();
        v->remove_piece(ROOK);
        v->remove_piece(KNIGHT);
        v->add_piece(ROOKNI, 'r');
        v->add_piece(KNIROO, 'n');
        v->castlingRookPieces[WHITE] = v->castlingRookPieces[BLACK] = piece_set(ROOKNI);
        v->promotionPieceTypes[WHITE] = piece_set(QUEEN) | ROOKNI | BISHOP | KNIROO;
        v->promotionPieceTypes[BLACK] = piece_set(QUEEN) | ROOKNI | BISHOP | KNIROO;
        return v;
    }
    // King of the Hill
    // https://lichess.org/variant/kingOfTheHill
    Variant* kingofthehill_variant() {
        Variant* v = chess_variant_base()->init();
        v->flagPiece[WHITE] = v->flagPiece[BLACK] = KING;
        v->flagRegion[WHITE] = (Rank4BB | Rank5BB) & (FileDBB | FileEBB);
        v->flagRegion[BLACK] = (Rank4BB | Rank5BB) & (FileDBB | FileEBB);
        v->flagMove = false;
        return v;
    }
    // Knightmate
    // https://www.chessvariants.com/diffobjective.dir/knightmate.html
    Variant* knightmate_variant() {
        Variant* v = chess_variant_base()->init();
        v->add_piece(COMMONER, 'm');
        v->remove_piece(KNIGHT);
        v->startFen = "rmbqkbmr/pppppppp/8/8/8/8/PPPPPPPP/RMBQKBMR w KQkq - 0 1";
        v->kingType = KNIGHT;
        v->castlingKingPiece[WHITE] = v->castlingKingPiece[BLACK] = KING;
        v->promotionPieceTypes[WHITE] = piece_set(COMMONER) | QUEEN | ROOK | BISHOP;
        v->promotionPieceTypes[BLACK] = piece_set(COMMONER) | QUEEN | ROOK | BISHOP;
        return v;
    }
    // Misere chess
    // Get checkmated to win.
    // Variant used to run some selfmate analysis http://www.kotesovec.cz/gustav/gustav_alybadix.htm
    Variant* misere_variant() {
        Variant* v = chess_variant_base()->init();
        v->checkmateValue = VALUE_MATE;
        v->endgameEval = EG_EVAL_MISERE;
        return v;
    }

    // Extinction chess
    // https://en.wikipedia.org/wiki/Extinction_chess
    Variant* extinction_variant() {
        Variant* v = chess_variant_base()->init();
        v->remove_piece(KING);
        v->add_piece(COMMONER, 'k');
        v->castlingKingPiece[WHITE] = v->castlingKingPiece[BLACK] = COMMONER;
        v->promotionPieceTypes[WHITE] = piece_set(COMMONER) | QUEEN | ROOK | BISHOP | KNIGHT;
        v->promotionPieceTypes[BLACK] = piece_set(COMMONER) | QUEEN | ROOK | BISHOP | KNIGHT;
        v->extinctionValue = -VALUE_MATE;
        v->extinctionPieceTypes = piece_set(COMMONER) | QUEEN | ROOK | BISHOP | KNIGHT | PAWN;
        return v;
    }
    // Kinglet
    // https://en.wikipedia.org/wiki/V._R._Parton#Kinglet_chess
    Variant* kinglet_variant() {
        Variant* v = extinction_variant()->init();
        v->promotionPieceTypes[WHITE] = piece_set(COMMONER);
        v->promotionPieceTypes[BLACK] = piece_set(COMMONER);
        v->extinctionPieceTypes = piece_set(PAWN);
        return v;
    }
    // Three Kings Chess
    // https://github.com/cutechess/cutechess/blob/master/projects/lib/src/board/threekingsboard.h
    Variant* threekings_variant() {
        Variant* v = chess_variant_base()->init();
        v->remove_piece(KING);
        v->add_piece(COMMONER, 'k');
        v->castlingKingPiece[WHITE] = v->castlingKingPiece[BLACK] = COMMONER;
        v->startFen = "knbqkbnk/pppppppp/8/8/8/8/PPPPPPPP/KNBQKBNK w - - 0 1";
        v->extinctionValue = -VALUE_MATE;
        v->extinctionPieceTypes = piece_set(COMMONER);
        v->extinctionPieceCount = 2;
        return v;
    }
    // Petrified
    // Sideways pawns + petrification on capture
    // https://www.chess.com/variants/petrified
    Variant* petrified_variant() {
        Variant* v = pawnsideways_variant()->init();
        v->remove_piece(KING);
        v->add_piece(COMMONER, 'k');
        v->castlingKingPiece[WHITE] = v->castlingKingPiece[BLACK] = COMMONER;
        v->extinctionValue = -VALUE_MATE;
        v->extinctionPieceTypes = piece_set(COMMONER);
        v->extinctionPseudoRoyal = true;
        v->petrifyOnCaptureTypes = piece_set(COMMONER) | QUEEN | ROOK | BISHOP | KNIGHT;
        return v;
    }

    Variant* fox_and_hounds_variant() { //https://boardgamegeek.com/boardgame/148180/fox-and-hounds
        Variant* v = chess_variant_base()->init();
        v->reset_pieces();
        v->add_piece(CUSTOM_PIECE_1, 'h', "mfF"); //Hound
        v->add_piece(CUSTOM_PIECE_2, 'f', "mF"); //Fox
        v->startFen = "1h1h1h1h/8/8/8/8/8/8/4F3 w - - 0 1";
        v->stalemateValue = -VALUE_MATE;
        v->flagPiece[WHITE] = CUSTOM_PIECE_2;
        v->flagRegion[WHITE] = Rank8BB;
        return v;
    }

    // Three-check chess
    // Check the king three times to win
    // https://lichess.org/variant/threeCheck
    Variant* threecheck_variant() {
        Variant* v = chess_variant_base()->init();
        v->startFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 3+3 0 1";
        v->checkCounting = true;
        return v;
    }
    // Five-check chess
    // Check the king five times to win
    Variant* fivecheck_variant() {
        Variant* v = threecheck_variant()->init();
        v->startFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 5+5 0 1";
        v->nnueAlias = "3check";
        return v;
    }

    // Paradigm chess30
    // 8x8 variant with a bishop+horse hybrid piece replacing bishops
    // https://www.chessvariants.com/rules/paradigm-chess30
    Variant* paradigm_variant() {
        Variant *v = chess_variant_base()->init();
        v->remove_piece(BISHOP);
        v->add_piece(CUSTOM_PIECE_1, 'b', "BnN");
        v->promotionPieceTypes[WHITE] = piece_set(QUEEN) | CUSTOM_PIECE_1 | ROOK | KNIGHT;
        v->promotionPieceTypes[BLACK] = piece_set(QUEEN) | CUSTOM_PIECE_1 | ROOK | KNIGHT;
        return v;
    }

    // Almost chess
    // Queens are replaced by chancellors
    // https://en.wikipedia.org/wiki/Almost_chess
    Variant* almost_variant() {
        Variant* v = chess_variant_base()->init();
        v->pieceToCharTable = "PNBR............CKpnbr............ck";
        v->remove_piece(QUEEN);
        v->add_piece(CHANCELLOR, 'c');
        v->startFen = "rnbckbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBCKBNR w KQkq - 0 1";
        v->promotionPieceTypes[WHITE] = piece_set(CHANCELLOR) | ROOK | BISHOP | KNIGHT;
        v->promotionPieceTypes[BLACK] = piece_set(CHANCELLOR) | ROOK | BISHOP | KNIGHT;
        return v;
    }
    // Sort of almost chess
    // One queen is replaced by a chancellor
    // https://en.wikipedia.org/wiki/Almost_chess#Sort_of_almost_chess
    Variant* sortofalmost_variant() {
        Variant* v = chess_variant();
        v->pieceToCharTable = "PNBRQ...........CKpnbrq...........ck";
        v->add_piece(CHANCELLOR, 'c');
        v->startFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBCKBNR w KQkq - 0 1";
        v->promotionPieceTypes[WHITE] = piece_set(CHANCELLOR) | ROOK | BISHOP | KNIGHT;
        v->promotionPieceTypes[BLACK] = piece_set(QUEEN) | ROOK | BISHOP | KNIGHT;
        return v;
    }
    // Chigorin chess
    // Asymmetric variant with knight vs. bishop movements
    // https://www.chessvariants.com/diffsetup.dir/chigorin.html
    Variant* chigorin_variant() {
        Variant* v = chess_variant_base()->init();
        v->pieceToCharTable = "PNBR............CKpnbrq............k";
        v->add_piece(CHANCELLOR, 'c');
        v->startFen = "rbbqkbbr/pppppppp/8/8/8/8/PPPPPPPP/RNNCKNNR w KQkq - 0 1";
        v->promotionPieceTypes[WHITE] = piece_set(CHANCELLOR) | ROOK | KNIGHT;
        v->promotionPieceTypes[BLACK] = piece_set(QUEEN) | ROOK | BISHOP;
        return v;
    }
    // Perfect chess
    // https://www.chessvariants.com/diffmove.dir/perfectchess.html
    Variant* perfect_variant() {
        Variant* v = chess_variant_base()->init();
        v->add_piece(CHANCELLOR, 'c');
        v->add_piece(ARCHBISHOP, 'm');
        v->add_piece(AMAZON, 'g');
        v->startFen = "cmqgkbnr/pppppppp/8/8/8/8/PPPPPPPP/CMQGKBNR w KQkq - 0 1";
        v->promotionPieceTypes[WHITE] = piece_set(AMAZON) | CHANCELLOR | ARCHBISHOP | QUEEN | ROOK | BISHOP | KNIGHT;
        v->promotionPieceTypes[BLACK] = piece_set(AMAZON) | CHANCELLOR | ARCHBISHOP | QUEEN | ROOK | BISHOP | KNIGHT;
        v->castlingRookPieces[WHITE] = v->castlingRookPieces[BLACK] |= piece_set(CHANCELLOR);
        return v;
    }
    // Spartan chess
    // https://www.chessvariants.com/rules/spartan-chess
    Variant* spartan_variant() {
        Variant* v = threekings_variant()->init();
        v->add_piece(DRAGON, 'g');
        v->add_piece(ARCHBISHOP, 'w');
        v->add_piece(CUSTOM_PIECE_1, 'h', "fmFfcWimA");
        v->add_piece(CUSTOM_PIECE_2, 'l', "FAsmW");
        v->add_piece(CUSTOM_PIECE_3, 'c', "WD");
        v->startFen = "lgkcckwl/hhhhhhhh/8/8/8/8/PPPPPPPP/RNBQKBNR w KQ - 0 1";
        v->promotionPawnType[BLACK] = CUSTOM_PIECE_1;
        v->promotionPawnTypes[BLACK] = piece_set(CUSTOM_PIECE_1);
        v->nMoveRuleTypes[BLACK] = piece_set(CUSTOM_PIECE_1);
        v->promotionPieceTypes[BLACK] = piece_set(COMMONER) | DRAGON | ARCHBISHOP | CUSTOM_PIECE_2 | CUSTOM_PIECE_3;
        v->promotionLimit[COMMONER] = 2;
        v->enPassantRegion = 0;
        v->extinctionPieceCount = 0;
        v->extinctionPseudoRoyal = true;
        v->dupleCheck = true;
        return v;
    }

    // Coregal chess
    // Queens are also subject to check and checkmate
    // https://www.chessvariants.com/winning.dir/coregal.html
    Variant* coregal_variant() {
        Variant* v = chess_variant_base()->init();
        v->extinctionValue = -VALUE_MATE;
        v->extinctionPieceTypes = piece_set(QUEEN);
        v->extinctionPseudoRoyal = true;
        v->extinctionPieceCount = 64; // no matter how many queens, all are royal
        return v;
    }

#ifdef LARGEBOARDS
    // Capablanca chess
    // https://en.wikipedia.org/wiki/Capablanca_chess
    Variant* capablanca_variant() {
        Variant* v = chess_variant_base()->init();
        v->pieceToCharTable = "PNBRQ..AC............Kpnbrq..ac............k";
        v->maxRank = RANK_8;
        v->maxFile = FILE_J;
        v->castlingKingsideFile = FILE_I;
        v->castlingQueensideFile = FILE_C;
        v->add_piece(ARCHBISHOP, 'a');
        v->add_piece(CHANCELLOR, 'c');
        v->startFen = "rnabqkbcnr/pppppppppp/10/10/10/10/PPPPPPPPPP/RNABQKBCNR w KQkq - 0 1";
        v->promotionPieceTypes[WHITE] = piece_set(ARCHBISHOP) | CHANCELLOR | QUEEN | ROOK | BISHOP | KNIGHT;
        v->promotionPieceTypes[BLACK] = piece_set(ARCHBISHOP) | CHANCELLOR | QUEEN | ROOK | BISHOP | KNIGHT;
        return v;
    }
    // Capablanca random chess (CRC)
    // Shuffle variant of capablanca chess
    // https://en.wikipedia.org/wiki/Capablanca_random_chess
    Variant* caparandom_variant() {
        Variant* v = capablanca_variant()->init();
        v->chess960 = true;
        v->nnueAlias = "capablanca";
        return v;
    }
    // Gothic chess
    // Capablanca chess with changed starting position
    // https://www.chessvariants.com/large.dir/gothicchess.html
    Variant* gothic_variant() {
        Variant* v = capablanca_variant()->init();
        v->startFen = "rnbqckabnr/pppppppppp/10/10/10/10/PPPPPPPPPP/RNBQCKABNR w KQkq - 0 1";
        v->nnueAlias = "capablanca";
        return v;
    }
    // Janus chess
    // 10x8 variant with two archbishops per side
    // https://en.wikipedia.org/wiki/Janus_Chess
    Variant* janus_variant() {
        Variant* v = chess_variant_base()->init();
        v->pieceToCharTable = "PNBRQ............J...Kpnbrq............j...k";
        v->maxRank = RANK_8;
        v->maxFile = FILE_J;
        v->castlingKingsideFile = FILE_I;
        v->castlingQueensideFile = FILE_B;
        v->add_piece(ARCHBISHOP, 'j');
        v->startFen = "rjnbkqbnjr/pppppppppp/10/10/10/10/PPPPPPPPPP/RJNBKQBNJR w KQkq - 0 1";
        v->promotionPieceTypes[WHITE] = piece_set(ARCHBISHOP) | QUEEN | ROOK | BISHOP | KNIGHT;
        v->promotionPieceTypes[BLACK] = piece_set(ARCHBISHOP) | QUEEN | ROOK | BISHOP | KNIGHT;
        return v;
    }
    // Embassy chess
    // Capablanca chess with different starting position
    // https://en.wikipedia.org/wiki/Embassy_chess
    Variant* embassy_variant() {
        Variant* v = capablanca_variant()->init();
        v->castlingKingsideFile = FILE_H;
        v->castlingQueensideFile = FILE_B;
        v->startFen = "rnbqkcabnr/pppppppppp/10/10/10/10/PPPPPPPPPP/RNBQKCABNR w KQkq - 0 1";
        v->nnueAlias = "capablanca";
        return v;
    }
    // Centaur chess (aka Royal Court)
    // 10x8 variant with a knight+commoner compound
    // https://www.chessvariants.com/large.dir/contest/royalcourt.html
    Variant* centaur_variant() {
        Variant* v = chess_variant_base()->init();
        v->pieceToCharTable = "PNBRQ...............CKpnbrq...............ck";
        v->maxRank = RANK_8;
        v->maxFile = FILE_J;
        v->castlingKingsideFile = FILE_I;
        v->castlingQueensideFile = FILE_C;
        v->add_piece(CENTAUR, 'c');
        v->startFen = "rcnbqkbncr/pppppppppp/10/10/10/10/PPPPPPPPPP/RCNBQKBNCR w KQkq - 0 1";
        v->promotionPieceTypes[WHITE] = piece_set(CENTAUR) | QUEEN | ROOK | BISHOP | KNIGHT;
        v->promotionPieceTypes[BLACK] = piece_set(CENTAUR) | QUEEN | ROOK | BISHOP | KNIGHT;
        return v;
    }
    // Gustav III chess
    // 10x8 variant with an amazon piece and wall squares
    // https://www.chessvariants.com/play/gustav-iiis-chess
    Variant* gustav3_variant() {
        Variant* v = chess_variant_base()->init();
        v->pieceToCharTable = "PNBRQ.............AKpnbrq.............ak";
        v->maxRank = RANK_8;
        v->maxFile = FILE_J;
        v->castlingKingsideFile = FILE_H;
        v->castlingQueensideFile = FILE_D;
        v->add_piece(AMAZON, 'a');
        v->startFen = "arnbqkbnra/*pppppppp*/*8*/*8*/*8*/*8*/*PPPPPPPP*/ARNBQKBNRA w KQkq - 0 1";
        v->promotionPieceTypes[WHITE] = piece_set(AMAZON) | QUEEN | ROOK | BISHOP | KNIGHT;
        v->promotionPieceTypes[BLACK] = piece_set(AMAZON) | QUEEN | ROOK | BISHOP | KNIGHT;
        return v;
    }

    // Janggi (Korean chess)
    // https://en.wikipedia.org/wiki/Janggi
    // Official tournament rules with bikjang and material counting.
    Variant* janggi_variant() {
        Variant* v = new Variant();
        v->reset_pieces();
        v->add_piece(ROOK, 'r');
        v->add_piece(HORSE, 'n', 'h');
        v->add_piece(KING, 'k');
        v->add_piece(SOLDIER, 'p');
        v->add_piece(WAZIR, 'a');
        v->add_piece(JANGGI_CANNON, 'c');
        v->add_piece(JANGGI_ELEPHANT, 'b', 'e');
        
        v->variantTemplate = "janggi";
        v->pieceToCharTable = ".N.R.AB.P..C.........K.n.r.ab.p..c.........k";
        v->startFen = "rnba1abnr/4k4/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/4K4/RNBA1ABNR w - - 0 1";

        v->kingType = WAZIR;
        // v->doubleStep = false;
        //v->castling = false;
        v->stalemateValue = -VALUE_MATE;
        v->maxRank = RANK_10;
        v->maxFile = FILE_I;
        v->mobilityRegion[WHITE][KING] = (Rank1BB | Rank2BB | Rank3BB) & (FileDBB | FileEBB | FileFBB);
        v->mobilityRegion[BLACK][KING] = (Rank8BB | Rank9BB | Rank10BB) & (FileDBB | FileEBB | FileFBB);
        v->mobilityRegion[WHITE][WAZIR] = v->mobilityRegion[WHITE][KING];
        v->mobilityRegion[BLACK][WAZIR] = v->mobilityRegion[BLACK][KING];
        
        v->soldierPromotionRank = RANK_1;
        v->flyingGeneral = false;
        v->bikjangRule = true;
        v->materialCounting = JANGGI_MATERIAL;
        v->diagonalLines = make_bitboard(SQ_D1, SQ_F1, SQ_E2, SQ_D3, SQ_F3,
                                         SQ_D8, SQ_F8, SQ_E9, SQ_D10, SQ_F10);
        v->pass[WHITE] = true;
        v->pass[BLACK] = true;
        v->nFoldValue = VALUE_DRAW;
        v->perpetualCheckIllegal = true;
        return v;
    }
    // Traditional rules of Janggi, where bikjang is a draw
    Variant* janggi_traditional_variant() {
        Variant* v = janggi_variant()->init();
        v->bikjangRule = true;
        v->materialCounting = NO_MATERIAL_COUNTING;
        v->nnueAlias = "janggi";
        return v;
    }
    // Modern rules of Janggi, where bikjang is not considered, but material counting is.
    // The repetition rules are also adjusted for better compatibility with Kakao Janggi.
    Variant* janggi_modern_variant() {
        Variant* v = janggi_variant()->init();
        v->bikjangRule = false;
        v->materialCounting = JANGGI_MATERIAL;
        v->moveRepetitionIllegal = true;
        v->nFoldRule = 4; // avoid nFold being triggered before move repetition
        v->nMoveRule = 100; // avoid adjudication before reaching 200 half-moves
        v->nnueAlias = "janggi";
        return v;
    }
    // Casual rules of Janggi, where bikjang and material counting are not considered
    Variant* janggi_casual_variant() {
        Variant* v = janggi_variant()->init();
        v->bikjangRule = false;
        v->materialCounting = NO_MATERIAL_COUNTING;
        v->nnueAlias = "janggi";
        return v;
    }
#endif

} // namespace


/// VariantMap::init() is called at startup to initialize all predefined variants

void VariantMap::init() {
    // Add to UCI_Variant option
    add("fairy", fairy_variant()); // fairy variant used for endgame code initialization
    // add("janggi", janggi_variant());
    // add("janggitraditional", janggi_traditional_variant());
    add("janggimodern", janggi_modern_variant());
    // add("janggicasual", janggi_casual_variant());
}


// Pre-calculate derived properties
Variant* Variant::conclude() {

    // Determine optimizations
    bool restrictedMobility = false;
    for (PieceSet ps = pieceTypes; !restrictedMobility && ps;)
    {
        PieceType pt = pop_lsb(ps);
        if (mobilityRegion[WHITE][pt] || mobilityRegion[BLACK][pt])
          restrictedMobility = true;
    }
    fastAttacks =  !(pieceTypes & ~(CHESS_PIECES | COMMON_FAIRY_PIECES))
                  && kingType == KING
                  && !restrictedMobility
                  && !cambodianMoves
                  && !diagonalLines;
    fastAttacks2 =  !(pieceTypes & ~(SHOGI_PIECES | COMMON_STEP_PIECES))
                  && kingType == KING
                  && !restrictedMobility
                  && !cambodianMoves
                  && !diagonalLines;

    // Initialize calculated NNUE properties
    nnueKing =  pieceTypes & KING ? KING
              : extinctionPieceCount == 0 && (extinctionPieceTypes & COMMONER) ? COMMONER
              : NO_PIECE_TYPE;
    // The nnueKing has to present exactly once and must not change in count
    if (nnueKing != NO_PIECE_TYPE)
    {
        // If the nnueKing is involved in promotion, count might change
        if (   ((promotionPawnTypes[WHITE] | promotionPawnTypes[BLACK]) & nnueKing)
            || ((promotionPieceTypes[WHITE] | promotionPieceTypes[BLACK]) & nnueKing)
            || std::find(std::begin(promotedPieceType), std::end(promotedPieceType), nnueKing) != std::end(promotedPieceType))
            nnueKing = NO_PIECE_TYPE;
    }
    if (nnueKing != NO_PIECE_TYPE)
    {
        std::string fenBoard = startFen.substr(0, startFen.find(' '));
        // Switch NNUE from KA to A if there is no unique piece
        if (   std::count(fenBoard.begin(), fenBoard.end(), pieceToChar[make_piece(WHITE, nnueKing)]) != 1
            || std::count(fenBoard.begin(), fenBoard.end(), pieceToChar[make_piece(BLACK, nnueKing)]) != 1)
            nnueKing = NO_PIECE_TYPE;
    }
    // We can not use popcount here yet, as the lookup tables are initialized after the variants
    int nnueSquares = (maxRank + 1) * (maxFile + 1);
    nnueUsePockets = false;
    int nnuePockets = nnueUsePockets ? 2 * int(maxFile + 1) : 0;
    int nnueNonDropPieceIndices = (2 * std::bitset<64>(pieceTypes).count() - (nnueKing != NO_PIECE_TYPE)) * nnueSquares;
    int nnuePieceIndices = nnueNonDropPieceIndices + 2 * (std::bitset<64>(pieceTypes).count() - (nnueKing != NO_PIECE_TYPE)) * nnuePockets;
    int i = 0;
    for (PieceSet ps = pieceTypes; ps;)
    {
        // Make sure that the nnueKing type gets the last index, since the NNUE architecture relies on that
        PieceType pt = lsb(ps != piece_set(nnueKing) ? ps & ~piece_set(nnueKing) : ps);
        ps ^= pt;
        assert(pt != nnueKing || !ps);

        for (Color c : { WHITE, BLACK})
        {
            pieceSquareIndex[c][make_piece(c, pt)] = 2 * i * nnueSquares;
            pieceSquareIndex[c][make_piece(~c, pt)] = (2 * i + (pt != nnueKing)) * nnueSquares;
            pieceHandIndex[c][make_piece(c, pt)] = 2 * i * nnuePockets + nnueNonDropPieceIndices;
            pieceHandIndex[c][make_piece(~c, pt)] = (2 * i + 1) * nnuePockets + nnueNonDropPieceIndices;
        }
        i++;
    }

    // Map king squares to enumeration of actually available squares.
    // E.g., for xiangqi map from 0-89 to 0-8.
    // Variants might be initialized before bitboards, so do not rely on precomputed bitboards (like SquareBB).
    // Furthermore conclude() might be called on invalid configuration during validation,
    // therefore skip proper initialization in case of invalid board size.
    int nnueKingSquare = 0;
    if (nnueKing && nnueSquares <= SQUARE_NB)
        for (Square s = SQ_A1; s < nnueSquares; ++s)
        {
            Square bitboardSquare = Square(s + s / (maxFile + 1) * (FILE_MAX - maxFile));
            if (   !mobilityRegion[WHITE][nnueKing] || !mobilityRegion[BLACK][nnueKing]
                || (mobilityRegion[WHITE][nnueKing] & make_bitboard(bitboardSquare))
                || (mobilityRegion[BLACK][nnueKing] & make_bitboard(relative_square(BLACK, bitboardSquare, maxRank))))
            {
                kingSquareIndex[s] = nnueKingSquare++ * nnuePieceIndices;
            }
        }
    else
        kingSquareIndex[SQ_A1] = nnueKingSquare++ * nnuePieceIndices;
    nnueDimensions = nnueKingSquare * nnuePieceIndices;

    // Determine maximum piece count
    std::istringstream ss(startFen);
    ss >> std::noskipws;
    unsigned char token;
    nnueMaxPieces = 0;
    while ((ss >> token) && !isspace(token))
    {
        if (pieceToChar.find(token) != std::string::npos || pieceToCharSynonyms.find(token) != std::string::npos)
            nnueMaxPieces++;
    }
    // For endgame evaluation to be applicable, no special win rules must apply.
    // Furthermore, rules significantly changing game mechanics also invalidate it.
    endgameEval =  endgameEval != EG_EVAL_CHESS
                 ||
                   (   endgameEval == EG_EVAL_CHESS
                    && extinctionValue == VALUE_NONE
                    && checkmateValue == -VALUE_MATE
                    && stalemateValue == VALUE_DRAW
                    && !materialCounting
                    && !(flagRegion[WHITE] || flagRegion[BLACK])
                    && !checkCounting
                    && !makpongRule
                    && !connectN
                    && !petrifyOnCaptureTypes
                    && !restrictedMobility
                    && kingType == KING
                   )
                 ? endgameEval : NO_EG_EVAL;

    shogiStylePromotions = false;
    for (PieceType current: promotedPieceType)
        if (current != NO_PIECE_TYPE)
        {
            shogiStylePromotions = true;
            break;
        }

    connect_directions.clear();
    if (connectHorizontal)
    {
        connect_directions.push_back(EAST);
    }
    if (connectVertical)
    {
        connect_directions.push_back(NORTH);
    }
    if (connectDiagonal)
    {
        connect_directions.push_back(NORTH_EAST);
        connect_directions.push_back(SOUTH_EAST);
    }

    // If not a connect variant, set connectPieceTypesTrimmed to no pieces.
    // connectPieceTypesTrimmed is separated so that connectPieceTypes is left unchanged for inheritance.
    if ( !(connectRegion1[WHITE] || connectRegion1[BLACK] || connectN || connectNxN || collinearN) )
    {
          connectPieceTypesTrimmed = NO_PIECE_SET;
    }
    //Otherwise optimize to pieces actually in the game.
    else
    {
        connectPieceTypesTrimmed = connectPieceTypes & pieceTypes;
    };

    return this;
}


/// VariantMap::parse_istream reads variants from an INI-style configuration input stream.

template <bool DoCheck>
void VariantMap::parse_istream(std::istream& file) {
    std::string variant, variant_template, key, value, input;
    while (file.peek() != '[' && std::getline(file, input)) {}

    std::vector<std::string> varsToErase = {};
    while (file.get() && std::getline(std::getline(file, variant, ']'), input))
    {
        // Extract variant template, if specified
        if (!std::getline(std::getline(std::stringstream(variant), variant, ':'), variant_template))
            variant_template = "";

        // Read variant rules
        Config attribs = {};
        while (file.peek() != '[' && std::getline(file, input))
        {
            if (!input.empty() && input.back() == '\r')
                input.pop_back();
            std::stringstream ss(input);
            if (ss.peek() != ';' && ss.peek() != '#')
            {
                if (DoCheck && !input.empty() && input.find('=') == std::string::npos)
                    std::cerr << "Invalid syntax: '" << input << "'." << std::endl;
                if (std::getline(std::getline(ss, key, '=') >> std::ws, value) && !key.empty())
                    attribs[key.erase(key.find_last_not_of(" ") + 1)] = value;
            }
        }

        // Create variant
        if (variants.find(variant) != variants.end())
            std::cerr << "Variant '" << variant << "' already exists." << std::endl;
        else if (!variant_template.empty() && variants.find(variant_template) == variants.end())
            std::cerr << "Variant template '" << variant_template << "' does not exist." << std::endl;
        else
        {
            if (DoCheck)
                std::cerr << "Parsing variant: " << variant << std::endl;
            Variant* v = !variant_template.empty() ? VariantParser<DoCheck>(attribs).parse((new Variant(*variants.find(variant_template)->second))->init())
                                                   : VariantParser<DoCheck>(attribs).parse();
            if (v->maxFile <= FILE_MAX && v->maxRank <= RANK_MAX)
            {
                add(variant, v);
                // In order to allow inheritance, we need to temporarily add configured variants
                // even when only checking them, but we remove them later after parsing is finished.
                if (DoCheck)
                    varsToErase.push_back(variant);
            }
            else
                delete v;
        }
    }
    // Clean up temporary variants
    for (std::string tempVar : varsToErase)
    {
        delete variants[tempVar];
        variants.erase(tempVar);
    }
}

/// VariantMap::parse reads variants from an INI-style configuration file.

template <bool DoCheck>
void VariantMap::parse(std::string path) {
    if (path.empty() || path == "<empty>")
        return;
    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cerr << "Unable to open file " << path << std::endl;
        return;
    }
    parse_istream<DoCheck>(file);
    file.close();
}

template void VariantMap::parse<true>(std::string path);
template void VariantMap::parse<false>(std::string path);

void VariantMap::add(std::string s, Variant* v) {
  insert(std::pair<std::string, const Variant*>(s, v->conclude()));
}

void VariantMap::clear_all() {
  for (auto const& element : *this)
      delete element.second;
  clear();
}

std::vector<std::string> VariantMap::get_keys() {
  std::vector<std::string> keys;
  for (auto const& element : *this)
      keys.push_back(element.first);
  return keys;
}

} // namespace Stockfish
