#include "node_analysis_permitted.hpp"
#include "../analysis/qap.hpp"
#include <algorithm>
#include <cmath>

namespace multra {
namespace {
// Eq. 19-8 before its turn and shared factors: s_th for a shared lane (fLU 1), and the protected far
// turn's s_lt (with fLU and fLT) for an exclusive one.
double saturationOf(const SignalisedLaneGroup& in) {
    const auto f = saturationFactors(in);
    return in.baseSaturation * f.fw * f.fHVg * f.fp * f.fbb * f.fa * f.fLU * f.fLT * f.fRT * f.fShared;
}
double throughSaturation(SignalisedLaneGroup in) { in.sharedShare = 1.0; return saturationOf(in); }
struct Part { double start{}, length{}, service{}, sneakers{}; };
// The cycle as polygon periods from the opposing green's start, gaps served at 0 (Exhibits 31-13 to 31-18).
std::vector<QapPeriod> cycleOf(const std::vector<Part>& given, double cycle, double arrival, std::size_t& serviceFrom) {
    std::vector<Part> parts;
    for (auto p : given) { // a part running past the cycle's end continues at its start
        if (!(p.length > 0)) continue;
        p.start = std::fmod(std::fmod(p.start, cycle) + cycle, cycle);
        if (p.start + p.length > cycle + 1e-9) {
            parts.push_back({0, p.start + p.length - cycle, p.service, p.sneakers});
            p.length = cycle - p.start; p.sneakers = 0;
        }
        parts.push_back(p);
    }
    std::sort(parts.begin(), parts.end(), [](const Part& a, const Part& b) { return a.start < b.start; });
    std::vector<QapPeriod> out;
    std::optional<std::size_t> first; // the first served period after an unserved one
    double at = 0;
    for (const auto& p : parts) {
        const double start = std::max(at, p.start), length = p.length - (start - p.start);
        if (!(length > 0)) continue;
        if (start > at + 1e-9) out.push_back({start - at, arrival, 0, 0});
        if (!first && p.service > 0 && (out.empty() || out.back().service == 0)) first = out.size();
        out.push_back({length, arrival, p.service / 3600, p.sneakers});
        at = start + length;
    }
    if (cycle > at + 1e-9) out.push_back({cycle - at, arrival, 0, 0});
    serviceFrom = first.value_or(0);
    return out;
}
}
PermittedLaneGroupResult analysePermitted(const PermittedLaneGroupInput& x) {
    PermittedLaneGroupResult r;
    const auto& t = x.timing;
    const auto& in = x.input;
    const double cycle = in.cycle, lanes = in.lanes;
    constexpr double kStart = 2.0, kExtend = 2.0; // l_1 and e (Ch. 19, PDF p. 1415)
    r.green = permittedGreen(t.windowStart, t.windowStart + t.window, 0, x.opposingQueue, t.opensDisplay, t.gap, t.endsDisplay);
    r.protectedGreen = t.protectedGreen > 0 ? std::max(0.0, t.protectedGreen - (t.protectedOpens ? kStart : 0) + (t.protectedEnds ? kExtend : 0)) : 0;
    r.operation = t.protectedGreen > 0 ? "protected-permitted" : "permitted";
    r.opposingFlow = x.opposingFlow; r.opposingQueue = x.opposingQueue;
    r.permittedSaturation = permittedSaturation(x.opposingFlow);
    const double so = in.baseSaturation, el1 = so / r.permittedSaturation; // Eq. 31-101
    const double gp = r.green.effective, gu = r.green.unblocked;
    const double permitStart = t.windowStart + r.green.startupLost;
    const double guardStart = std::fmod(t.protectedStart - t.opposingStart + cycle, cycle) + (t.protectedOpens ? kStart : 0);
    std::vector<Part> parts;
    double protectedSaturation = 0, permittedCapacity = 0, averagePermitted = 0;
    if (x.kind == LaneGroupKind::left) {
        const double slt = saturationOf(in);                          // Eq. 31-112 (A1's protected value)
        r.laneSaturation = slt * kProtectedFarEquivalent * r.permittedSaturation / so; // Eq. 31-110: s_p for s_o fLT
        const double el2 = queuedEquivalent(x.opposingFarShare, opposingArrivals(gp, gu, 0));
        const double first = x.opposingOneLane ? r.laneSaturation / el2 : 0; // Eq. 31-111, else 0
        parts.push_back({permitStart, std::max(0.0, gp - gu), first, 0});
        parts.push_back({permitStart + std::max(0.0, gp - gu), gu, r.laneSaturation, kSneakersPerCycle});
        protectedSaturation = slt;
        r.result.capacity = exclusivePermittedCapacity(gu, r.laneSaturation, cycle, in.lanes, r.protectedGreen, slt); // Eqs. 31-119, 31-124
        permittedCapacity = exclusivePermittedCapacity(gu, r.laneSaturation, cycle, in.lanes);
        // Eq. 31-59 with P_L 1, g_diff = g_f = 0 and s_th as s_lt (PDF p. 2958), E_L1 for Xc (Ch. 19 §4).
        averagePermitted = gp > 0 ? slt / gp * (std::min(gp, gu) / el1 + 3600 * kSneakersPerCycle / slt) : 0;
    } else {
        const double sth = throughSaturation(in);
        r.farShare = in.flowRate > 0 ? std::min(1.0, x.farFlow / in.flowRate) : 0;
        const double pl = r.farShare;
        r.firstBlock = firstBlockTime(r.green.displayed, gp, r.green.startupLost, pl, x.farFlow, cycle, x.approachOneLane);
        const auto periods = sharedPeriods(gp, gu, r.firstBlock);
        const double el2 = queuedEquivalent(x.opposingFarShare, opposingArrivals(gp, gu, r.firstBlock));
        r.laneSaturation = sth * sharedPermittedShare(periods, gp, pl, el2, el1, x.opposingOneLane, 0); // Eq. 31-122
        protectedSaturation = sth / (1 + pl * (kProtectedFarEquivalent - 1));                          // Eq. 31-113
        parts.push_back({permitStart, periods.first, sth, 0});
        parts.push_back({permitStart + periods.first, periods.second, x.opposingOneLane ? sth / (1 + pl * (el2 - 1)) : 0, 0}); // Eq. 31-108
        parts.push_back({permitStart + periods.first + periods.second, periods.third, sth / (1 + pl * (el1 - 1)), 1 + pl});      // Eq. 31-109
        r.result.capacity = sharedPermittedCapacity(gp, r.laneSaturation, pl, cycle, r.protectedGreen, protectedSaturation); // Eqs. 31-121, 31-126
        permittedCapacity = sharedPermittedCapacity(gp, r.laneSaturation, pl, cycle);
        averagePermitted = sth * sharedPermittedShare(periods, gp, pl, el2, el1, x.opposingOneLane, sharedSneakers(pl) / sth);
    }
    if (r.protectedGreen > 0) parts.push_back({guardStart, r.protectedGreen, protectedSaturation, 0});
    // Per lane, with random arrivals; a lane group over capacity is drawn at capacity (PDF p. 3007).
    const double v = in.flowRate, arrival = std::min(v, r.result.capacity) / (lanes * 3600);
    std::size_t from = 0;
    const auto periods = cycleOf(parts, cycle, arrival, from);
    const auto qap = queueAccumulation(periods, from);
    r.serviceTime = qap.serviceTime;
    r.result.factors = saturationFactors(in);
    r.result.saturation = r.laneSaturation;
    r.result.greenRatio = cycle > 0 ? (gp + r.protectedGreen) / cycle : 0;
    r.result.ratio = r.result.capacity > 0 ? v / r.result.capacity : 0;
    r.result.progression = 1;
    r.result.uniformDelay = qap.uniformDelay;                                                       // Eq. 31-115
    r.result.incrementalDelay = incrementalDelay(in.period, r.result.ratio, in.incrementalFactor, in.upstreamFiltering, r.result.capacity); // Eq. 19-26
    r.result.delay = r.result.uniformDelay + r.result.incrementalDelay;
    // Xc (Ch. 19 §4): the part displayed first carries min(v, its capacity), the other the rest.
    const double protectedCapacity = cycle > 0 ? r.protectedGreen * protectedSaturation * lanes / cycle : 0;
    const bool guardFirst = r.protectedGreen > 0 && t.leading;
    const double firstFlow = std::min(v, guardFirst ? protectedCapacity : permittedCapacity), secondFlow = std::max(0.0, v - firstFlow);
    const double guardFlow = r.protectedGreen > 0 ? (guardFirst ? firstFlow : secondFlow) : 0, permitFlow = v - guardFlow;
    const double windowAt = std::fmod(t.opposingStart + t.windowStart, cycle);
    r.intervals.push_back({windowAt, t.window, gp, averagePermitted > 0 ? permitFlow / (lanes * averagePermitted) : 0});
    if (r.protectedGreen > 0)
        r.intervals.push_back({t.protectedStart, t.protectedGreen, r.protectedGreen, protectedSaturation > 0 ? guardFlow / (lanes * protectedSaturation) : 0});
    return r;
}
double permittedLaneShare(const PermittedLaneGroupInput& x, double farShare, double farFlow, double laneChange) {
    const auto& t = x.timing;
    const auto g = permittedGreen(t.windowStart, t.windowStart + t.window, 0, x.opposingQueue, t.opensDisplay, t.gap, t.endsDisplay);
    const double sth = throughSaturation(x.input), el1 = x.input.baseSaturation / permittedSaturation(x.opposingFlow);
    const double gf = firstBlockTime(g.displayed, g.effective, g.startupLost, farShare, farFlow, x.input.cycle, x.approachOneLane);
    const auto periods = sharedPeriods(g.effective, g.unblocked, gf);
    const double el2 = queuedEquivalent(x.opposingFarShare, opposingArrivals(g.effective, g.unblocked, gf));
    const double share = sharedPermittedShare(periods, g.effective, farShare, modifiedEquivalent(el2, laneChange),
                                              modifiedEquivalent(el1, laneChange), x.opposingOneLane, sharedSneakers(farShare) / sth); // Eq. 31-59
    return std::clamp(share, 1e-6, 1.0);
}
std::optional<double> protectedServiceTime(const SignalisedLaneGroup& in, const SignalisedResult& result) {
    const double q = in.flowRate / (in.lanes * 3600), s = result.saturation / 3600, g = in.effectiveGreen, red = in.cycle - g;
    if (!(s > q) || !(g > 0)) return std::nullopt;
    const double service = q * red / (s - q);
    if (service > g) return std::nullopt;
    return service;
}
}
