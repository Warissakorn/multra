#pragma once
// M6 kind S1's reductions and verdicts (D535, docs/reference/M6_S1_RUN.md §3-§4). Pure functions
// over discharge cycles, so `m6s1` drives them with synthetic cycles (MS2-MS6) before any run.
#include "../src/project/json.hpp"
#include "../src/runner/batch.hpp"
#include <array>
#include <cmath>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace multra::m6s1 {
inline constexpr std::size_t kRanks = 12;
inline constexpr std::size_t kMinimumCycles = 30;

struct Cycle {
    std::uint32_t seed{};
    double go{};
    std::string reason;                 // empty: usable
    std::optional<double> meanHeadway;  // ranks 5-12, from estimateDischarge
    std::vector<double> times;          // crossing times in rank order
    std::vector<std::uint64_t> vehicleIds;  // the same crossings' vehicles (D536 diagnostic)
};
struct Figure {
    std::optional<double> mean, sd, halfWidth95;
    std::size_t n{};
};
enum class Verdict { pass, miss, unavailable };

inline const char* verdictName(Verdict v) {
    return v == Verdict::pass ? "pass" : v == Verdict::miss ? "miss" : "unavailable";
}
// §4: |mean - reference| <= band * reference, edge included. A tiny relative slack absorbs the
// binary representation of an exact edge (1,995 against 1,900 is 5 % exactly).
inline Verdict verdict(std::optional<double> mean, double reference, double band = .05) {
    if (!mean) return Verdict::unavailable;
    return std::abs(*mean - reference) <= band * reference * (1 + 1e-12) ? Verdict::pass : Verdict::miss;
}
inline Figure figure(const std::vector<double>& values) {
    Figure f;
    f.n = values.size();
    if (values.empty()) return f;
    double sum = 0;
    for (const double v : values) sum += v;
    f.mean = sum / static_cast<double>(f.n);
    if (f.n < 2) return f;
    double squares = 0;
    for (const double v : values) squares += (v - *f.mean) * (v - *f.mean);
    f.sd = std::sqrt(squares / static_cast<double>(f.n - 1));
    f.halfWidth95 = tQuantile975(f.n - 1) * *f.sd / std::sqrt(static_cast<double>(f.n));
    return f;
}
inline bool usable(const Cycle& c) { return c.reason.empty() && c.meanHeadway && c.times.size() >= kRanks; }

// §3, S1-a and S1-f's s: per seed 3600 / (mean of per-cycle mean headways), then over seeds.
struct Saturation {
    Figure flow;
    std::map<std::uint32_t, double> bySeed;
    std::vector<std::uint32_t> seedsWithoutCycles;
    std::size_t usableCycles{};
};
inline Saturation saturation(const std::vector<Cycle>& cycles, const std::vector<std::uint32_t>& seeds) {
    Saturation result;
    std::vector<double> flows;
    for (const auto seed : seeds) {
        double sum = 0; std::size_t n = 0;
        for (const auto& c : cycles)
            if (c.seed == seed && usable(c)) { sum += *c.meanHeadway; ++n; }
        result.usableCycles += n;
        if (n == 0) { result.seedsWithoutCycles.push_back(seed); continue; }
        result.bySeed[seed] = 3600 / (sum / static_cast<double>(n));
        flows.push_back(result.bySeed[seed]);
    }
    result.flow = figure(flows);
    return result;
}

// §3, S1-d and S1-f's profile: per rank, per seed mean over usable cycles, then over seeds.
struct Ranks {
    std::array<Figure, kRanks> headway, crossing;
    std::size_t usableCycles{};
    bool available{};
};
inline Ranks byRank(const std::vector<Cycle>& cycles, const std::vector<std::uint32_t>& seeds) {
    Ranks result;
    std::array<std::vector<double>, kRanks> headways, crossings;
    for (const auto seed : seeds) {
        std::array<double, kRanks> h{}, t{};
        std::size_t n = 0;
        for (const auto& c : cycles) {
            if (c.seed != seed || !usable(c)) continue;
            ++n;
            for (std::size_t r = 0; r < kRanks; ++r) {
                h[r] += c.times[r] - (r == 0 ? c.go : c.times[r - 1]);
                t[r] += c.times[r] - c.go;
            }
        }
        result.usableCycles += n;
        if (n == 0) continue;
        for (std::size_t r = 0; r < kRanks; ++r) {
            headways[r].push_back(h[r] / static_cast<double>(n));
            crossings[r].push_back(t[r] / static_cast<double>(n));
        }
    }
    result.available = result.usableCycles >= kMinimumCycles;
    if (!result.available) return result;
    for (std::size_t r = 0; r < kRanks; ++r) {
        result.headway[r] = figure(headways[r]);
        result.crossing[r] = figure(crossings[r]);
    }
    return result;
}

