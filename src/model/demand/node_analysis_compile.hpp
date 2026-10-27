#pragma once
#include "definition.hpp"
#include "../network/node_membership.hpp"
#include "../../analysis/lane_flow.hpp"
#include "../../analysis/signalised.hpp"

namespace multra {
// M13 A1/A2 (NODE_ANALYSIS §3-§4, §11, D371, D373): a signalised Node's lane groups, derived from its
// Micro detail and signal programs, ready for analyseSignalised. Pure, and never throws; what cannot
// be analysed is an advisory at "network.nodes[i]" naming the turn, and that turn has no lane group:
// - NODE_ANALYSIS_NEEDS_DETAIL: no turn (no detail, or a membership issue);
// - NODE_ANALYSIS_NOT_SIGNALISED: the Node's control is not signalised;
// - NODE_ANALYSIS_UNSIGNALISED: a turn whose vehicles pass no head (reported, not analysed);
// - NODE_ANALYSIS_SIGNAL: a turn under more than one program, a program with more than one green
//   interval per cycle, or two programs on one Node with different cycles;
// - NODE_ANALYSIS_LANE_LAYOUT: an approach with a shared lane whose lanes are not one of Exhibit 31-6's
//   layouts (PDF p. 2952): at most one lane group of each kind, at most one sl, sr or lr lane, lr
//   without sl, t or sr, and `all` only as an approach's one lane;
// - NODE_ANALYSIS_PERMITTED: a turn that yields in a Node conflict to a turn green at the same time,
//   or both turns when the conflict's priority is undetermined (Ch. 31 §2, A3).
// On an approach with a shared lane (two or more movements in one lane) the lane groups are coupled
// (Ch. 31 §2), so a turn refused for any reason refuses every turn of that approach with its code,
// and two turns of one shared lane under different programs are NODE_ANALYSIS_SIGNAL. An approach
// without one keeps A1's lane groups: one per turn, on that turn's lanes.
// A turn's lanes are those its first Connector leaves on the in-leg Link (from its `from` lane,
// fromLaneCount wide); a path without one, or whose first Connector leaves an internal Link, takes
// every lane of the in-leg Link.
struct AnalysisLaneGroup {
    std::vector<std::size_t> turns;   // indexes into membership.turns
    std::string name, inLink, program; // name: its turns' names joined by " + "
    LaneGroupKind kind{};
    LaneGroupMovement movement{};     // an exclusive lane group's movement; through for a shared one
    bool coupled{};                   // on an approach with a shared lane (flows from approachLaneFlows)
    int lanes{}; double laneWidth{};  // m, the mean of its lanes
    double displayedGreen{}, effectiveGreen{}, cycle{};
    bool operator==(const AnalysisLaneGroup&) const = default;
};
struct NodeAnalysisCompile {
    NodeMembership membership;
    std::vector<LaneGroupMovement> movements; // per membership turn
    std::vector<AnalysisLaneGroup> groups;
    std::vector<ValidationIssue> advisories;
};
// Start-up lost time l1 and extension of effective green e: HCM 7 Ch. 19's 2.0 s defaults (the
// definitions under Eq. 19-1, PDF p. 1415). Effective green g = G + Y + Rc - lt with lt = l1 + Y + Rc
// - e, so g = G - l1 + e.
inline constexpr double kStartUpLostTime = 2.0, kGreenExtension = 2.0;
NodeAnalysisCompile compileNodeAnalysis(const Network&, std::size_t node, const std::vector<SignalController>&,
                                        const std::vector<SignalProgram>&);
}
