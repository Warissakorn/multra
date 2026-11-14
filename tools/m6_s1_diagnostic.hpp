#pragma once
// M6 kind S1's diagnostic (D530, D536, docs/reference/M6_S1_RUN.md §7): the engine's saturation
// headway split as [AkcBes02] Eq. 9, h_S = t_x + 3.6 L_hj / v_S. It decides nothing; no band.
#include "m6_s1_summary.hpp"
#include "../src/core/simulation.hpp"
#include <map>

namespace multra::m6s1 {
struct Record {
    std::optional<double> goTime, goDistance, goSpeed, moveTime, lineSpeed, crossingTime;
};
// §7 (D537): watches every tick; at each Go tick (the program turns green) it records each vehicle's
// front distance and speed, then the first later tick at or above beginSpeed, until the line.
class DiagnosticObserver {
public:
    DiagnosticObserver(double lineDistance, SignalProgram program, double beginSpeed)
        : line_(lineDistance), program_(std::move(program)), begin_(beginSpeed) {}
    void observe(const SimState& state) {
        const bool green = signalColorAt(program_, state.time) == SignalColor::green;
        const bool go = green && seen_ && !wasGreen_;
        seen_ = true; wasGreen_ = green;
        for (const auto& v : state.vehicles) {
            auto& r = records_[v.id];
            if (r.lineSpeed) continue;
            if (go) { r.goTime = state.time; r.goDistance = v.distance; r.goSpeed = v.speed; r.moveTime.reset(); }
            else if (r.goTime && !r.moveTime && v.speed >= begin_) r.moveTime = state.time;
            if (v.distance >= line_) { r.lineSpeed = v.speed; r.crossingTime = state.time; }
        }
    }
    const std::map<std::uint64_t, Record>& records() const { return records_; }
private:
    double line_;
    SignalProgram program_;
    double begin_;
    bool seen_{}, wasGreen_{};
    std::map<std::uint64_t, Record> records_;
};

// A usable cycle's ranks 1-12 with their records; complete when every rank has a record of this Go.
struct DiagnosticCycle {
    std::uint32_t seed{};
    double go{};
    std::vector<Record> ranks;
    bool complete() const {
        if (ranks.size() < kRanks) return false;
        for (std::size_t r = 0; r < kRanks; ++r)
            if (!ranks[r].goTime || std::abs(*ranks[r].goTime - go) > 1e-9 || !ranks[r].goDistance ||
                !ranks[r].goSpeed || !ranks[r].moveTime || !ranks[r].lineSpeed) return false;
        return true;
    }
};
inline DiagnosticCycle diagnosticCycle(const Cycle& c, const std::map<std::uint64_t, Record>& records) {
    DiagnosticCycle d{c.seed, c.go, {}};
    for (std::size_t r = 0; r < kRanks && r < c.vehicleIds.size(); ++r) {
        const auto found = records.find(c.vehicleIds[r]);
        d.ranks.push_back(found == records.end() ? Record{} : found->second);
    }
    return d;
}

struct Diagnostic {
    Figure jamSpacing, responseMeasured, responseEq10, saturationSpeed, firstResponse;
    std::array<Figure, kRanks> lineSpeed, goSpeed;  // km/h
    std::size_t cycles{}, unavailable{};
};
// §7: per complete cycle over ranks 5-12, per seed mean, then over seeds. `headway` is the seed's
// §3 mean headway H_seed (the seeds without one are skipped for Eq. 10 only).
inline Diagnostic diagnostic(const std::vector<DiagnosticCycle>& cycles, const std::vector<std::uint32_t>& seeds,
                             const std::map<std::uint32_t, double>& headway) {
    Diagnostic result;
    std::vector<double> jam, response, eq10, speed, first;
    std::array<std::vector<double>, kRanks> lines, gos;
    for (const auto seed : seeds) {
        double l = 0, t = 0, v = 0, t1 = 0; std::array<double, kRanks> rankSpeed{}, rankGo{};
        std::size_t n = 0;
        for (const auto& c : cycles) {
            if (c.seed != seed) continue;
            if (!c.complete()) { ++result.unavailable; continue; }
            ++n;
            double cl = 0, ct = 0, cv = 0;
            for (std::size_t r = 4; r < kRanks; ++r) {
                cl += *c.ranks[r - 1].goDistance - *c.ranks[r].goDistance;
                ct += *c.ranks[r].moveTime - *c.ranks[r - 1].moveTime;
                cv += *c.ranks[r].lineSpeed;
            }
            l += cl / 8; t += ct / 8; v += 3.6 * cv / 8;
            t1 += *c.ranks[0].moveTime - c.go;
            for (std::size_t r = 0; r < kRanks; ++r) {
                rankSpeed[r] += 3.6 * *c.ranks[r].lineSpeed; rankGo[r] += 3.6 * *c.ranks[r].goSpeed;
            }
        }
        result.cycles += n;
        if (n == 0) continue;
        const double k = static_cast<double>(n);
        jam.push_back(l / k); response.push_back(t / k); speed.push_back(v / k); first.push_back(t1 / k);
        for (std::size_t r = 0; r < kRanks; ++r) { lines[r].push_back(rankSpeed[r] / k); gos[r].push_back(rankGo[r] / k); }
        if (const auto h = headway.find(seed); h != headway.end())
            eq10.push_back(h->second - 3.6 * (l / k) / (v / k));
    }
    result.jamSpacing = figure(jam); result.responseMeasured = figure(response);
    result.responseEq10 = figure(eq10); result.saturationSpeed = figure(speed);
    result.firstResponse = figure(first);
    for (std::size_t r = 0; r < kRanks; ++r) { result.lineSpeed[r] = figure(lines[r]); result.goSpeed[r] = figure(gos[r]); }
    return result;
}
// Per seed H from §3's figures: 3600 / s_seed.
inline std::map<std::uint32_t, double> seedHeadways(const Saturation& s) {
    std::map<std::uint32_t, double> h;
    for (const auto& [seed, flow] : s.bySeed) h[seed] = 3600 / flow;
    return h;
}
inline Json diagnosticJson(const Diagnostic& d) {
    Json speeds = Json::array();
    for (std::size_t r = 0; r < kRanks; ++r) speeds.push_back({{"rank", r + 1}, {"lineKmh", figureJson(d.lineSpeed[r])}, {"goKmh", figureJson(d.goSpeed[r])}});
    return {{"label", "S1 diagnostic (D530, D536, D537): parts read at Go; no band, decides nothing"},
        {"cycles", d.cycles}, {"diagnosticUnavailable", d.unavailable},
        {"jamSpacingMetres", figureJson(d.jamSpacing)}, {"responseMeasuredSeconds", figureJson(d.responseMeasured)},
        {"responseEq10Seconds", figureJson(d.responseEq10)}, {"saturationSpeedKmh", figureJson(d.saturationSpeed)},
        {"firstResponseSeconds", figureJson(d.firstResponse)}, {"speedByRank", speeds},
        {"akcelikBesley", {{"tx", 1.17}, {"Lhj", 6.9}, {"vS", 45.1}, {"source", "AkcBes02 Table 2, Through (isolated) average"}}}};
}
}
