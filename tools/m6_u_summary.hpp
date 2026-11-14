#pragma once
// M6 kind U's reductions and references (D542, docs/reference/M6_U_RUN.md §4). Pure functions of
// the per-seed priority-line rows, so `m6u` checks each on hand values.
#include "m6_s1_summary.hpp"
#include "../src/eval/priority_lines.hpp"
#include <cmath>
#include <map>

namespace multra::m6u {
inline constexpr double kMinimumShare = .9, kBand = .15;
// HCM 7 Eq. 20-18 (Harders' form), veh/h.
inline double harders(double vc, double tc, double tf) {
    return vc * std::exp(-vc * tc / 3600) / (1 - std::exp(-vc * tf / 3600));
}
// Siegloch's form, veh/h; shown without a band.
inline double siegloch(double vc, double t0, double tf) { return 3600 / tf * std::exp(-vc * t0 / 3600); }
// The saturated share of the window's major gaps; nothing without a gap.
inline std::optional<double> share(const PriorityLineRow& r) {
    if (r.majorEntered < 2) return std::nullopt;
    return static_cast<double>(r.sieglochGaps) / static_cast<double>(r.majorEntered - 1);
}
inline bool available(double meanShare) { return meanShare >= kMinimumShare; }
// Siegloch points of many rows: per n, gaps summed and meanGap weighted by gaps.
inline std::vector<SieglochPoint> pool(const std::vector<PriorityLineRow>& rows) {
    std::map<std::uint64_t, std::pair<std::uint64_t, double>> byN;
    for (const auto& r : rows)
        for (const auto& p : r.sieglochPoints) {
            auto& [gaps, sum] = byN[p.vehicles];
            gaps += p.gaps; sum += p.meanGap * static_cast<double>(p.gaps);
        }
    std::vector<SieglochPoint> points;
    for (const auto& [n, g] : byN) points.push_back({n, g.first, g.second / static_cast<double>(g.first)});
    return points;
}
// followUpMean weighted by followUps; nothing without a follow-up.
inline std::optional<double> pooledFollowUp(const std::vector<PriorityLineRow>& rows) {
    double sum = 0; std::uint64_t n = 0;
    for (const auto& r : rows) if (r.followUpMean) { sum += *r.followUpMean * static_cast<double>(r.followUps); n += r.followUps; }
    if (n == 0) return std::nullopt;
    return sum / static_cast<double>(n);
}
inline m6s1::Verdict verdict(std::optional<double> mean, double reference, bool levelAvailable) {
    return levelAvailable ? m6s1::verdict(mean, reference, kBand) : m6s1::Verdict::unavailable;
}
}
