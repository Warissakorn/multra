#pragma once
// PRIORITY_RULES slice 2 (D604, D606, D616): the Stop line tool's marker gestures. Each is one
// document change the caller runs inside History::execute, so each is one Undo step.
#include "../project/document.hpp"

namespace multra {
// Vissim's defaults for a new marker (D606 Q5 (b)): 3.0 s, 5.0 m, no speed limit, every type.
inline constexpr double kMarkerGapTime = 3.0, kMarkerHeadway = 5.0;
// Adds a marker at `at` to the line's priority rule, starting the rule when the line has none.
// EDIT_UNKNOWN_OBJECT for an unknown line; PRIORITY_MARKER_ON_OWN_PATH on the line's own path.
// Returns the rule's id.
std::string addPriorityMarker(ProjectDocument&, const std::string& lineId, const ControlPoint& at);
// Moves marker `k` of the rule along its own path.
void movePriorityMarker(ProjectDocument&, const std::string& ruleId, std::size_t k, double station);
// Removes marker `k`; the rule goes with its last marker, and a Stop with no area with the rule.
void removePriorityMarker(ProjectDocument&, const std::string& ruleId, std::size_t k);
}
