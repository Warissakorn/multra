#include "signalised.hpp"
#include <algorithm>
#include <cmath>

namespace multra {
namespace {
constexpr double kFoot = 0.3048; // m, exact
}
std::optional<std::string> signalisedRangeIssue(const SignalisedLaneGroup& g) {
    if (g.lanes < 1) return "ANALYSIS_LANES";
    if (g.laneWidth < 8.0 * kFoot) return "ANALYSIS_LANE_WIDTH";
    if (g.heavyPercent < 0 || g.heavyPercent > 50) return "ANALYSIS_HEAVY_VEHICLES";
    if (g.gradePercent < -4 || g.gradePercent > 10) return "ANALYSIS_GRADE";
    if (g.parkingManeuvers && (*g.parkingManeuvers < 0 || *g.parkingManeuvers > 180)) return "ANALYSIS_PARKING";
    if (g.busesStopping < 0 || g.busesStopping > 250) return "ANALYSIS_BUSES";
    if (!(g.cycle > 0) || !(g.effectiveGreen > 0) || g.effectiveGreen > g.cycle) return "ANALYSIS_TIMING";
    if (g.flowRate < 0 || !(g.baseSaturation > 0) || !(g.period > 0)) return "ANALYSIS_FLOW";
    return std::nullopt;
}
double laneUtilization(LaneGroupMovement movement, int lanes) {
    if (lanes <= 1) return 1.0;
    switch (movement) {
    case LaneGroupMovement::through: return lanes == 2 ? 0.952 : 0.908;
    case LaneGroupMovement::farTurn: return 0.971;
    case LaneGroupMovement::kerbTurn: return 0.885;
    }
    return 1.0;
}
SaturationFactors saturationFactors(const SignalisedLaneGroup& g) {
    SaturationFactors f;
    // Exhibit 19-20 (PDF p. 1466): < 10.0 ft 0.96, 10.0-12.9 ft 1.00, > 12.9 ft 1.04.
    const double feet = g.laneWidth / kFoot;
    f.fw = feet < 10.0 ? 0.96 : feet > 12.9 ? 1.04 : 1.00;
    // Eq. 19-9 for a downhill grade, Eq. 19-10 otherwise (PDF p. 1467).
    f.fHVg = g.gradePercent < 0 ? (100 - 0.79 * g.heavyPercent - 2.07 * g.gradePercent) / 100
                                : (100 - 0.78 * g.heavyPercent - 0.31 * g.gradePercent * g.gradePercent) / 100;
    const double n = g.lanes;
    // Eq. 19-11 (PDF p. 1468): 1.00 without parking.
    if (g.parkingManeuvers) f.fp = std::max(0.050, (n - 0.1 - 18 * *g.parkingManeuvers / 3600) / n);
    // Eq. 19-12 (PDF p. 1469).
    f.fbb = std::max(0.050, (n - 14.4 * g.busesStopping / 3600) / n);
    f.fa = g.cbd ? 0.90 : 1.00; // PDF p. 1469
    f.fLU = laneUtilization(g.movement, g.lanes);
    // Eqs. 19-13 and 19-14 (PDF p. 1470): E_R = 1.18 and E_L = 1.05 for protected turns.
    if (g.movement == LaneGroupMovement::kerbTurn) f.fRT = 1 / 1.18;
    if (g.movement == LaneGroupMovement::farTurn) f.fLT = 1 / 1.05;
    return f;
}
double progressionFactor(double p, double gc, double x) {
    const double y = std::min(1.0, x) * gc; // Eq. 19-21
    return (1 - p) / (1 - gc) * (1 - y) / (1 - std::min(1.0, x) * p) * (1 + y * (1 - p / gc) / (1 - gc));
}
double uniformDelay(double cycle, double gc, double x, double pf) {
    return pf * 0.5 * cycle * (1 - gc) * (1 - gc) / (1 - std::min(1.0, x) * gc);
}
double incrementalDelay(double period, double x, double k, double filtering, double capacity) {
    if (!(capacity > 0)) return 0;
    return 900 * period * ((x - 1) + std::sqrt((x - 1) * (x - 1) + 8 * k * filtering * x / (capacity * period)));
}
SignalisedResult analyseSignalised(const SignalisedLaneGroup& g) {
    SignalisedResult r;
    r.factors = saturationFactors(g);
    const auto& f = r.factors;
    r.saturation = g.baseSaturation * f.fw * f.fHVg * f.fp * f.fbb * f.fa * f.fLU * f.fLT * f.fRT; // Eq. 19-8
    r.greenRatio = g.effectiveGreen / g.cycle;
    r.capacity = g.lanes * r.saturation * r.greenRatio; // Eq. 19-16, veh/h for the lane group
    r.ratio = r.capacity > 0 ? g.flowRate / r.capacity : 0; // Eq. 19-17
    // A green over the whole cycle has no uniform delay; Eq. 19-20 divides by 1 - g/C.
    r.progression = r.greenRatio < 1 ? progressionFactor(g.arrivalsOnGreen, r.greenRatio, r.ratio) : 1;
    r.uniformDelay = r.greenRatio < 1 ? uniformDelay(g.cycle, r.greenRatio, r.ratio, r.progression) : 0;
    r.incrementalDelay = incrementalDelay(g.period, r.ratio, g.incrementalFactor, g.upstreamFiltering, r.capacity);
    r.initialQueueDelay = 0; // A1: no initial queue (Step 8B)
    r.delay = r.uniformDelay + r.incrementalDelay + r.initialQueueDelay; // Eq. 19-18
    return r;
}
std::optional<double> flowWeightedDelay(const std::vector<std::pair<double, double>>& groups) {
    double flow = 0, weighted = 0;
    for (const auto& [v, d] : groups) { flow += v; weighted += v * d; }
    if (!(flow > 0)) return std::nullopt;
    return weighted / flow;
}
char signalisedLos(double delay, std::optional<double> ratio, const std::array<double, 5>& bounds) {
    if (ratio && *ratio > 1.0) return 'F';
    for (std::size_t i = 0; i < bounds.size(); ++i)
        if (delay <= bounds[i]) return static_cast<char>('A' + i);
    return 'F';
}
}
