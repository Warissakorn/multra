#pragma once
// M6 S2's effective-green diagnostic (D541, docs/reference/M6_S2_RUN.md §8): capacity, effective green
// and lost time per 90/40/3 s cycle, and the references recomputed at the engine's own values.
// No band; decides nothing.
#include "m6_s2_summary.hpp"
#include <algorithm>
#include <cmath>
#include <optional>
#include <vector>

namespace multra::m6s2 {
inline constexpr double kGreenCycle = 90, kGreenGreen = 40, kGreenAmber = 3;

struct CycleCount { std::size_t n{}, amber{}; std::optional<double> hS; };
// §8 A: stamps in [go, go + 90), those in [go + 40, go + 43), and the mean headway of ranks 5-12.
inline CycleCount cycleCount(const std::vector<double>& stamps, double go) {
    CycleCount c;
    std::vector<double> in;
    for (const double t : stamps) {
        if (t < go || t >= go + kGreenCycle) continue;
        in.push_back(t);
        if (t >= go + kGreenGreen && t < go + kGreenGreen + kGreenAmber) ++c.amber;
    }
    std::sort(in.begin(), in.end());
    c.n = in.size();
    if (in.size() >= 12) {
        double sum = 0;
        for (std::size_t r = 4; r < 12; ++r) sum += in[r] - in[r - 1];
        c.hS = sum / 8;
    }
    return c;
}
struct Green { double n{}, amber{}, hS{}, effectiveGreen{}, lostTime{}, capacity{}; };
inline Green green(double meanN, double meanAmber, double meanHs) {
    Green g{meanN, meanAmber, meanHs, meanN * meanHs, 0, meanN * 3600 / kGreenCycle};
    g.lostTime = kGreenGreen + kGreenAmber - g.effectiveGreen;
    return g;
}
// §8 B: HCM d1 + d2 at a given green and capacity (T = 1 h, k = 0.5, I = 1, PF = 1).
inline double hcmDelayAt(double v, double cycle, double greenSeconds, double capacity) {
    const double X = v / capacity, gc = greenSeconds / cycle, T = 1, k = .5, I = 1;
    const double d1 = .5 * cycle * (1 - gc) * (1 - gc) / (1 - std::min(1.0, X) * gc);
    const double d2 = 900 * T * ((X - 1) + std::sqrt((X - 1) * (X - 1) + 8 * k * I * X / (capacity * T)));
    return d1 + d2;
}
struct Slots { int green{}, red{}; double slot{}; };
inline Slots declaredSlots() { return {21, 26, 3600 / kSaturation}; }
inline Slots engineSlots(double effectiveGreen, double hS) {
    return {static_cast<int>(std::lround(effectiveGreen / hS)),
            static_cast<int>(std::lround((kGreenCycle - effectiveGreen) / hS)), hS};
}
// h * E D in seconds, or nothing when (green + red) * mu >= green (no stationary queue).
inline std::optional<double> fctlSeconds(const Slots& s, double v) {
    const double mu = v * s.slot / 3600;
    if ((s.green + s.red) * mu >= s.green || s.green < 1) return std::nullopt;
    return s.slot * fctlPoisson({s.green, s.red, mu}).meanDelay;
}
}
