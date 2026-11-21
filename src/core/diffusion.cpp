#include "diffusion.hpp"
#include "lanes.hpp"
#include <cmath>

namespace multra {
std::vector<ValidationIssue> diffusionIssues(const DriverBehaviour& b, const std::string& path) {
    if (b.diffusionTime && !(std::isfinite(*b.diffusionTime) && *b.diffusionTime > 0 && *b.diffusionTime <= 3600))
        return {{"INVALID_BEHAVIOUR_PARAMETER", path + ".diffusionTime"}};
    return {};
}
bool applyDiffusion(const Scenario& s, const ScenarioIndex& index, std::vector<Vehicle>& vehicles,
                    const std::vector<VehicleRefs>& refs, std::uint64_t tick, double time, std::vector<SimEvent>& events) {
    std::vector<bool> gone(vehicles.size(), false);
    bool any = false;
    for (std::size_t v = 0; v < vehicles.size(); ++v) {
        auto& x = vehicles[v];
        const auto limit = index.diffusionTicks[refs[v].behaviour];
        if (limit == 0) continue;
        const bool changed = x.lastLaneChange && x.lastLaneChange->tick == tick;
        if (changed || x.lateralTarget || x.routeIndex != refs[v].route ||
            !waitingAtDeadEnd(index, x.routeIndex, x, s.behaviours[refs[v].behaviour])) {
            x.diffusionWait = 0; continue;
        }
        if (++x.diffusionWait < limit) continue;
        events.emplace_back(DiffusedEvent{time, x.id, s.routes[x.routeIndex].id});
        gone[v] = true; any = true;
    }
    if (!any) return false;
    std::size_t keep = 0;
    for (std::size_t v = 0; v < vehicles.size(); ++v) if (!gone[v]) { if (keep != v) vehicles[keep] = std::move(vehicles[v]); ++keep; }
    vehicles.resize(keep);
    return true;
}
}
