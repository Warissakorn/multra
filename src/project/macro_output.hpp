#pragma once
#include "macro_run.hpp"

namespace trafficsim {
// S2.3 (MACRO_NETWORK §9, D275): a Macro run's per-run fields and one row per arc, in graph order:
// kind, source, station range, link type, veh/h per class, pcu/h, V/C (capacitated arcs only), free
// and congested time. Both carry the not-yet-validated marker.
Json macroJson(const MacroRun&);
std::string macroCsv(const MacroRun&);
}
