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

#ifdef FAIRY_STOCKFISH
#include <cstdlib>
#include "piece.h"
#endif
#include "uci.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <deque>
#include <memory>
#include <optional>
#include <sstream>
#include <vector>
#include <cstdint>

#include "benchmark.h"
#include "evaluate.h"
#include "movegen.h"
#include "nnue/evaluate_nnue.h"
#include "nnue/nnue_architecture.h"
#include "position.h"
#include "search.h"
#include "syzygy/tbprobe.h"
#include "types.h"
#include "ucioption.h"
#include "perft.h"

namespace Stockfish {

constexpr auto StartFEN             = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
constexpr int  NormalizeToPawnValue = 356;
constexpr int  MaxHashMB            = Is64Bit ? 33554432 : 2048;

#ifdef FAIRY_STOCKFISH
std::set<std::string> standard_variants = {
    "normal", "nocastle", "fischerandom", "knightmate", "3check", "makruk", "shatranj",
    "asean", "seirawan", "crazyhouse", "bughouse", "suicide", "giveaway", "losers", "atomic",
    "capablanca", "gothic", "janus", "caparandom", "grand", "shogi", "xiangqi", "duck",
    "berolina", "spartan"
};

void UCI::init_variant(const Variant* v) {
    pieceMap.init(v);
    Bitboards::init_pieces();
}
void on_variant_set(const Option &o) {
    // Re-initialize NNUE
    // Eval::NNUE::init();

    const Variant* v = variants.find(o)->second;
    UCI::init_variant(v);
    // PSQT::init(v);
}

void on_variant_change(const Option &o) {
    // Variant initialization
    on_variant_set(o);

    const Variant* v = variants.find(o)->second;
    // Do not send setup command for known variants
    if (standard_variants.find(o) != standard_variants.end())
        return;
    int pocketsize = 0;

    sync_cout << "info string variant " << (std::string)o
            << " files " << v->maxFile + 1
            << " ranks " << v->maxRank + 1
            << " pocket " << pocketsize
            << " template " << v->variantTemplate
            << " startpos " << v->startFen
            << sync_endl;
}

void Option::set_default(std::string newDefault) {
    defaultValue = currentValue = newDefault;

    // When changing the variant default, suppress variant definition output,
    // but still do the essential re-initialization of the variant
    //if (on_change)
    //    (on_change == on_variant_change ? on_variant_set : on_change)(*this);
}

#endif
UCI::UCI(int argc, char** argv) :
    cli(argc, argv) {

    evalFiles = {{Eval::NNUE::Big, {"EvalFile", EvalFileDefaultNameBig, "None", ""}},
                 {Eval::NNUE::Small, {"EvalFileSmall", EvalFileDefaultNameSmall, "None", ""}}};


    options["Debug Log File"] << Option("", [](const Option& o) { start_logger(o); });

    options["Threads"] << Option(1, 1, 1024, [this](const Option&) {
        threads.set({options, threads, tt});
    });

    options["Hash"] << Option(16, 1, MaxHashMB, [this](const Option& o) {
        threads.main_thread()->wait_for_search_finished();
        tt.resize(o, options["Threads"]);
    });

    options["Clear Hash"] << Option([this](const Option&) { search_clear(); });
    options["Ponder"] << Option(false);
    options["MultiPV"] << Option(1, 1, MAX_MOVES);
#ifndef FAIRY_STOCKFISH
    options["Skill Level"] << Option(20, 0, 20);
#else
    options["Skill Level"] << Option(20, -20, 20);
#endif
    options["Move Overhead"] << Option(10, 0, 5000);
    options["nodestime"] << Option(0, 0, 10000);
    options["UCI_Chess960"] << Option(false);
#ifdef FAIRY_STOCKFISH
    options["UCI_Variant"] << Option("janggimodern", variants.get_keys(), on_variant_change);
#endif
    options["UCI_LimitStrength"] << Option(false);
#ifndef FAIRY_STOCKFISH
    options["UCI_Elo"] << Option(1320, 1320, 3190);
#else
    options["UCI_Elo"] << Option(1320, 500, 3190);
#endif
    options["UCI_ShowWDL"] << Option(false);
    options["SyzygyPath"] << Option("<empty>", [](const Option& o) { Tablebases::init(o); });
    options["SyzygyProbeDepth"] << Option(1, 1, 100);
    options["Syzygy50MoveRule"] << Option(true);
    options["SyzygyProbeLimit"] << Option(7, 0, 7);
    options["EvalFile"] << Option(EvalFileDefaultNameBig, [this](const Option&) {
        evalFiles = Eval::NNUE::load_networks(cli.binaryDirectory, options, evalFiles);
    });
    options["EvalFileSmall"] << Option(EvalFileDefaultNameSmall, [this](const Option&) {
        evalFiles = Eval::NNUE::load_networks(cli.binaryDirectory, options, evalFiles);
    });
#ifdef FAIRY_STOCKFISH
    options["TsumeMode"] << Option(false);
    options["VariantPath"] << Option("<empty>", [this](const Option& o) {
        std::stringstream ss((std::string)o);
        std::string path;

        while (std::getline(ss, path, SepChar))
            variants.parse<false>(path);

        options["UCI_Variant"].set_combo(variants.get_keys());
    });
    options["usemillisec"] << Option(true); // time unit for UCCI
#endif

    threads.set({options, threads, tt});

    search_clear();  // After threads are up
}

#ifdef FAIRY_STOCKFISH
// load() is called when engine receives the "load" or "check" command.
// The function reads variant configuration files.

void UCI::load(std::istringstream& is, bool check) {

    std::string token;
    std::getline(is >> std::ws, token);

    // The argument to load either is a here-doc or a file path
    if (token.rfind("<<", 0) == 0)
    {
        // Trim the EOF marker
        if (!(std::stringstream(token.substr(2)) >> token))
            token = "";

        // Parse variant config till EOF marker
        std::stringstream ss;
        std::string line;
        while (std::getline(std::cin, line) && line != token)
            ss << line << std::endl;
        if (check)
            variants.parse_istream<true>(ss);
        else
        {
            variants.parse_istream<false>(ss);
            options["UCI_Variant"].set_combo(variants.get_keys());
        }
    }
    else
    {
        // store path if non-empty after trimming
        std::size_t end = token.find_last_not_of(' ');
        if (end != std::string::npos)
        {
            if (check)
                variants.parse<true>(token.erase(end + 1));
            else
                options["VariantPath"] = token.erase(end + 1);
        }
    }
}

#endif

void UCI::loop() {

    Position     pos;
    std::string  token, cmd;
    StateListPtr states(new std::deque<StateInfo>(1));

#ifndef FAIRY_STOCKFISH
    pos.set(StartFEN, false, &states->back());
#else
    assert(variants.find(options["UCI_Variant"])->second != nullptr);
    pos.set(variants.find(options["UCI_Variant"])->second, variants.find(options["UCI_Variant"])->second->startFen, false, &states->back());
#endif

    for (int i = 1; i < cli.argc; ++i)
        cmd += std::string(cli.argv[i]) + " ";
#ifdef FAIRY_STOCKFISH
  // UCCI banmoves state
    std::vector<Move> banmoves = {};

    if (cli.argc > 1 && (std::strcmp(cli.argv[1], "noautoload") == 0))
    {
        cmd = "";
        cli.argc = 1;
    }
    else if (cli.argc == 1 || !(std::strcmp(cli.argv[1], "load") == 0))
    {
        // Check environment for variants.ini file
        char *envVariantPath = std::getenv("FAIRY_STOCKFISH_VARIANT_PATH");
        if (envVariantPath != NULL)
            options["VariantPath"] = std::string(envVariantPath);
    }
#endif

    do
    {
        if (cli.argc == 1
            && !getline(std::cin, cmd))  // Wait for an input or an end-of-file (EOF) indication
            cmd = "quit";

        std::istringstream is(cmd);

        token.clear();  // Avoid a stale if getline() returns nothing or a blank line
        is >> std::skipws >> token;

        if (token == "quit" || token == "stop")
            threads.stop = true;

        // The GUI sends 'ponderhit' to tell that the user has played the expected move.
        // So, 'ponderhit' is sent if pondering was done on the same move that the user
        // has played. The search should continue, but should also switch from pondering
        // to the normal search.
        else if (token == "ponderhit")
            threads.main_manager()->ponder = false;  // Switch to the normal search

        else if (token == "uci")
#ifndef FAIRY_STOCKFISH
            sync_cout << "id name " << engine_info(true) << "\n"
                      << options << "\nuciok" << sync_endl;
#else
        {
            std::istringstream ss("startpos");
            position(pos, ss, states);
            sync_cout << "id name " << engine_info(true)
                      << "\n" << options
                      << "\n" << token << "ok"  << sync_endl;
          // Allow to enforce protocol at startup
            cli.argc = 1;
        }
#endif

        else if (token == "setoption")
            setoption(is);
#ifdef FAIRY_STOCKFISH
        // UCCI-specific banmoves command
        else if (token == "banmoves")
            while (is >> token)
                banmoves.push_back(UCI::to_move(pos, token));
#endif
#ifndef FAIRY_STOCKFISH
        else if (token == "go")
            go(pos, is, states);
        else if (token == "position")
            position(pos, is, states);
        else if (token == "ucinewgame")
            search_clear();
#else
        else if (token == "go")
            go(pos, is, states, banmoves);
        else if (token == "position")
            position(pos, is, states), banmoves.clear();
        else if (token == "ucinewgame" || token == "usinewgame" || token == "uccinewgame")
            search_clear();
#endif
        else if (token == "isready")
            sync_cout << "readyok" << sync_endl;

        // Add custom non-UCI commands, mainly for debugging purposes.
        // These commands must not be used during a search!
        else if (token == "flip")
            pos.flip();
        else if (token == "bench")
            bench(pos, is, states);
        else if (token == "d")
            sync_cout << pos << sync_endl;
        else if (token == "eval")
            trace_eval(pos);
        else if (token == "compiler")
            sync_cout << compiler_info() << sync_endl;
        else if (token == "export_net")
        {
            std::optional<std::string> filename;
            std::string                f;
            if (is >> std::skipws >> f)
                filename = f;
            Eval::NNUE::save_eval(filename, Eval::NNUE::Big, evalFiles);
        }
        else if (token == "--help" || token == "help" || token == "--license" || token == "license")
            sync_cout
              << "\nStockfish is a powerful chess engine for playing and analyzing."
                 "\nIt is released as free software licensed under the GNU GPLv3 License."
                 "\nStockfish is normally used with a graphical user interface (GUI) and implements"
                 "\nthe Universal Chess Interface (UCI) protocol to communicate with a GUI, an API, etc."
                 "\nFor any further information, visit https://github.com/official-stockfish/Stockfish#readme"
                 "\nor read the corresponding README.md and Copying.txt files distributed along with this program.\n"
              << sync_endl;
#ifdef FAIRY_STOCKFISH
        else if (token == "load")     { load(is); cli.argc = 1; } // continue reading stdin
        else if (token == "check")    load(is, true);
        // UCI-Cyclone omits the "position" keyword
        else if (token == "fen" || token == "startpos")
        {
            is.seekg(0);
            position(pos, is, states);
        }
#endif
        else if (!token.empty() && token[0] != '#')
            sync_cout << "Unknown command: '" << cmd << "'. Type help for more information."
                      << sync_endl;

    } while (token != "quit" && cli.argc == 1);  // The command-line arguments are one-shot
}

#ifndef FAIRY_STOCKFISH
void UCI::go(Position& pos, std::istringstream& is, StateListPtr& states) {
#else
void UCI::go(Position& pos, std::istringstream& is, StateListPtr& states, const std::vector<Move>& banmoves) {
#endif

    Search::LimitsType limits;
    std::string        token;
    bool               ponderMode = false;

    limits.startTime = now();  // The search starts as early as possible

#ifdef FAIRY_STOCKFISH
    limits.banmoves = banmoves;
    bool isUsi = false;
    int secResolution = options["usemillisec"] ? 1 : 1000;
#endif

    while (is >> token)
        if (token == "searchmoves")  // Needs to be the last command on the line
            while (is >> token)
                limits.searchmoves.push_back(to_move(pos, token));

#ifndef FAIRY_STOCKFISH
        else if (token == "wtime")
            is >> limits.time[WHITE];
        else if (token == "btime")
            is >> limits.time[BLACK];
        else if (token == "winc")
            is >> limits.inc[WHITE];
        else if (token == "binc")
            is >> limits.inc[BLACK];
#else
        else if (token == "wtime")
            is >> limits.time[isUsi ? BLACK : WHITE];
        else if (token == "btime")
            is >> limits.time[isUsi ? WHITE : BLACK];
        else if (token == "winc")
            is >> limits.inc[isUsi ? BLACK : WHITE];
        else if (token == "binc")
            is >> limits.inc[isUsi ? WHITE : BLACK];
#endif
        else if (token == "movestogo")
            is >> limits.movestogo;
        else if (token == "depth")
            is >> limits.depth;
        else if (token == "nodes")
            is >> limits.nodes;
        else if (token == "movetime")
            is >> limits.movetime;
        else if (token == "mate")
            is >> limits.mate;
        else if (token == "perft")
            is >> limits.perft;
        else if (token == "infinite")
            limits.infinite = 1;
        else if (token == "ponder")
            ponderMode = true;

#ifdef FAIRY_STOCKFISH
        // UCCI commands
        else if (token == "time")
            is >> limits.time[pos.side_to_move()], limits.time[pos.side_to_move()] *= secResolution;
        else if (token == "opptime")
            is >> limits.time[~pos.side_to_move()], limits.time[~pos.side_to_move()] *= secResolution;
        else if (token == "increment")
            is >> limits.inc[pos.side_to_move()], limits.inc[pos.side_to_move()] *= secResolution;
        else if (token == "oppincrement")
            is >> limits.inc[~pos.side_to_move()], limits.inc[~pos.side_to_move()] *= secResolution;
        // USI commands
        else if (token == "byoyomi")
        {
            int byoyomi = 0;
            is >> byoyomi;
            limits.inc[WHITE] = limits.inc[BLACK] = byoyomi;
            limits.time[WHITE] += byoyomi;
            limits.time[BLACK] += byoyomi;
        }
#endif

    Eval::NNUE::verify(options, evalFiles);

    if (limits.perft)
    {
        perft(pos.fen(), limits.perft, options["UCI_Chess960"]);
        return;
    }

    threads.start_thinking(options, pos, states, limits, ponderMode);
}

void UCI::bench(Position& pos, std::istream& args, StateListPtr& states) {
    std::string token;
    uint64_t    num, nodes = 0, cnt = 1;

#ifndef FAIRY_STOCKFISH
    std::vector<std::string> list = setup_bench(pos, args);
#else
    std::vector<std::string> list = setup_bench(pos, args, options);
#endif

    num = count_if(list.begin(), list.end(),
                   [](const std::string& s) { return s.find("go ") == 0 || s.find("eval") == 0; });

    TimePoint elapsed = now();

    for (const auto& cmd : list)
    {
        std::istringstream is(cmd);
        is >> std::skipws >> token;

        if (token == "go" || token == "eval")
        {
            std::cerr << "\nPosition: " << cnt++ << '/' << num << " (" << pos.fen() << ")"
                      << std::endl;
            if (token == "go")
            {
                go(pos, is, states);
                threads.main_thread()->wait_for_search_finished();
                nodes += threads.nodes_searched();
            }
            else
                trace_eval(pos);
        }
        else if (token == "setoption")
            setoption(is);
        else if (token == "position")
            position(pos, is, states);
        else if (token == "ucinewgame")
        {
            search_clear();  // Search::clear() may take a while
            elapsed = now();
        }
    }

    elapsed = now() - elapsed + 1;  // Ensure positivity to avoid a 'divide by zero'

    dbg_print();

    std::cerr << "\n==========================="
              << "\nTotal time (ms) : " << elapsed << "\nNodes searched  : " << nodes
              << "\nNodes/second    : " << 1000 * nodes / elapsed << std::endl;
}

void UCI::trace_eval(Position& pos) {
    StateListPtr states(new std::deque<StateInfo>(1));
    Position     p;
#ifndef FAIRY_STOCKFISH
    p.set(pos.fen(), options["UCI_Chess960"], &states->back());
#else
    p.set(pos.variant(), pos.fen(), options["UCI_Chess960"], &states->back());
#endif

    Eval::NNUE::verify(options, evalFiles);

    sync_cout << "\n" << Eval::trace(p) << sync_endl;
}

void UCI::search_clear() {
    threads.main_thread()->wait_for_search_finished();

    tt.clear(options["Threads"]);
    threads.clear();
    Tablebases::init(options["SyzygyPath"]);  // Free mapped files
}

void UCI::setoption(std::istringstream& is) {
    threads.main_thread()->wait_for_search_finished();
    options.setoption(is);
}

void UCI::position(Position& pos, std::istringstream& is, StateListPtr& states) {
    Move        m;
    std::string token, fen;

    is >> token;
#ifdef FAIRY_STOCKFISH
    // Parse as SFEN if specified
    bool sfen = token == "sfen";

#endif
    if (token == "startpos")
    {
#ifndef FAIRY_STOCKFISH
        fen = StartFEN;
#else
        fen = variants.find(options["UCI_Variant"])->second->startFen;
#endif
        is >> token;  // Consume the "moves" token, if any
    }
#ifndef FAIRY_STOCKFISH
    else if (token == "fen")
#else
    else if (token == "fen" || token == "sfen")
#endif
        while (is >> token && token != "moves")
            fen += token + " ";
    else
        return;

    states = StateListPtr(new std::deque<StateInfo>(1));  // Drop the old state and create a new one
#ifndef FAIRY_STOCKFISH
    pos.set(fen, options["UCI_Chess960"], &states->back());
#else
    pos.set(variants.find(options["UCI_Variant"])->second, fen, options["UCI_Chess960"], &states->back(), sfen);
#endif

    // Parse the move list, if any
    while (is >> token && (m = to_move(pos, token)) != Move::none())
    {
        states->emplace_back();
        pos.do_move(m, states->back());
    }
}

int UCI::to_cp(Value v) { return 100 * v / NormalizeToPawnValue; }

std::string UCI::value(Value v) {
    assert(-VALUE_INFINITE < v && v < VALUE_INFINITE);

    std::stringstream ss;

    if (std::abs(v) < VALUE_TB_WIN_IN_MAX_PLY)
        ss << "cp " << to_cp(v);

    else if (std::abs(v) <= VALUE_TB)
    {
        const int ply = VALUE_TB - std::abs(v);  // recompute ss->ply
        ss << "cp " << (v > 0 ? 20000 - ply : -20000 + ply);
    }
    else
#ifndef FAIRY_STOCKFISH
        ss << "mate " << (v > 0 ? VALUE_MATE - v + 1 : -VALUE_MATE - v) / 2;
#else
        ss << "mate " << (v > 0 ? VALUE_MATE - v + 1 : -VALUE_MATE - v - 1) / 2;
#endif
    return ss.str();
}

#ifdef FAIRY_STOCKFISH
/// UCI::dropped_piece() generates a piece label string from a Move.

std::string UCI::dropped_piece(const Position& pos, Move m) {
  assert(type_of(m) == DROP);
  if (dropped_piece_type(m) == pos.promoted_piece_type(in_hand_piece_type(m)))
      // Dropping as promoted piece
      return std::string{'+', pos.piece_to_char()[in_hand_piece_type(m)]};
  else
      return std::string{pos.piece_to_char()[dropped_piece_type(m)]};
}
#endif

#ifndef FAIRY_STOCKFISH
std::string UCI::square(Square s) {
    return std::string{char('a' + file_of(s)), char('1' + rank_of(s))};
}

#else
std::string UCI::square(const Position& pos, Square s) {
#ifdef LARGEBOARDS
        return rank_of(s) < RANK_10 ? std::string{ char('a' + file_of(s)), char('1' + (rank_of(s) % 10)) }
                                    : std::string{ char('a' + file_of(s)), char('0' + ((rank_of(s) + 1) / 10)),
                                                   char('0' + ((rank_of(s) + 1) % 10)) };
#else
    return std::string{ char('1' + pos.max_file() - file_of(s)), char('a' + pos.max_rank() - rank_of(s)) };
#endif
}
#endif

#ifndef FAIRY_STOCKFISH
std::string UCI::move(Move m, bool chess960) {
#else
std::string UCI::move(const Position& pos, Move m) {
#endif
    if (m == Move::none())
        return "(none)";

    if (m == Move::null())
        return "0000";

    Square from = m.from_sq();
    Square to   = m.to_sq();

#ifndef FAIRY_STOCKFISH
    if (m.type_of() == CASTLING && !chess960)
        to = make_square(to > from ? FILE_G : FILE_C, rank_of(from));
#else
    if (is_gating(m) && gating_square(m) == to)
        from = m.to_sq(), to = m.from_sq();
    else if (m.type_of() == CASTLING && !pos.is_chess960())
    {
        to = make_square(to > from ? pos.castling_kingside_file() : pos.castling_queenside_file(), rank_of(from));
        // If the castling move is ambiguous with a normal king move, switch to 960 notation
        if (pos.pseudo_legal(make_move(from, to)))
            to = m.to_sq();
    }
#endif

#ifndef FAIRY_STOCKFISH
    std::string move = square(from) + square(to);
#else
    std::string move = (m.type_of() == DROP ? UCI::dropped_piece(pos, m) + '@'
                                    : UCI::square(pos, from)) + UCI::square(pos, to);

#endif

    if (m.type_of() == PROMOTION)
#ifndef FAIRY_STOCKFISH
        move += " pnbrqk"[m.promotion_type()];
#else
        move += pos.piece_to_char()[make_piece(BLACK, m.promotion_type( ))];
        else if (m.type_of() == PIECE_PROMOTION)
            move += '+';
        else if (m.type_of() == PIECE_DEMOTION)
            move += '-';
        else if (is_gating(m))
        {
            move += pos.piece_to_char()[make_piece(BLACK, gating_type(m))];
            if (gating_square(m) != from)
                move += UCI::square(pos, gating_square(m));
        }

#endif
    return move;
}

namespace {
// The win rate model returns the probability of winning (in per mille units) given an
// eval and a game ply. It fits the LTC fishtest statistics rather accurately.
int win_rate_model(Value v, int ply) {

    // The fitted model only uses data for moves in [8, 120], and is anchored at move 32.
    double m = std::clamp(ply / 2 + 1, 8, 120) / 32.0;

    // The coefficients of a third-order polynomial fit is based on the fishtest data
    // for two parameters that need to transform eval to the argument of a logistic
    // function.
    constexpr double as[] = {-1.06249702, 7.42016937, 0.89425629, 348.60356174};
    constexpr double bs[] = {-5.33122190, 39.57831533, -90.84473771, 123.40620748};

    // Enforce that NormalizeToPawnValue corresponds to a 50% win rate at move 32.
    static_assert(NormalizeToPawnValue == int(0.5 + as[0] + as[1] + as[2] + as[3]));

    double a = (((as[0] * m + as[1]) * m + as[2]) * m) + as[3];
    double b = (((bs[0] * m + bs[1]) * m + bs[2]) * m) + bs[3];

    // Return the win rate in per mille units, rounded to the nearest integer.
    return int(0.5 + 1000 / (1 + std::exp((a - double(v)) / b)));
}
}

std::string UCI::wdl(Value v, int ply) {
    std::stringstream ss;

    int wdl_w = win_rate_model(v, ply);
    int wdl_l = win_rate_model(-v, ply);
    int wdl_d = 1000 - wdl_w - wdl_l;
    ss << " wdl " << wdl_w << " " << wdl_d << " " << wdl_l;

    return ss.str();
}

Move UCI::to_move(const Position& pos, std::string& str) {
    if (str.length() == 5)
    {
#ifdef FAIRY_STOCKFISH
        if (str[4] == '=')
            // shogi moves refraining from promotion might use equals sign
            str.pop_back();
        else
            // Junior could send promotion piece in uppercase
        str[4] = char(tolower(str[4]));  // The promotion piece character must be lowercased
#endif
    }

    for (const auto& m : MoveList<LEGAL>(pos))
#ifndef FAIRY_STOCKFISH
        if (str == move(m, pos.is_chess960()))
#else
        if (str == UCI::move(pos, m) || (is_pass(m) && str == UCI::square(pos, m.from_sq()) + UCI::square(pos, m.to_sq())))
#endif
            return m;

    return Move::none();
}

}  // namespace Stockfish