// [AkcBes02] Eq. 7: n_s(t) = (q_n/3600)[(t - t_r) - (1 - e^{-m_q (t - t_r)})/m_q]. Returns the t at
// which n_s(t) = k, by bisection on [t_r, t_r + 200] to 1e-9 s.
inline double akcelikTime(double k, double qn = 2086, double mq = .369, double tr = 0) {
    const auto served = [&](double t) { return qn / 3600 * ((t - tr) - (1 - std::exp(-mq * (t - tr))) / mq); };
    double lo = tr, hi = tr + 200;
    while (hi - lo > 1e-9) {
        const double mid = (lo + hi) / 2;
        (served(mid) < k ? lo : hi) = mid;
    }
    return (lo + hi) / 2;
}

inline std::map<std::string, std::size_t> reasons(const std::vector<Cycle>& cycles) {
    std::map<std::string, std::size_t> counts;
    for (const auto& c : cycles) counts[usable(c) ? std::string("usable") : c.reason.empty() ? "noEstimate" : c.reason] += 1;
    return counts;
}

// One cycle of dischargeJson's output (src/project/discharge_json.cpp).
inline Cycle cycleOf(std::uint32_t seed, const Json& row) {
    Cycle c;
    c.seed = seed;
    c.go = row.at("go").get<double>();
    c.reason = row.at("unavailableReason").get<std::string>();
    if (!row.at("meanHeadwaySeconds").is_null()) c.meanHeadway = row.at("meanHeadwaySeconds").get<double>();
    for (const auto& x : row.at("crossings")) {
        c.times.push_back(x.at("time").get<double>());
        c.vehicleIds.push_back(x.at("vehicleId").get<std::uint64_t>());
    }
    return c;
}

inline const std::array<double, kRanks> kLeeChen{3.70, 2.46, 2.28, 2.17, 2.07, 2.00, 1.93, 1.91, 1.93, 1.78, 1.64, 1.76};
inline const std::array<double, kRanks> kBonneson{3.22, 2.10, 2.03, 1.97, 1.93, 1.89, 1.85, 1.83, 1.81, 1.79, 1.78, 1.76};

inline Json figureJson(const Figure& f) {
    const auto value = [](const std::optional<double>& v) { return v ? Json(*v) : Json(nullptr); };
    return {{"mean", value(f.mean)}, {"sd", value(f.sd)}, {"halfWidth95", value(f.halfWidth95)}, {"n", f.n}};
}
// §4's table as one object.
inline Json summaryJson(const std::vector<Cycle>& cycles, const std::vector<std::uint32_t>& seeds) {
    const auto s = saturation(cycles, seeds);
    const auto r = byRank(cycles, seeds);
    Json bySeed = Json::object();
    for (const auto& [seed, flow] : s.bySeed) bySeed[std::to_string(seed)] = flow;
    Json ranks = Json::array();
    for (std::size_t k = 0; k < kRanks; ++k) {
        const bool banded = k >= 1 && k <= 8;
        const auto& h = r.headway[k];
        ranks.push_back({{"rank", k + 1}, {"headway", figureJson(h)}, {"crossing", figureJson(r.crossing[k])},
            {"leeChen", kLeeChen[k]}, {"bonneson", kBonneson[k]}, {"akcelikTime", akcelikTime(static_cast<double>(k + 1))},
            {"verdict", banded ? verdictName(verdict(r.available ? h.mean : std::nullopt, kLeeChen[k])) : "shown"}});
    }
    return {{"label", "M6 kind S1 (D535); a comparison, not a calibration"},
        {"seeds", seeds}, {"cycles", reasons(cycles)}, {"usableCycles", s.usableCycles},
        {"seedsWithoutCycles", s.seedsWithoutCycles},
        {"s1a", {{"figure", figureJson(s.flow)}, {"bySeed", bySeed}, {"reference", 1900},
                 {"verdict", verdictName(verdict(s.flow.mean, 1900))}, {"zegeer86", 1875}}},
        {"s1f", {{"figure", figureJson(s.flow)}, {"reference", 2083},
                 {"verdict", verdictName(verdict(s.flow.mean, 2083))}}},
        {"s1d", {{"usableCycles", r.usableCycles}, {"available", r.available}, {"ranks", ranks}}}};
}
}
