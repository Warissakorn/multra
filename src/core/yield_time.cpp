#include "yield_time.hpp"
#include "detail.hpp"
#include "random_streams.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace multra {
namespace {
constexpr double kStandstill = 0.1;          // m/s, SUMO's halting speed and the engine's stopped speed
constexpr double kTimeToImpatience = 180.0;  // s, SUMO's --time-to-impatience default
constexpr double kMaxYieldTime = 3600.0;     // s, the range's top and yieldTimeMaximum's default
constexpr int kDraws = 8;                    // truncated normal: resampled pairs before clamping
}
double vehicleYieldTime(const DriverBehaviour& b, std::uint32_t seed, const Vehicle& v) {
    if (!b.yieldTime) return 0;
    const double mean = *b.yieldTime, sd = b.yieldTimeDeviation.value_or(0);
    const double lo = b.yieldTimeMinimum.value_or(0), hi = b.yieldTimeMaximum.value_or(kMaxYieldTime);
    if (sd <= 0) return std::clamp(mean, lo, hi);
    const auto mix = detail::splitmix64Mix;
    const std::uint64_t identity = detail::traitIdentity(v.id, v.streamKey, v.ordinal);
    const std::uint64_t base = mix(static_cast<std::uint64_t>(seed)) ^ (identity * 0x9E3779B97F4A7C15ULL);
    const auto u = [&](std::uint64_t i) { return (static_cast<double>(mix(base ^ mix(61 + i)) >> 11) + 0.5) * 0x1p-53; };
    double x = mean;
    for (int k = 0; k < kDraws; ++k) {
        const double radius = u(2 * static_cast<std::uint64_t>(k)), angle = u(2 * static_cast<std::uint64_t>(k) + 1);
        x = mean + sd * std::sqrt(-2 * std::log(radius)) * std::cos(2 * std::numbers::pi * angle);
        if (x >= lo && x <= hi) return x;
    }
    return std::clamp(x, lo, hi);
}
double requiredGap(double gapTime, const std::optional<GapReduction>& r, const DriverBehaviour& b,
                   std::uint32_t seed, const Vehicle& v, double waited) {
    if (!r || !b.yieldTime || waited <= 0) return gapTime;
    const double y = vehicleYieldTime(b, seed, v), from = r->start * y, to = r->end * y;
    if (waited <= from) return gapTime;
    if (waited >= to) return r->finalGapTime;
    return gapTime + (r->finalGapTime - gapTime) * (waited - from) / (to - from);
}
double impatienceAfter(const DriverBehaviour& b, double waited) {
    if (!b.impatience) return 0;
    return std::clamp(*b.impatience + waited / b.timeToImpatience.value_or(kTimeToImpatience), 0.0, 1.0);
}
bool majorCanYield(double impatience, double distance, double speed, double maxDeceleration) {
    if (impatience <= 0 || distance < 0) return false;
    const double m = maxDeceleration * impatience;
    return m > 0 && 0.5 * speed * speed / m <= distance;
}
void countStandstill(std::vector<Vehicle>& vehicles) {
    for (auto& v : vehicles) v.standstillTicks = v.speed < kStandstill ? v.standstillTicks + 1 : 0;
}
std::vector<ValidationIssue> yieldBehaviourIssues(const DriverBehaviour& b, const std::string& path) {
    std::vector<ValidationIssue> issues;
    const auto bad = [&](const char* key) { issues.push_back({"INVALID_BEHAVIOUR_PARAMETER", path + "." + key}); };
    const auto in = [](const std::optional<double>& x, double lo, double hi, bool open) {
        return !x || (std::isfinite(*x) && (open ? *x > lo : *x >= lo) && *x <= hi);
    };
    if (!in(b.yieldTime, 0, kMaxYieldTime, true)) bad("yieldTime");
    const double mean = b.yieldTime.value_or(0);
    if (b.yieldTimeDeviation && (!b.yieldTime || !in(b.yieldTimeDeviation, 0, kMaxYieldTime, false))) bad("yieldTimeDeviation");
    if (b.yieldTimeMinimum && (!b.yieldTime || !in(b.yieldTimeMinimum, 0, mean, false))) bad("yieldTimeMinimum");
    if (b.yieldTimeMaximum && (!b.yieldTime || !in(b.yieldTimeMaximum, mean, kMaxYieldTime, false))) bad("yieldTimeMaximum");
    if (!in(b.impatience, -1, 1, false)) bad("impatience");
    if (b.timeToImpatience && (!b.impatience || !in(b.timeToImpatience, 0, kMaxYieldTime, true))) bad("timeToImpatience");
    return issues;
}
std::vector<ValidationIssue> gapReductionIssues(const std::optional<GapReduction>& r, double gapTime,
                                                const std::string& path, const char* code) {
    if (!r) return {};
    std::vector<ValidationIssue> issues;
    if (!(std::isfinite(r->finalGapTime) && r->finalGapTime >= 0 && r->finalGapTime <= gapTime)) issues.push_back({code, path + ".finalGapTime"});
    if (!(std::isfinite(r->start) && r->start >= 0 && r->start < 100)) issues.push_back({code, path + ".gapReductionStart"});
    if (!(std::isfinite(r->end) && r->end > r->start && r->end <= 100)) issues.push_back({code, path + ".gapReductionEnd"});
    return issues;
}
}
