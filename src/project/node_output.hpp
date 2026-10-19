#pragma once
// S0.3 (docs/reference/NODES.md §4, D264): each Node's turns, legs and conflicts as evaluation
// specs, and their rows in single-run and batch JSON/CSV. Written only when a Node exists, so every
// other output keeps its bytes. Simulated section delay, NOT HCM control delay, not validated (M6).
#include "json.hpp"
#include "../eval/movement.hpp"
#include "../model/network/network.hpp"
#include "../runner/batch.hpp"
#include <iosfwd>

namespace trafficsim {
// One spec per Node with a valid polygon, in Node order. A turn's section runs from its in-leg Link's
// start (owner choice 1a) to the out-leg's boundary crossing; a leg's queue lines are on every lane
// of its in-leg at the crossing (choice 2a). A refused Node has its issues and no turn or leg.
std::vector<NodeSpec> nodeSpecs(const Network&, const RuntimeSections&);
void addNodesJson(Json&, const MovementReport&);
void writeNodesCsv(std::ostream&, const MovementReport&);
void addBatchNodesJson(Json&, const BatchReport&);
void writeBatchNodesCsv(std::ostream&, const BatchReport&);
}
