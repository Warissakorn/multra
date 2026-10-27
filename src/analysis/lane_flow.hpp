#pragma once
#include <array>

// M13 A2 (docs/reference/NODE_ANALYSIS.md §11, D373): HCM 7 Ch. 31 §2's lane group flow rate on a
// multiple-lane approach with a shared lane (Eqs. 31-46 to 31-66, PDF p. 2949-2960), protected
// operation only, so fLpb = fRpb = 1 and no permitted terms. Pure: values in, values out.
namespace multra {
// Exhibit 31-6's lane groups (PDF p. 2952): exclusive left, shared left+through, exclusive through,
// shared through+right, exclusive right, shared left+right; and `all`, the one lane of a one-lane
// approach serving every movement (Eq. 31-63's form with both shares). HCM's left is the far turn
// and its right the kerb turn.
enum class LaneGroupKind { left, sharedLeft, through, sharedRight, right, leftRight, all };
inline constexpr int kLaneGroupKinds = 7;
struct ApproachDemand { double left{}, through{}, right{}; }; // v_lt, v_th, v_rt in veh/h
struct LaneFlowGroup {
    int lanes{};
    double flow{};             // veh/h for the whole lane group
    double farTurns{}, kerbTurns{}; // its far-turn and kerb-turn flows, veh/h
    double saturationShare{1}; // s / s_th (Eqs. 31-61 to 31-63 and their exclusive forms)
};
struct LaneFlows {
    std::array<LaneFlowGroup, kLaneGroupKinds> groups{}; // indexed by LaneGroupKind
    double laneChange{};                                  // P_lc, Eq. 31-50
    double farEquivalent{1}, kerbEquivalent{1};           // E_L,m and E_R,m, Eqs. 31-47 and 31-53
    bool converged{};
    const LaneFlowGroup& operator[](LaneGroupKind k) const { return groups[static_cast<int>(k)]; }
};
// E_L 1.05 and E_R 1.18 for protected turns (PDF p. 2954, 2955); t_lc 3.7 s (PDF p. 2954).
inline constexpr double kFarTurnEquivalent = 1.05, kKerbTurnEquivalent = 1.18, kLaneChangeHeadway = 3.7;
// Eq. 31-50: 1 - (2 v_app/s_lc - 1)^2, at least 0, with s_lc = 3,600/t_lc.
double laneChangeProbability(double approachFlowPerLane);
// `lanes[k]` is the lane count of each kind. A layout with a shared lane runs Steps B-H until each
// shared lane group's flow moves by less than 0.1 veh/h; without one, each movement keeps its own
// lanes. Lane groups that trade vehicles are the through-serving ones plus the exclusive lanes of a
// turn that also uses a shared lane; a turn without a shared lane keeps all its flow (Eq. 31-55 with
// v_sl,lt = 0). `converged` is false after 100 iterations.
LaneFlows approachLaneFlows(const ApproachDemand&, const std::array<int, kLaneGroupKinds>& lanes);
}
