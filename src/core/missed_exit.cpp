#include "missed_exit.hpp"
#include "lanes.hpp"
#include <cmath>

namespace multra {
std::vector<ValidationIssue> missedExitIssues(const DriverBehaviour& b, const std::string& path) {
    if (b.missedExitAfter && !(std::isfinite(*b.missedExitAfter) && *b.missedExitAfter > 0 && *b.missedExitAfter <= 3600))
        return {{"INVALID_BEHAVIOUR_PARAMETER", path + ".missedExitAfter"}};
    return {};
}
std::optional<std::uint32_t> missedExitRoute(const Scenario& s, const ScenarioIndex& index, std::size_t route, double distance) {
    const auto& own = index.parts[route];
    if (own.empty()) return std::nullopt;
    std::size_t k = 0; // the part holding the front
    while (k + 1 < own.size() && distance >= own[k + 1].start - 1e-9) ++k;
    std::optional<std::uint32_t> best;
    std::size_t bestShared = 0;
    for (std::size_t r = 0; r < s.routes.size(); ++r) {
        if (r == route || index.remainingOfRoute[r] != 0) continue;
        const auto& other = index.parts[r];
        std::size_t shared = 0;
        while (shared < own.size() && shared < other.size() && own[shared].segmentIndex == other[shared].segmentIndex) ++shared;
        if (shared <= k) continue;
        if (!best || shared > bestShared || (shared == bestShared && s.routes[r].id < s.routes[*best].id)) {
            best = static_cast<std::uint32_t>(r); bestShared = shared;
        }
    }
    return best;
}
bool applyMissedExits(const Scenario& s, const ScenarioIndex& index, std::vector<Vehicle>& vehicles,
                      const std::vector<VehicleRefs>& refs, std::uint64_t tick, double time, std::vector<SimEvent>& events) {
    bool diverted = false;
    for (std::size_t v = 0; v < vehicles.size(); ++v) {
        auto& x = vehicles[v];
        const auto limit = index.missedExitTicks[refs[v].behaviour];
        if (limit == 0) continue;
        const bool changed = x.lastLaneChange && x.lastLaneChange->tick == tick;
        if (changed || x.lateralTarget || x.routeIndex != refs[v].route ||
            !waitingAtDeadEnd(index, x.routeIndex, x, s.behaviours[refs[v].behaviour])) {
            x.deadEndTicks = 0; continue;
        }
        if (++x.deadEndTicks < limit) continue;
        const auto target = missedExitRoute(s, index, x.routeIndex, x.distance);
        if (!target) continue; // LL6: it keeps waiting, and tries again next tick
        events.emplace_back(MissedExitEvent{time, x.id, s.routes[x.routeIndex].id, s.routes[*target].id});
        if (!x.laneChangeTrace.empty()) x.routeRemapTrace.push_back({x.routeIndex, *target, x.distance, x.laneChangeTrace.size()});
        x.routeIndex = *target; x.deadEndTicks = 0; diverted = true;
    }
    return diverted;
}
}
