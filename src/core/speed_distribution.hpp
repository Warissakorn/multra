#pragma once
#include "speed_decision_types.hpp"
#include <algorithm>

namespace trafficsim {
// B-5 (docs/reference/SPEED_DISTRIBUTION.md §3, D229). The speed of cumulative share u, and the share
// of a speed. Without a curve both are the uniform formulas the engine always used, to the bit.
inline double speedAt(const SpeedRange& r, double u) {
    if (r.curve.empty()) return r.min + u * (r.max - r.min);
    for (std::size_t i = 1; i < r.curve.size(); ++i) {
        const auto& a = r.curve[i - 1]; const auto& b = r.curve[i];
        if (u <= b.share && b.share > a.share) return a.speed + (u - a.share) / (b.share - a.share) * (b.speed - a.speed);
    }
    return r.curve.back().speed;
}
// §3: an edited min/max moves every curve speed linearly onto the new range, keeping the shares.
inline void fitCurve(SpeedRange& r) {
    if (r.curve.size() < 2) return;
    const double from = r.curve.front().speed, to = r.curve.back().speed;
    if ((from == r.min && to == r.max) || !(to > from)) return;
    for (auto& p : r.curve) p.speed = r.min + (p.speed - from) / (to - from) * (r.max - r.min);
    r.curve.front().speed = r.min; r.curve.back().speed = r.max;
}
inline double shareOf(const SpeedRange& r, double speed) {
    if (r.curve.empty()) {
        const double span = r.max - r.min; // as D191's rescale
        return span > 0 ? std::clamp((speed - r.min) / span, 0.0, 1.0) : 0.5;
    }
    if (speed <= r.curve.front().speed) return r.curve.front().share;
    for (std::size_t i = 1; i < r.curve.size(); ++i) {
        const auto& a = r.curve[i - 1]; const auto& b = r.curve[i];
        if (speed <= b.speed) return a.share + (speed - a.speed) / (b.speed - a.speed) * (b.share - a.share);
    }
    return r.curve.back().share;
}
}
