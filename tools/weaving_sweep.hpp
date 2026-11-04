#pragma once
// D449: one run of the weaving lab (docs/reference/WEAVING_FIXTURE.md §4-§5) with its demand scaled
// and the lateral switch set on an owned catalog copy -- the run a data directory carrying the keys
// would make (the keys must be there when compiling: the cross-sections are built then). Shared by multra-weaving-sweep and `weavinglab`.
// Development evidence only: no calibration, no M6 validity.
#include "lane_change_sweep.hpp"
#include "../src/commands/catalog_commands.hpp"
#include "../src/core/lateral.hpp"
#include "../src/project/demand_catalog.hpp"
#include <algorithm>
#include <map>

namespace multra::sweep {
struct WeavingRun {
    MovementReport movements;
    LaneChangeReport changes;
    bool overlap{};          // two bodies overlapped in 2D after some tick
    std::uint64_t removed{}; // vehicles that left the run without arriving: always 0 (M3 §4)
    std::map<std::string, double> lastArrival; // by authored route id: when a scene last served
};
// Which part of the lab runs: every input, or only those of the named authored routes, over the
// file's 900 s or a shorter run (inputs end with it).
struct WeavingOptions {
    double factor{1};
    bool lateral{};
    std::vector<std::string> routes;
    std::optional<double> duration;
};
// "route-12/lane-2" -> "route-12": the authored route a compiled lane route came from.
inline std::string authoredRoute(const std::string& id) { return id.substr(0, id.find("/lane-")); }
// The audit's keys (lateral-run-audit): 0.5 m standing, 1.0 m at 50 km/h, 1 m/s.
inline LateralParameters weavingKeys() {
    LateralParameters p;
    p.lateralMinDistanceStanding = 0.5; p.lateralMinDistanceAt50 = 1.0; p.maxLateralSpeed = 1;
    return p;
}
// Two bodies sharing a lane section overlap along it and across it (the lateral rows' test).
inline bool overlaps2d(const SimState& state) {
    const auto spans = occupiedSpans(*state.scenario, state.vehicles, *state.index,
                                     resolveRefs(*state.scenario, state.vehicles, *state.index));
    for (std::size_t i = 0; i < spans.size(); ++i)
        for (std::size_t j = i + 1; j < spans.size(); ++j) {
            const auto& a = spans[i]; const auto& b = spans[j];
            if (a.vehicleId == b.vehicleId || a.segmentIndex != b.segmentIndex) continue;
            if (a.rear < b.front && b.rear < a.front && a.right < b.left && b.right < a.left) return true;
        }
    return false;
}
// `factor` scales every compiled input's volume; `lateral` switches the duration on.
inline WeavingRun runWeaving(ProjectDocument document, const std::filesystem::path& data, std::uint32_t seed,
                             const WeavingOptions& o) {
    if (o.lateral) {
        auto catalog = resolveDemandCatalog(document.definition.value_or(AuthoringDefinition{}), data);
        for (auto& b : catalog.behaviours) b.lateral = weavingKeys();
        putDemandCatalog(document, catalog);
    }
    auto snapshot = compileDocument(document, data);
    auto& inputs = snapshot.scenario.inputs;
    if (!o.routes.empty())
        std::erase_if(inputs, [&](const auto& i) { return std::find(o.routes.begin(), o.routes.end(), authoredRoute(i.routeId)) == o.routes.end(); });
    for (auto& input : inputs) input.vehiclesPerHour *= o.factor;
    if (o.duration) {
        snapshot.scenario.duration = *o.duration;
        for (auto& input : inputs) input.endTime = std::min(input.endTime, *o.duration);
    }
    const auto spec = evaluationSpec(document, snapshot, data);
    MovementAccumulator m(spec);
    LaneChangeAccumulator c(spec);
    WeavingRun run;
    auto s = createSimulation(snapshot.scenario, seed);
    m.observe(s); c.observe(s);
    const auto ticks = totalTicks(snapshot.scenario);
    while (s.tick < ticks) {
        const auto before = s.vehicles.size();
        s = stepSimulation(std::move(s));
        m.observe(s); c.observe(s);
        std::uint64_t in = 0, out = 0;
        for (const auto& e : s.events) {
            if (const auto* a = std::get_if<ArrivedEvent>(&e)) { ++out; run.lastArrival[authoredRoute(a->routeId)] = a->time; }
            if (std::holds_alternative<DepartedEvent>(e)) ++in;
        }
        if (s.vehicles.size() + out != before + in) ++run.removed;
        run.overlap = run.overlap || overlaps2d(s);
    }
    run.movements = m.report(s);
    run.changes = c.report();
    return run;
}
}
