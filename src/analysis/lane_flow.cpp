#include "lane_flow.hpp"
#include <algorithm>
#include <cmath>

namespace multra {
namespace {
constexpr int at(LaneGroupKind k) { return static_cast<int>(k); }
}
double laneChangeProbability(double approachFlowPerLane) {
    const double ratio = 2 * approachFlowPerLane / (3600 / kLaneChangeHeadway) - 1;
    return std::max(0.0, 1 - ratio * ratio);
}
LaneFlows approachLaneFlows(const ApproachDemand& v, const std::array<int, kLaneGroupKinds>& n) {
    LaneFlows out;
    for (int k = 0; k < kLaneGroupKinds; ++k) out.groups[k].lanes = n[k];
    const int nl = n[at(LaneGroupKind::left)], nsl = n[at(LaneGroupKind::sharedLeft)], nt = n[at(LaneGroupKind::through)];
    const int nsr = n[at(LaneGroupKind::sharedRight)], nr = n[at(LaneGroupKind::right)], nlr = n[at(LaneGroupKind::leftRight)];
    const int nall = n[at(LaneGroupKind::all)];
    // Eq. 31-51, or Eq. 31-52 with a shared left+right lane; the one lane of a one-lane approach.
    const double perLane = nlr > 0 ? (v.left + v.right) / nlr
                         : nall > 0 ? v.left + v.through + v.right
                         : nsl + nt + nsr > 0 ? (v.left + v.through + v.right) / (nsl + nt + nsr) : 0;
    out.laneChange = laneChangeProbability(perLane);
    out.farEquivalent = (kFarTurnEquivalent - 1) * out.laneChange + 1;   // Eq. 31-47
    out.kerbEquivalent = (kKerbTurnEquivalent - 1) * out.laneChange + 1; // Eq. 31-53, fRpb = 1
    const double el = out.farEquivalent, er = out.kerbEquivalent;
    auto& g = out.groups;
    // Shares of the exclusive turn lanes: Eq. 31-63's form with one share 1 and the other 0 (the
    // right one is Eq. 31-61 with P_R = 1, PDF p. 2958).
    g[at(LaneGroupKind::left)].saturationShare = 1 / el;
    g[at(LaneGroupKind::right)].saturationShare = 1 / er;
    if (nall > 0) { // one lane, every movement, nothing to choose
        auto& a = g[at(LaneGroupKind::all)];
        a.flow = v.left + v.through + v.right; a.farTurns = v.left; a.kerbTurns = v.right;
        const double pl = a.flow > 0 ? v.left / a.flow : 0, pr = a.flow > 0 ? v.right / a.flow : 0;
        a.saturationShare = 1 / (1 + pl * (el - 1) + pr * (er - 1));
        out.converged = true;
        return out;
    }
    const bool shared = nsl + nsr + nlr > 0;
    // A turn whose lanes include a shared one trades vehicles with it; one that does not keeps them.
    const bool leftTrades = nsl + nlr > 0, rightTrades = nsr + nlr > 0;
    // Step B: the shared lanes start at the average lane flow and carry no turns.
    double sl = perLane, sr = perLane, lr = perLane, slLeft = 0, srRight = 0, lrLeft = 0, lrRight = 0;
    double lanesLeft = 0, lanesRight = 0, lanesThrough = 0; // flow per exclusive lane
    for (int iteration = 0; iteration < 100; ++iteration) {
        // Step C, Eqs. 31-55 and 31-56; with a left+right lane its turns take the place of v_sl,lt.
        const double sharedLeftTurns = nlr > 0 ? lrLeft : slLeft, sharedRightTurns = nlr > 0 ? lrRight : srRight;
        lanesLeft = nl > 0 ? std::max(0.0, (v.left - (leftTrades ? sharedLeftTurns : 0)) / nl) : 0;
        lanesRight = nr > 0 ? std::max(0.0, (v.right - (rightTrades ? sharedRightTurns : 0)) / nr) : 0;
        lanesThrough = nt > 0 ? std::max(0.0, (v.through - (nsl > 0 ? sl - slLeft : 0) - (nsr > 0 ? sr - srRight : 0)) / nt) : 0;
        // Step D, Eqs. 31-57 and 31-58.
        const double pl = nsl > 0 && sl > 0 ? std::min(1.0, slLeft / sl) : 0;
        const double pr = nsr > 0 && sr > 0 ? std::min(1.0, srRight / sr) : 0;
        const double lrl = nlr > 0 && lr > 0 ? std::min(1.0, lrLeft / lr) : 0, lrr = nlr > 0 && lr > 0 ? std::min(1.0, lrRight / lr) : 0;
        // Step E: Eq. 31-63 with P_R = 0 for a protected sl lane, Eq. 31-61, Eq. 31-62 with f_s = 1, Eq. 31-63.
        g[at(LaneGroupKind::sharedLeft)].saturationShare = 1 / (1 + pl * (el - 1));
        g[at(LaneGroupKind::sharedRight)].saturationShare = 1 / (1 + pr * (er - 1));
        g[at(LaneGroupKind::through)].saturationShare = 1;
        g[at(LaneGroupKind::leftRight)].saturationShare = 1 / (1 + lrl * (el - 1) + lrr * (er - 1));
        if (!shared) { out.converged = true; break; }
        // Step F, Eq. 31-64, over the lane groups that trade.
        const auto s = [&](LaneGroupKind k) { return g[at(k)].saturationShare; };
        // The numerator is the demand those lane groups carry between them (each exclusive turn lane
        // that does not trade keeps its own), so the distribution conserves every movement.
        const double flow = v.through + (leftTrades ? v.left : 0) + (rightTrades ? v.right : 0);
        double capacity = s(LaneGroupKind::sharedLeft) * nsl +
            s(LaneGroupKind::through) * nt + s(LaneGroupKind::sharedRight) * nsr + s(LaneGroupKind::leftRight) * nlr;
        if (leftTrades) capacity += s(LaneGroupKind::left) * nl;
        if (rightTrades) capacity += s(LaneGroupKind::right) * nr;
        const double y = capacity > 0 ? flow / capacity : 0;
        // Step G, Eq. 31-65 and its like; Step H, Eq. 31-66 and its like.
        const double nextSl = s(LaneGroupKind::sharedLeft) * y, nextSr = s(LaneGroupKind::sharedRight) * y;
        const double nextLr = s(LaneGroupKind::leftRight) * y;
        if (leftTrades) lanesLeft = s(LaneGroupKind::left) * y;
        if (rightTrades) lanesRight = s(LaneGroupKind::right) * y;
        const double turnLeft = std::max(0.0, v.left - lanesLeft * nl), turnRight = std::max(0.0, v.right - lanesRight * nr);
        // Settled when every shared lane's flow, and the turns it carries, moved by less than 0.1 veh/h.
        const double lastLeft = nlr > 0 ? lrLeft : slLeft, lastRight = nlr > 0 ? lrRight : srRight;
        const bool settled = std::abs(nextSl - sl) < 0.1 && std::abs(nextSr - sr) < 0.1 && std::abs(nextLr - lr) < 0.1 &&
                             std::abs(turnLeft - lastLeft) < 0.1 && std::abs(turnRight - lastRight) < 0.1;
        sl = nextSl; sr = nextSr; lr = nextLr;
        if (nlr > 0) { lrLeft = turnLeft; lrRight = turnRight; }
        else { slLeft = turnLeft; srRight = turnRight; }
        if (settled) { out.converged = true; break; }
    }
    // The converged shared flows; exclusive lanes take what remains of their movement (Step C).
    const double sharedLeftTurns = nlr > 0 ? lrLeft : slLeft, sharedRightTurns = nlr > 0 ? lrRight : srRight;
    lanesLeft = nl > 0 ? std::max(0.0, (v.left - (leftTrades ? sharedLeftTurns : 0)) / nl) : 0;
    lanesRight = nr > 0 ? std::max(0.0, (v.right - (rightTrades ? sharedRightTurns : 0)) / nr) : 0;
    lanesThrough = nt > 0 ? std::max(0.0, (v.through - (nsl > 0 ? sl - slLeft : 0) - (nsr > 0 ? sr - srRight : 0)) / nt) : 0;
    g[at(LaneGroupKind::left)].flow = lanesLeft * nl; g[at(LaneGroupKind::left)].farTurns = lanesLeft * nl;
    g[at(LaneGroupKind::right)].flow = lanesRight * nr; g[at(LaneGroupKind::right)].kerbTurns = lanesRight * nr;
    g[at(LaneGroupKind::through)].flow = lanesThrough * nt;
    if (nsl > 0) { g[at(LaneGroupKind::sharedLeft)].flow = sl; g[at(LaneGroupKind::sharedLeft)].farTurns = slLeft; }
    if (nsr > 0) { g[at(LaneGroupKind::sharedRight)].flow = sr; g[at(LaneGroupKind::sharedRight)].kerbTurns = srRight; }
    if (nlr > 0) { g[at(LaneGroupKind::leftRight)].flow = lr; g[at(LaneGroupKind::leftRight)].farTurns = lrLeft; g[at(LaneGroupKind::leftRight)].kerbTurns = lrRight; }
    return out;
}
}
