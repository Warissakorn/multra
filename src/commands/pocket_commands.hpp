#pragma once
#include "history.hpp"
#include "../model/network/approach_layout.hpp"

namespace multra {
// J2: retain the upstream id; the returned Link has `storage` metres before its current end
// (the intended stop line). The preceding `taper` metres become Connectors, with a widening
// Connector on the selected side. Existing lanes keep their footprints. Run inside one
// History::execute: routes, heads and controls follow the split; Undo restores every byte.
// No recipe/length metadata is stored. The legacy splitLink(bool) keeps its 0.2 m span.
std::string addTurnPocket(ProjectDocument&, const std::string& link, double storage, double taper, PocketSide);
}
