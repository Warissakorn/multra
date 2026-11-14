#pragma once
// M6 S2's arrival diagnostic (D540, docs/reference/M6_S2_RUN.md §7): stamps each vehicle once at
// fixed route distances and measures how far the stamps are from Poisson. No band; decides nothing.
#include "m6_s2_network.hpp"
#include <cmath>
#include <map>
#include <optional>
#include <set>
#include <vector>

namespace multra::m6s2 {
inline constexpr double kIntervalSeconds = 90;  // window [kWarmup, kWindowEnd) from m6_s2_network.hpp

class ArrivalObserver {
public:
    explicit ArrivalObserver(std::vector<double> points) : points_(std::move(points)), stamps_(points_.size()) {}
    // End-of-tick time of the first tick whose front is at or past each point, once per vehicle.
    void observe(const SimState& state) {
        for (const auto& v : state.vehicles)
            for (std::size_t p = 0; p < points_.size(); ++p)
                if (v.distance >= points_[p] && seen_[p].insert(v.id).second) stamps_[p].push_back(state.time);
    }
    const std::vector<double>& stamps(std::size_t point) const { return stamps_.at(point); }
private:
    std::vector<double> points_;
    std::vector<std::vector<double>> stamps_;
    std::map<std::size_t, std::set<std::uint64_t>> seen_;
};

// Index of dispersion of counts in consecutive intervals over [start, end): sample variance / mean.
inline std::optional<double> dispersion(const std::vector<double>& stamps, double start = kWarmup,
                                        double end = kWindowEnd, double interval = kIntervalSeconds) {
    const auto n = static_cast<std::size_t>(std::llround((end - start) / interval));
    if (n < 2) return std::nullopt;
    std::vector<double> counts(n, 0);
    for (const double t : stamps) {
        if (t < start || t >= end) continue;
        const auto k = static_cast<std::size_t>((t - start) / interval);
        if (k < n) counts[k] += 1;
    }
    double mean = 0;
    for (const double c : counts) mean += c;
    mean /= static_cast<double>(n);
    if (mean <= 0) return std::nullopt;
    double var = 0;
    for (const double c : counts) var += (c - mean) * (c - mean);
    return var / static_cast<double>(n - 1) / mean;
}
// Coefficient of variation of the headways between consecutive stamps inside [start, end).
inline std::optional<double> headwayCv(const std::vector<double>& stamps, double start = kWarmup,
                                       double end = kWindowEnd) {
    std::vector<double> inside;
    for (const double t : stamps) if (t >= start && t < end) inside.push_back(t);
    if (inside.size() < 3) return std::nullopt;
    std::vector<double> h;
    for (std::size_t i = 1; i < inside.size(); ++i) h.push_back(inside[i] - inside[i - 1]);
    double mean = 0;
    for (const double x : h) mean += x;
    mean /= static_cast<double>(h.size());
    if (mean <= 0) return std::nullopt;
    double var = 0;
    for (const double x : h) var += (x - mean) * (x - mean);
    return std::sqrt(var / static_cast<double>(h.size() - 1)) / mean;
}
}
