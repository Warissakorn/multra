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
// - NODE_ANALYSIS_PERMITTED: a turn that yields in a Node conflict to a turn green at the same time
//   (both turns when the conflict's priority is undetermined), unless it is a far turn yielding only to
//   the through and kerb turns of one other in-leg (analysed, A3) or a kerb turn (A3, below);
// - NODE_ANALYSIS_PERMITTED_LAYOUT: such a far turn on an approach whose layout A3 does not analyse
//   permitted (only an exclusive far-turn lane group, or one shared far+through lane without an
//   exclusive far-turn lane: NODE_ANALYSIS_A3 §1);
// - NODE_ANALYSIS_KERB_YIELD (advisory, the turn is analysed with protected values): a kerb turn that
//   yields; HCM's kerb turn yields only to pedestrians (NODE_ANALYSIS_A3 §1).
// A turn may have two green runs per cycle only as a protected-permitted far turn.
// On an approach with a shared lane (two or more movements in one lane) the lane groups are coupled
// (Ch. 31 §2), so a turn refused for any reason refuses every turn of that approach with its code,
// and two turns of one shared lane under different programs are NODE_ANALYSIS_SIGNAL. An approach
// without one keeps A1's lane groups: one per turn, on that turn's lanes.
// A turn's lanes are those its first Connector leaves on the in-leg Link (from its `from` lane,
// fromLaneCount wide); a path without one, or whose first Connector leaves an internal Link, takes
// every lane of the in-leg Link.
// A3 (NODE_ANALYSIS_A3 §1-§2): a far turn's permitted window inside the opposing through's green, in
// displayed seconds. Times after the opposing green's start unless named as cycle times.
struct PermittedTiming {
    std::vector<std::size_t> opposing;  // the turns it yields to: the opposing in-leg's through and kerb turns
    std::string opposingLink;
    double opposingStart{}, opposingGreen{}; // the opposing through's displayed green, cycle time and length
    double windowStart{}, window{};     // S_a - S_o and G_p's window, s
    bool opensDisplay{}, endsDisplay{}; // the window opens or ends the subject's display
    double protectedGreen{}, protectedStart{}; // displayed protected part (0: permitted only), its cycle time
    bool leading{}, protectedOpens{}, protectedEnds{};
    double gap{};                       // red between a leading protected part and the window, s
    bool operator==(const PermittedTiming&) const = default;
};
struct AnalysisLaneGroup {
    std::vector<std::size_t> turns;   // indexes into membership.turns
    std::string name, inLink, program; // name: its turns' names joined by " + "
    LaneGroupKind kind{};
    LaneGroupMovement movement{};     // an exclusive lane group's movement; through for a shared one
    bool coupled{};                   // on an approach with a shared lane (flows from approachLaneFlows)
    int lanes{}; double laneWidth{};  // m, the mean of its lanes
    double displayedGreen{}, effectiveGreen{}, cycle{};
    double greenStart{};              // the displayed green's start in the cycle, s (critical v/c)
    double storage{};                 // m: the in-leg Link from its start to where its turns leave (queues)
    std::optional<PermittedTiming> permitted; // its far turn's, when permitted or protected-permitted (A3)
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
