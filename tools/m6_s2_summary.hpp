#pragma once
// M6 kind S2-c's reduction and verdict (D538, docs/reference/M6_S2_RUN.md §3-§4). Pure functions,
// so `m6s2` drives them with synthetic per-seed values (S2C2-S2C5) before any run.
#include "m6_s1_summary.hpp"
#include "m6_s2_network.hpp"
#include "../src/eval/fctl_reference.hpp"
#include <algorithm>
#include <cmath>

namespace multra::m6s2 {
inline constexpr double kBand = .15;

struct Level { double x{}, mu{}, flow{}, slot{}, reference{}; };
// §1's table and §4's reference: mu = x/2 per slot, v = mu * s, h * E D (slots -> seconds).
inline Level level(double x) {
    Level l;
    l.x = x; l.mu = x * kGreenSlots / (kGreenSlots + kRedSlots);
    l.slot = 3600 / kSaturation;
    l.flow = l.mu * 3600 / l.slot;
    l.reference = l.slot * fctlPoisson({kGreenSlots, kRedSlots, l.mu}).meanDelay;
    return l;
}
struct Paired {
    m6s1::Figure delta;
    std::map<std::uint32_t, double> bySeed;
    std::vector<std::uint32_t> missing;
};
// §3: per seed signal minus none, in the batch's seed order; a seed missing either value is listed.
inline Paired paired(const std::vector<std::uint32_t>& seeds, const std::vector<std::optional<double>>& signal,
                     const std::vector<std::optional<double>>& none) {
    if (signal.size() != seeds.size() || none.size() != seeds.size())
        throw std::invalid_argument("Per-seed values must match the seeds");
    Paired p;
    std::vector<double> values;
    for (std::size_t i = 0; i < seeds.size(); ++i) {
        if (!signal[i] || !none[i]) { p.missing.push_back(seeds[i]); continue; }
        p.bySeed[seeds[i]] = *signal[i] - *none[i];
        values.push_back(p.bySeed[seeds[i]]);
    }
    p.delta = m6s1::figure(values);
    return p;
}
inline m6s1::Verdict verdict(std::optional<double> mean, double reference) { return m6s1::verdict(mean, reference, kBand); }

// D539 (§6): HCM 7 d1 + d2 for one lane, cycle C, effective green g, PF = 1, T = 1 h, k = 0.5, I = 1.
inline double hcmDelay(double x, double cycle = 90, double green = 40) {
    const double c = kSaturation * green / cycle, gc = green / cycle, T = 1, k = .5, I = 1;
    const double d1 = .5 * cycle * (1 - gc) * (1 - gc) / (1 - std::min(1.0, x) * gc);
    const double d2 = 900 * T * ((x - 1) + std::sqrt((x - 1) * (x - 1) + 8 * k * I * x / (c * T)));
    return d1 + d2;
}
inline Level hcmLevel(double x) {
    Level l;
    l.x = x; l.slot = 3600 / kSaturation;
    l.flow = x * kSaturation * 40 / 90;
    l.reference = hcmDelay(x);
    return l;
}
// §6: a level counts only when no seed of its signal run ends with an unfinished window trip or a
// vehicle pending at a source.
inline bool available(const std::vector<std::optional<double>>& unfinished, const std::vector<std::optional<double>>& pending) {
    for (const auto* values : {&unfinished, &pending})
        for (const auto& v : *values) if (!v || *v > 0) return false;
    return true;
}
}
