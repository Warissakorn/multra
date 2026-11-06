#pragma once
// M10.4 E1 (MODE_CHOICE §7, D478): a mode-choice run's JSON (full precision: per purpose the alternatives'
// P-A persons and totals; per period each alternative's OD persons and vehicles and each mode's vehicles)
// and CSV (one row per period, alternative and pair, plus a totals row). Both carry the not-validated
// marker and the unit conditional (§5); the same run gives the same bytes.
#include "mode_choice_run.hpp"
#include <string>

namespace multra {
Json modeChoiceJson(const ModeChoiceRun&);
std::string modeChoiceCsv(const ModeChoiceRun&);
}
