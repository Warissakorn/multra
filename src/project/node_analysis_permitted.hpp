#pragma once
#include "../model/demand/node_analysis_compile.hpp"
#include "../analysis/critical.hpp"
#include "../analysis/permitted.hpp"
#include "../analysis/signalised.hpp"
#include <optional>
#include <string>
#include <vector>

// M13 A3 (docs/reference/NODE_ANALYSIS_A3.md, D376): one permitted or protected-permitted far-turn lane
// group of a Node, from its compiled timing, its A1/A2 input and the opposing queue. Analytical, not
// simulated, not validated.
namespace multra {
struct PermittedLaneGroupInput {
    SignalisedLaneGroup input;  // as A1/A2 build it: flow, lanes, factors, cycle (its green is ignored)
    PermittedTiming timing;
    LaneGroupKind kind{};       // left (exclusive) or sharedLeft
    double farFlow{};           // v_lt in this lane group, veh/h
    double opposingFlow{};      // v_o: the opposing through and kerb flows it yields to, veh/h (Eq. 31-100)
    double opposingFarShare{};  // P_lto on a one-lane opposing approach
    bool opposingOneLane{};     // PDF p. 2994
    bool approachOneLane{};     // Eq. 31-97 rather than 31-98
    double opposingQueue{};     // G_q2, displayed s (Exhibit 31-12 note a)
};
struct PermittedLaneGroupResult {
    std::string operation;      // "permitted" or "protected-permitted"
    PermittedGreen green;       // G_p, G_U, l_1,p, e_p, g_p, g_u
    double protectedGreen{};    // g_l, effective s (0 when permitted only)
    double permittedSaturation{}; // s_p, veh/h/ln (Eq. 31-100)
    double farShare{}, firstBlock{}; // P_L and g_f in a shared lane (0 in an exclusive one)
    double laneSaturation{};    // s_l (Eq. 31-110) or s_sl (Eq. 31-122), veh/h/ln
    double opposingFlow{}, opposingQueue{}; // v_o and G_q2 as used
    SignalisedResult result;    // capacity (Step 7 B), X, d1 (Eq. 31-115), d2 (Eq. 19-26), delay
    std::optional<double> serviceTime; // g_s from the start of its first effective green, s
    std::vector<CriticalInterval> intervals; // its protected and permitted parts for Xc (Ch. 19 §4)
};
PermittedLaneGroupResult analysePermitted(const PermittedLaneGroupInput&);
// A shared lane's s/s_th for the lane-flow procedure: Eq. 31-59 with E_L1,m and E_L2,m (P_lc) and the
// sneaker term of Eq. 31-60, at P_L and v_sl,lt.
double permittedLaneShare(const PermittedLaneGroupInput&, double farShare, double farFlow, double laneChange);
// g_s of a lane group without a permitted far turn: Exhibit 31-11's polygon, Q_r/(s - q) per lane with
// random arrivals; none when its arrivals reach its saturation flow.
std::optional<double> protectedServiceTime(const SignalisedLaneGroup&, const SignalisedResult&);
}
