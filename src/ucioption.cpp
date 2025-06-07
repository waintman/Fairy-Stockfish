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

#include "ucioption.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <iostream>
#include <sstream>
#ifdef FAIRY_STOCKFISH
#include <iostream>
#endif
#include <utility>

#include "misc.h"
#ifdef FAIRY_STOCKFISH
#include "piece.h"
#include "variant.h"
#endif

using std::string;

namespace Stockfish {

#ifdef FAIRY_STOCKFISH

namespace PSQT {
  void init(const Variant* v);
}

// standard variants of XBoard/WinBoard
std::set<std::string> standard_variants = {
    "normal", "nocastle", "fischerandom", "knightmate", "3check", "makruk", "shatranj",
    "asean", "seirawan", "crazyhouse", "bughouse", "suicide", "giveaway", "losers", "atomic",
    "capablanca", "gothic", "janus", "caparandom", "grand", "shogi", "xiangqi", "duck",
    "berolina", "spartan"
};

/// 'On change' actions, triggered by an option's value change
static void on_clear_hash(const Option&) { Search::clear(); }
static void on_hash_size(const Option& o) { TT.resize(size_t(o)); }
static void on_logger(const Option& o) { start_logger(o); }
static void on_threads(const Option& o) { Threads.set(size_t(o)); }
static void on_tb_path(const Option& o) { Tablebases::init(o); }
static void on_use_NNUE(const Option&) { Eval::NNUE::init(); }
static void on_eval_file(const Option&) { Eval::NNUE::init(); }

void on_variant_path(const Option& o) {
    std::stringstream ss((std::string)o);
    std::string path;

    while (std::getline(ss, path, SepChar))
        variants.parse<false>(path);

    Options["UCI_Variant"].set_combo(variants.get_keys());
}
void on_variant_set(const Option &o) {
    // Re-initialize NNUE
    Eval::NNUE::init();

    const Variant* v = variants.find(o)->second;
    init_variant(v);
    PSQT::init(v);
}
void on_variant_change(const Option &o) {
    // Variant initialization
    on_variant_set(o);

    const Variant* v = variants.find(o)->second;
    // Do not send setup command for known variants
    if (standard_variants.find(o) != standard_variants.end())
        return;
    int pocketsize = v->pieceDrops ? (v->pocketSize ? v->pocketSize : popcount(v->pieceTypes)) : 0;
    if (CurrentProtocol == XBOARD)
    {
        // Overwrite setup command for Janggi variants
        auto itJanggi = variants.find("janggi");
        if (   itJanggi != variants.end()
            && v->variantTemplate == itJanggi->second->variantTemplate
            && v->startFen == itJanggi->second->startFen
            && v->pieceToCharTable == itJanggi->second->pieceToCharTable)
        {
            sync_cout << "setup (PH.R.AE..K.C.ph.r.ae..k.c.) 9x10+0_janggi "
                      << "rhea1aehr/4k4/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/4K4/RHEA1AEHR w - - 0 1"
                      << sync_endl;
            return;
        }
        // Send setup command
        sync_cout << "setup (" << v->pieceToCharTable << ") "
                  << v->maxFile + 1 << "x" << v->maxRank + 1
                  << "+" << pocketsize << "_" << v->variantTemplate
                  << " " << v->startFen
                  << sync_endl;
        // Send piece command with Betza notation
        // https://www.gnu.org/software/xboard/Betza.html
        for (PieceSet ps = v->pieceTypes; ps;)
        {
            PieceType pt = pop_lsb(ps);
            string suffix =   pt == PAWN && v->doubleStep     ? "ifmnD"
                            : pt == KING && v->cambodianMoves ? "ismN"
                            : pt == FERS && v->cambodianMoves ? "ifD"
                                                              : "";
            // Janggi palace moves
            if (v->diagonalLines)
            {
                PieceType pt2 = pt == KING ? v->kingType : pt;
                if (pt2 == WAZIR)
                    suffix += "F";
                else if (pt2 == SOLDIER)
                    suffix += "fF";
                else if (pt2 == ROOK)
                    suffix += "B";
                else if (pt2 == JANGGI_CANNON)
                    suffix += "pB";
            }
            // Castling
            if (pt == KING && v->castling)
                 suffix += "O" + std::to_string((v->castlingKingsideFile - v->castlingQueensideFile) / 2);
            // Drop region
            if (v->pieceDrops)
            {
                if (pt == PAWN && !v->firstRankPawnDrops)
                    suffix += "j";
                else if (pt == v->dropNoDoubled)
                    suffix += std::string(v->dropNoDoubledCount, 'f');
                else if (pt == BISHOP && v->dropOppositeColoredBishop)
                    suffix += "s";
                suffix += "@" + std::to_string(pt == PAWN && !v->promotionZonePawnDrops && v->promotionRegion[WHITE] ? rank_of(lsb(v->promotionRegion[WHITE])) : v->maxRank + 1);
            }
            sync_cout << "piece " << v->pieceToChar[pt] << "& " << pieceMap.find(pt == KING ? v->kingType : pt)->second->betza << suffix << sync_endl;
            PieceType promType = v->promotedPieceType[pt];
            if (promType)
                sync_cout << "piece +" << v->pieceToChar[pt] << "& " << pieceMap.find(promType)->second->betza << sync_endl;
        }
    }
    else
        sync_cout << "info string variant " << (std::string)o
                << " files " << v->maxFile + 1
                << " ranks " << v->maxRank + 1
                << " pocket " << pocketsize
                << " template " << v->variantTemplate
                << " startpos " << v->startFen
                << sync_endl;
}

#endif

bool CaseInsensitiveLess::operator()(const std::string& s1, const std::string& s2) const {

    return std::lexicographical_compare(
      s1.begin(), s1.end(), s2.begin(), s2.end(),
      [](char c1, char c2) { return std::tolower(c1) < std::tolower(c2); });
}

void OptionsMap::setoption(std::istringstream& is) {
    std::string token, name, value;

    is >> token;  // Consume the "name" token

#ifdef FAIRY_STOCKFISH
    if (CurrentProtocol == UCCI)
        name = token;
    else
#endif
    // Read the option name (can contain spaces)
    while (is >> token && token != "value")
        name += (name.empty() ? "" : " ") + token;

    // Read the option value (can contain spaces)
    while (is >> token)
        value += (value.empty() ? "" : " ") + token;

    if (options_map.count(name))
#ifdef FAIRY_STOCKFISH
        options_map[name] = value;
    // Deal with option name aliases in UCI dialects
    else if (is_valid_option(options_map, name))
#endif
        options_map[name] = value;
    else
        sync_cout << "No such option: " << name << sync_endl;
}

Option OptionsMap::operator[](const std::string& name) const {
    auto it = options_map.find(name);
    return it != options_map.end() ? it->second : Option();
}

Option& OptionsMap::operator[](const std::string& name) { return options_map[name]; }

std::size_t OptionsMap::count(const std::string& name) const { return options_map.count(name); }

Option::Option(const char* v, OnChange f) :
    type("string"),
    min(0),
    max(0),
    on_change(std::move(f)) {
    defaultValue = currentValue = v;
}
#ifdef FAIRY_STOCKFISH
Option::Option(const char* v, const std::vector<std::string>& values, OnChange f) : type("combo"), min(0), max(0), comboValues(values), on_change(std::move(f))

{ defaultValue = currentValue = v; }
#endif

Option::Option(bool v, OnChange f) :
    type("check"),
    min(0),
    max(0),
    on_change(std::move(f)) {
    defaultValue = currentValue = (v ? "true" : "false");
}

Option::Option(OnChange f) :
    type("button"),
    min(0),
    max(0),
    on_change(std::move(f)) {}

Option::Option(double v, int minv, int maxv, OnChange f) :
    type("spin"),
    min(minv),
    max(maxv),
    on_change(std::move(f)) {
    defaultValue = currentValue = std::to_string(v);
}

Option::Option(const char* v, const char* cur, OnChange f) :
    type("combo"),
    min(0),
    max(0),
    on_change(std::move(f)) {
    defaultValue = v;
    currentValue = cur;
}

Option::operator int() const {
    assert(type == "check" || type == "spin");
    return (type == "spin" ? std::stoi(currentValue) : currentValue == "true");
}

Option::operator std::string() const {
#ifndef FAIRY_STOCKFISH
    assert(type == "string");
#else
    assert(type == "string" || type == "combo");
#endif
    return currentValue;
}

bool Option::operator==(const char* s) const {
    assert(type == "combo");
    return !CaseInsensitiveLess()(currentValue, s) && !CaseInsensitiveLess()(s, currentValue);
}
#ifdef FAIRY_STOCKFISH

bool Option::operator!=(const char* s) const {
  assert(type == "combo");
  return !(*this == s);
}
#endif


// Inits options and assigns idx in the correct printing order

void Option::operator<<(const Option& o) {

    static size_t insert_order = 0;

    *this = o;
    idx   = insert_order++;
}


// Updates currentValue and triggers on_change() action. It's up to
// the GUI to check for option's limits, but we could receive the new value
// from the user by console window, so let's check the bounds anyway.
Option& Option::operator=(const std::string& v) {

    assert(!type.empty());

    if ((type != "button" && type != "string" && v.empty())
        || (type == "check" && v != "true" && v != "false")
#ifdef FAIRY_STOCKFISH
        || (type == "combo" && (std::find(comboValues.begin(), comboValues.end(), v) == comboValues.end()))
#endif
        || (type == "spin" && (std::stof(v) < min || std::stof(v) > max)))
        return *this;

    if (type == "combo")
    {
        OptionsMap         comboMap;  // To have case insensitive compare
#ifndef FAIRY_STOCKFISH
        std::string        token;
        std::istringstream ss(defaultValue);
        while (ss >> token)
#else
        for (std::string token : comboValues)
#endif
            comboMap[token] << Option();
        if (!comboMap.count(v) || v == "var")
            return *this;
    }

    if (type != "button")
        currentValue = v;

    if (on_change)
        on_change(*this);

    return *this;
}
#ifdef FAIRY_STOCKFISH

void Option::set_combo(std::vector<std::string> newComboValues) {
    comboValues = newComboValues;
}

void Option::set_default(std::string newDefault) {
    defaultValue = currentValue = newDefault;

    // When changing the variant default, suppress variant definition output,
    // but still do the essential re-initialization of the variant
    if (on_change)
        (on_change == on_variant_change ? on_variant_set : on_change)(*this);
}

const std::string Option::get_type() const {
    return type;
}
#endif

std::ostream& operator<<(std::ostream& os, const OptionsMap& om) {
#ifdef FAIRY_STOCKFISH

  if (CurrentProtocol == XBOARD)
  {
      for (size_t idx = 0; idx < om.size(); ++idx)
          for (const auto& it : om)
              if (it.second.idx == idx && it.first != "UCI_Variant" && it.first != "Threads" && it.first != "Hash")
              {
                  const Option& o = it.second;
                  os << "\nfeature option=\"" << it.first << " -" << o.type;

                  if (o.type == "string" || o.type == "combo")
                      os << " " << o.defaultValue;
                  else if (o.type == "check")
                      os << " " << int(o.defaultValue == "true");

                  if (o.type == "combo")
                      for (string value : o.comboValues)
                          if (value != o.defaultValue)
                              os << " /// " << value;

                  if (o.type == "spin")
                      os << " " << int(stof(o.defaultValue))
                         << " " << o.min
                         << " " << o.max;

                  os << "\"";

                  break;
              }
  }
  else
#endif
    for (size_t idx = 0; idx < om.options_map.size(); ++idx)
        for (const auto& it : om.options_map)
            if (it.second.idx == idx)
            {
                const Option& o = it.second;
#ifdef FAIRY_STOCKFISH
              // UCI dialects do not allow spaces
              if (CurrentProtocol == UCCI || CurrentProtocol == USI)
              {
                  string name = option_name(it.first);
                  // UCCI skips "name"
                  os << "\noption " << (CurrentProtocol == UCCI ? "" : "name ") << name << " type " << o.type;
              }
              else
#endif
                os << "\noption name " << it.first << " type " << o.type;

                if (o.type == "string" || o.type == "check" || o.type == "combo")
                    os << " default " << o.defaultValue;

#ifdef FAIRY_STOCKFISH
                if (o.type == "combo")
                    for (string value : o.comboValues)
                        os << " var " << value;

#endif
                if (o.type == "spin")
                    os << " default " << int(stof(o.defaultValue)) << " min " << o.min << " max "
                       << o.max;

                break;
            }

    return os;
}
}
