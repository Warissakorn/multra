#pragma once
// D165/D166 (docs/reference/BEHAVIOUR_LAB.md §2): the lab's test network, built through the
// editor's own commands, so the canvas draws it and buildScenario compiles it exactly as it would
// any project's drawing. Never saved: the lab rebuilds it from LabSettings.
#include "lab_settings.hpp"
#include "../project/document.hpp"

namespace trafficsim::lab {
// The document and which authored Routes are which: `mainRoute` carries the tuned vehicle and its
// traffic, `crossRoutes` the Urban circuit's cross-traffic loops. Each compiles to one chain per
// lane, `id` or `id/lane-k`. No inputs: an input on a closed loop has an upstream segment, which a
// Run refuses (UNSUPPORTED_INTERNAL_INPUT), so LabRun compiles the drawing itself and places vehicles.
struct LabNetwork {
    ProjectDocument document;
    std::string mainRoute;
    std::vector<std::string> crossRoutes;
};
LabNetwork buildLabDocument(const LabSettings& settings);
// The tuned vehicle's loop length along the street centrelines, metres: an estimate for limits
// shown in the panel; LabRun checks capacity on the compiled lanes.
double referenceLapLength(const RoadSettings& road);
}
