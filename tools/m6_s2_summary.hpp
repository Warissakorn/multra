#pragma once
// M6 kind S2-c's reduction and verdict (D538, docs/reference/M6_S2_RUN.md §3-§4). Pure functions,
// so `m6s2` drives them with synthetic per-seed values (S2C2-S2C5) before any run.
#include "m6_s1_summary.hpp"
#include "m6_s2_network.hpp"
#include "../src/eval/fctl_reference.hpp"

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
}
