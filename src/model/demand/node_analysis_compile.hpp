#pragma once
#include "definition.hpp"
#include "../network/node_membership.hpp"
#include "../../analysis/signalised.hpp"

namespace multra {
// M13 A1 (NODE_ANALYSIS §3-§4, D371): a signalised Node's lane groups, derived from its Micro detail
// and signal programs, ready for analyseSignalised. Pure, and never throws; what A1 cannot analyse
// is an advisory at "network.nodes[i]" naming the turn, and that turn has no lane group:
// - NODE_ANALYSIS_NEEDS_DETAIL: no turn (no detail, or a membership issue);
// - NODE_ANALYSIS_NOT_SIGNALISED: the Node's control is not signalised;
// - NODE_ANALYSIS_UNSIGNALISED: a turn whose vehicles pass no head (reported, not analysed);
// - NODE_ANALYSIS_SIGNAL: a turn under more than one program, a program with more than one green
//   interval per cycle, or two programs on one Node with different cycles;
// - NODE_ANALYSIS_SHARED_LANE: a turn sharing an in-leg lane with another turn (Ch. 31 §2, later);
// - NODE_ANALYSIS_PERMITTED: a turn that yields in a Node conflict to a turn green at the same time,
//   or both turns when the conflict's priority is undetermined (Ch. 31 §2, later).
// A turn's lanes are those its first Connector leaves on the in-leg Link (from its `from` lane,
// fromLaneCount wide); a path without one, or whose first Connector leaves an internal Link, takes
// every lane of the in-leg Link.
struct AnalysisLaneGroup {
    std::size_t turn{};               // index into membership.turns
    std::string name, inLink, outLink, program;
    LaneGroupMovement movement{};
    int lanes{}; double laneWidth{};  // m, the mean of its lanes
    double displayedGreen{}, effectiveGreen{}, cycle{};
    bool operator==(const AnalysisLaneGroup&) const = default;
};
struct NodeAnalysisCompile {
    NodeMembership membership;
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
