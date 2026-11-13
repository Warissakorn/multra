#pragma once
// M6 kind U's follow-up split (D543, docs/reference/M6_U_RUN.md §6): where a Stop follow-up's
// headway goes. The observer records, on the one area, minor crossings and major entries
// (interpolated as the priority-line counter does), each minor vehicle's first standing, Stop
// service start and end, the nearest-to-line trace, and per state whether the gap rule blocks.
// No band; decides nothing.
#include "../src/core/types.hpp"
#include "../src/core/conflicts.hpp"
#include <algorithm>
#include <map>
#include <optional>
#include <vector>

namespace multra::m6u {
// M3_CONTRACT §4's gap test for one major vehicle, recomputed: d = entry - front.
inline bool blocks(double d, double speed, bool rearShortOfExit, double gapTime, double headway) {
    if (d < 0) return rearShortOfExit;
    if (d <= headway) return true;
    return speed > 0 && d / speed < gapTime;
}
struct Phases { double headway{}, moveUp{}, dwell{}, release{}, blocked{}, free{}; };
inline Phases phases(double tl, double s0, double s1, double tf, double blockedTime) {
    return {tf - tl, s0 - tl, s1 - s0, tf - s1, blockedTime, (tf - s1) - blockedTime};
}
struct Sample { double time{}, distance{}, speed{}; };
struct Approach { double settle{}, moveDistance{}, peakSpeed{}; };
// §6: settle is s0 minus the first time at or after tl within 0.5 m of the front at s0.
inline Approach approach(const std::vector<Sample>& trace, double tl, double s0) {
    Approach a;
    std::optional<double> start, at, near;
    for (const auto& p : trace) if (p.time <= s0 + 1e-9) at = p.distance;
    if (!at) return a;
    for (const auto& p : trace) {
        if (p.time < tl - 1e-9 || p.time > s0 + 1e-9) continue;
        if (!start) start = p.distance;
        a.peakSpeed = std::max(a.peakSpeed, p.speed);
        if (!near && p.distance >= *at - 0.5) near = p.time;
    }
    if (start) a.moveDistance = *at - *start;
    if (near) a.settle = s0 - *near;
    return a;
}
struct Pair { Phases phases; Approach approach; double standDistance{}; };

class SplitObserver {
public:
    SplitObserver(double warmup, double end, double beginSpeed) : warmup_(warmup), end_(end), beginSpeed_(beginSpeed) {}
    void observe(const SimState& state) {
        const auto& s = *state.scenario;
        const auto& zone = s.conflictZones.at(0);
        const double t = state.time, dt = s.timeStep;
        bool blocked = false;
        const RouteZone* nearest = nullptr; std::uint64_t nearestId = 0; double nearestDistance = -1, nearestSpeed = 0;
        std::map<std::uint64_t, double> now;
        for (const auto& v : state.vehicles) {
            now[v.id] = v.distance;
            const auto before = previous_.find(v.id);
            for (const auto& z : state.index->routeZones[v.routeIndex]) {
                if (z.zoneIndex != 0) continue;
                const double length = s.vehicleTypes[v.typeIndex].length;
                if (z.role == ZoneRole::major) {
                    if (blocks(z.entryAt - v.distance, v.speed, v.distance - length < z.exitAt, zone.gapTime, zone.headway)) blocked = true;
                    if (before != previous_.end() && before->second < z.entryAt && v.distance >= z.entryAt)
                        major_.push_back(cross(t, dt, before->second, v.distance, z.entryAt));
                    continue;
                }
                auto& m = minor_[v.id];
                if (v.distance < z.waitAt) {
                    if (!m.stood && v.speed < beginSpeed_) m.stood = t;
                    if (v.distance > nearestDistance) { nearest = &z; nearestId = v.id; nearestDistance = v.distance; nearestSpeed = v.speed; }
                }
                if (before != previous_.end() && before->second < z.waitAt && v.distance >= z.waitAt && !m.crossed) {
                    m.crossed = cross(t, dt, before->second, v.distance, z.waitAt);
                    m.waitAt = z.waitAt;
                }
            }
        }
        for (const auto& service : state.stopService) {
            auto m = minor_.find(service.vehicleId);
            if (m == minor_.end()) continue;
            if (!m->second.s0) { m->second.s0 = t; m->second.standAt = now[service.vehicleId]; }
            if (!m->second.s1 && state.tick > service.since + service.ticks) m->second.s1 = t;
        }
        if (nearest) minor_[nearestId].trace.push_back({t, nearestDistance, nearestSpeed});
        blocked_.push_back({t, blocked});
        previous_ = std::move(now);
    }
    std::vector<Pair> pairs() const {
        std::vector<std::pair<double, const Minor*>> crossings;
        for (const auto& [id, m] : minor_) if (m.crossed) crossings.push_back({*m.crossed, &m});
        std::sort(crossings.begin(), crossings.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
        std::vector<Pair> out;
        for (std::size_t k = 1; k < crossings.size(); ++k) {
            const double tl = crossings[k - 1].first, tf = crossings[k].first;
            const auto& f = *crossings[k].second;
            if (tl < warmup_ || tf > end_ || !f.stood || *f.stood > tl || !f.s0 || !f.s1 || *f.s0 < tl) continue;
            if (std::any_of(major_.begin(), major_.end(), [&](double m) { return m > tl && m < tf; })) continue;
            double blockedTime = 0, dt = blocked_.size() > 1 ? blocked_[1].first - blocked_[0].first : 0;
            for (const auto& [time, b] : blocked_) if (b && time >= *f.s1 - 1e-9 && time < tf) blockedTime += dt;
            out.push_back({phases(tl, *f.s0, *f.s1, tf, blockedTime), approach(f.trace, tl, *f.s0), f.waitAt - f.standAt});
        }
        return out;
    }
private:
    struct Minor { std::optional<double> stood, s0, s1, crossed; double standAt{}, waitAt{}; std::vector<Sample> trace; };
    static double cross(double t, double dt, double from, double to, double at) { return to > from ? t - dt + dt * (at - from) / (to - from) : t; }
    double warmup_{}, end_{}, beginSpeed_{};
    std::map<std::uint64_t, Minor> minor_;
    std::vector<double> major_;
    std::vector<std::pair<double, bool>> blocked_;
    std::map<std::uint64_t, double> previous_;
};
}
