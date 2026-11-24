#include "w99.hpp"
#include "w74.hpp"
#include "speed_distribution.hpp"
#include <stdexcept>
#include "start_up.hpp"
#include <algorithm>
#include <cmath>

namespace multra {
namespace {
// §3's fixed structure: a leader braking harder than this is taken as braking; CC6's display
// scale; CC9's speed (80 km/h). Not tunable content (rule 5).
constexpr double kLeaderBraking = -1.0;
constexpr double kCc6Scale = 1e-4;
constexpr double kCc9Speed = 80.0 / 3.6;
// §4, closing: the source's margin below SDXc.
constexpr double kClosingMargin = 0.1;

enum class Range { positive, nonNegative, nonPositive };
struct KeyRange { W99Key key; Range range; };
const std::vector<KeyRange>& keyRanges() {
    using P = W99Parameters;
    static const std::vector<KeyRange> table{
        {{"cc0", &P::cc0}, Range::positive}, {{"cc1", &P::cc1}, Range::nonNegative},
        {{"cc2", &P::cc2}, Range::nonNegative}, {{"cc3", &P::cc3}, Range::nonPositive},
        {{"cc4", &P::cc4}, Range::nonPositive}, {{"cc5", &P::cc5}, Range::nonNegative},
        {{"cc6", &P::cc6}, Range::nonNegative}, {{"cc7", &P::cc7}, Range::positive},
        {{"cc8", &P::cc8}, Range::positive}, {{"cc9", &P::cc9}, Range::nonNegative}};
    return table;
}
bool inRange(double v, Range range) {
    if (!std::isfinite(v)) return false;
    switch (range) {
    case Range::positive: return v > 0;
    case Range::nonNegative: return v >= 0;
    case Range::nonPositive: return v <= 0;
    }
    return false;
}
W99Regime classify(double g, double dv, const W99Thresholds& t) {
    if (dv < t.sdvo && g <= t.sdxc) return W99Regime::tooClose;
    if (dv < t.sdvc && g < t.sdxv) return W99Regime::closing;
    if (dv < t.sdvo && g < t.sdxo) return W99Regime::following;
    return W99Regime::free;
}
// §4, free: CC8 at standstill to CC9 at 80 km/h, linear, constant above; plus driverFactor.
double freeMaximum(double speed, double driverFactor, const W99Parameters& p) {
    return p.cc8 + (p.cc9 - p.cc8) * std::min(speed, kCc9Speed) / kCc9Speed + driverFactor;
}
FollowingMode modeOf(W99Regime regime) {
    switch (regime) {
    case W99Regime::tooClose: return FollowingMode::braking;
    case W99Regime::closing: return FollowingMode::approaching;
    case W99Regime::following: return FollowingMode::following;
    case W99Regime::free: break;
    }
    return FollowingMode::free;
}
}
W99Thresholds w99Thresholds(double speed, double zW, const W99Parameters& p, const Leader& obstacle) {
    W99Thresholds t{};
    const double g = obstacle.gap, vL = obstacle.speed, dv = vL - speed;
    t.vSlow = dv >= 0 || obstacle.acceleration < kLeaderBraking ? speed : vL + dv * (zW - 0.5);
    t.sdxc = vL <= 0 ? p.cc0 : p.cc0 + p.cc1 * std::max(0.0, t.vSlow);
    t.sdxo = t.sdxc + p.cc2;
    t.sdxv = t.sdxo + p.cc3 * (dv - p.cc4);
    t.sdv = p.cc6 * kCc6Scale * g * g;
    t.sdvc = vL > 0 ? p.cc4 - t.sdv : 0;
    t.sdvo = speed > p.cc5 ? t.sdv + p.cc5 : t.sdv;
    return t;
}
W99Result w99Acceleration(double speed, double desiredSpeed, double aPrev, double timeStep, double driverFactor,
    double zW, const VehicleType& type, const W99Parameters& p, std::optional<Leader> obstacle) {
    W99Regime regime = W99Regime::free;
    double a{};
    if (!obstacle) {
        a = speed >= desiredSpeed ? 0 : freeMaximum(speed, driverFactor, p);
    } else {
        const auto t = w99Thresholds(speed, zW, p, *obstacle);
        const double g = obstacle->gap, dv = obstacle->speed - speed, aL = obstacle->acceleration;
        regime = classify(g, dv, t);
        switch (regime) {
        case W99Regime::tooClose:
            if (speed > 0) {
                double a0 = 0;
                if (dv < 0) a0 = g > p.cc0 ? std::min(aL + dv * dv / (p.cc0 - g), aPrev)
                                           : std::min(aL + 0.5 * (dv - t.sdvo), aPrev);
                a = a0 > -p.cc7 ? -p.cc7 : a0; // always brakes at least by CC7
            }
            break;
        case W99Regime::closing: // g > SDXc here (§3's ordering), so the denominator is ≤ −0.1 m
            a = 0.5 * dv * dv / (t.sdxc - g - kClosingMargin);
            break;
        case W99Regime::following:
            // §4's standstill rule (ours): a driver at rest in the band creeps, or a Stop line it
            // stopped short of is never served.
            if (speed == 0) a = std::max(aPrev, p.cc7);
            else a = aPrev <= 0 ? std::min(aPrev, -p.cc7) : std::max(aPrev, p.cc7);
            break;
        case W99Regime::free:
            if (g <= t.sdxc || speed >= desiredSpeed) a = 0;
            else {
                const double aMax = freeMaximum(speed, driverFactor, p);
                a = g < t.sdxo ? std::min(dv * dv / (t.sdxo - g), aMax) : aMax;
            }
            break;
        }
    }
    a = std::clamp(a, -type.maxDeceleration, maxAccelerationAt(type, speed)); // §6, D608
    // D105, as in the prototype and W74: a stopped vehicle within the standstill gap waits for room.
    // §4 (D650): it also waits while one whole creep (a tick of +CC7, then braking to rest, at most
    // CC7·dt²) would not fit before CC0, where the hard cap would clamp it every tick.
    if (speed == 0 && obstacle && obstacle->gap - p.cc0 <= p.cc7 * timeStep * timeStep) a = std::min(0.0, a);
    return {a, regime, modeOf(regime)};
}
const std::vector<W99Key>& w99ParameterKeys() {
    static const std::vector<W99Key> keys = [] {
        std::vector<W99Key> result;
        for (const auto& k : keyRanges()) result.push_back(k.key);
        return result;
    }();
    return keys;
}
std::vector<ValidationIssue> w99ParameterIssues(const W99Parameters& p, const std::string& path) {
    std::vector<ValidationIssue> issues;
    for (const auto& k : keyRanges())
        if ((!p.cc1Curve.empty() && k.key.member == &W99Parameters::cc1)) continue; // §5a: the curve stands in for cc1
        else if (!inRange(p.*k.key.member, k.range)) issues.push_back({"INVALID_BEHAVIOUR_PARAMETER", path + "." + k.key.name});
    // §5a (D646): SPEED_DISTRIBUTION §2's point rule, `value` for `speed`.
    const auto& c = p.cc1Curve;
    if (c.empty()) return issues;
    const auto at = [&](std::size_t i, const char* key) { return path + ".cc1Curve[" + std::to_string(i) + "]." + key; };
    if (c.size() < 2) issues.push_back({"INVALID_BEHAVIOUR_PARAMETER", path + ".cc1Curve"});
    for (std::size_t i = 0; i < c.size(); ++i) {
        if (!std::isfinite(c[i].value) || c[i].value < 0 || (i > 0 && !(c[i].value > c[i - 1].value)))
            issues.push_back({"INVALID_BEHAVIOUR_PARAMETER", at(i, "value")});
        const bool end = (i == 0 && c[i].share != 0) || (i + 1 == c.size() && c[i].share != 1);
        if (!std::isfinite(c[i].share) || c[i].share < 0 || c[i].share > 1 || end || (i > 0 && c[i].share < c[i - 1].share))
            issues.push_back({"INVALID_BEHAVIOUR_PARAMETER", at(i, "share")});
    }
    return issues;
}
double w99Trait(std::uint32_t seed, std::uint64_t vehicleId) { return w74Uniform(seed, vehicleId, kW99TraitIndex); }
double w99Cc1Share(std::uint32_t seed, std::uint64_t vehicleId) { return w74Uniform(seed, vehicleId, kW99Cc1Index); }
double w99Cc1At(const std::vector<W99Cc1Point>& curve, double u) {
    return cumulativeInverse(curve, u, [](const W99Cc1Point& p) { return p.value; });
}
W99Parameters w99DriverParameters(const W99Parameters& p, std::optional<double> cc1Share) {
    double cc1 = p.cc1;
    if (!p.cc1Curve.empty()) {
        if (!cc1Share) throw std::logic_error("w99 cc1Curve without a driver share");
        cc1 = w99Cc1At(p.cc1Curve, *cc1Share);
    }
    return {p.cc0, cc1, p.cc2, p.cc3, p.cc4, p.cc5, p.cc6, p.cc7, p.cc8, p.cc9, {}}; // no curve copied
}
}
