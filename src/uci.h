/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2022 The Stockfish developers (see AUTHORS file)

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

#ifndef UCI_H_INCLUDED
#define UCI_H_INCLUDED

#include <map>
#include <string>
#ifdef FAIRY_STOCKFISH
#include <vector>
#endif

#include "types.h"

#ifdef FAIRY_STOCKFISH
#include "variant.h"
#endif

namespace Stockfish {

class Position;

namespace UCI {

#ifdef FAIRY_STOCKFISH
#ifndef _WIN32
  constexpr char SepChar = ':';
#else
  constexpr char SepChar = ';';
#endif

void init_variant(const Variant* v);
#endif

// Normalizes the internal value as reported by evaluate or search
// to the UCI centipawn result used in output. This value is derived from
// the win_rate_model() such that Stockfish outputs an advantage of
// "100 centipawns" for a position if the engine has a 50% probability to win
// from this position in selfplay at fishtest LTC time control.
const int NormalizeToPawnValue = 361;

class Option;

/// Define a custom comparator, because the UCI options should be case-insensitive
struct CaseInsensitiveLess {
  bool operator() (const std::string&, const std::string&) const;
};

/// The options container is defined as a std::map
typedef std::map<std::string, Option, CaseInsensitiveLess> OptionsMap;

/// The Option class implements each option as specified by the UCI protocol
class Option {

  typedef void (*OnChange)(const Option&);

public:
  Option(OnChange = nullptr);
  Option(bool v, OnChange = nullptr);
  Option(const char* v, OnChange = nullptr);
#ifdef FAIRY_STOCKFISH
  Option(const char* v, const std::vector<std::string>& variants, OnChange = nullptr);
#endif
  Option(double v, int minv, int maxv, OnChange = nullptr);
#ifndef FAIRY_STOCKFISH
  Option(const char* v, const char* cur, OnChange = nullptr);
#endif

  Option& operator=(const std::string&);
  void operator<<(const Option&);
  operator double() const;
  operator std::string() const;
  bool operator==(const char*) const;
#ifdef FAIRY_STOCKFISH
  bool operator!=(const char*) const;
  void set_combo(std::vector<std::string> newComboValues);
  void set_default(std::string newDefault);
  const std::string get_type() const;
#endif

private:
  friend std::ostream& operator<<(std::ostream&, const OptionsMap&);

  std::string defaultValue, currentValue, type;
  int min, max;
#ifdef FAIRY_STOCKFISH
  std::vector<std::string> comboValues;
#endif
  size_t idx;
  OnChange on_change;
};

void init(OptionsMap&);
void loop(int argc, char* argv[]);
std::string value(Value v);
#ifndef FAIRY_STOCKFISH
std::string square(Square s);
#else
std::string square(const Position& pos, Square s);
std::string dropped_piece(const Position& pos, Move m);
#endif
#ifndef FAIRY_STOCKFISH
std::string move(Move m, bool chess960);
#else
std::string move(const Position& pos, Move m);
#endif
std::string pv(const Position& pos, Depth depth);
std::string wdl(Value v, int ply);
Move to_move(const Position& pos, std::string& str);

#ifdef FAIRY_STOCKFISH
std::string option_name(std::string name);
bool is_valid_option(UCI::OptionsMap& options, std::string& name);
#endif

} // namespace UCI

extern UCI::OptionsMap Options;

#ifdef FAIRY_STOCKFISH
enum Protocol {
  UCI_GENERAL,
  USI,
  UCCI,
  UCI_CYCLONE,
  XBOARD,
};

constexpr bool is_uci_dialect(Protocol p) {
  return p != XBOARD;
}

extern Protocol CurrentProtocol;
#endif

} // namespace Stockfish

#endif // #ifndef UCI_H_INCLUDED
