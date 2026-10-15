#pragma once
// Shared by the lateral span and motion tests (M3.7, docs/reference/LATERAL.md): a straight road of
// owned catalogs with lateral keys, vehicles placed by segment, and the 2D overlap check.
#include "test.hpp"
#include "../src/commands/catalog_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/core/lateral.hpp"
#include "../src/project/demand_catalog.hpp"
#include "../src/project/run.hpp"
#include <algorithm>
#include <tuple>

namespace lateral_test {
namespace { // internal linkage: each test file gets its own copy
using namespace trafficsim;
LateralParameters keys(double standing, double at50) {
    LateralParameters p;
    p.lateralMinDistanceStanding = standing; p.lateralMinDistanceAt50 = at50; p.maxLateralSpeed = 1;
    return p;
}
// One straight Link of `lanes` lanes of `width`, a route on its first lane, owned catalogs from data/
// with every behaviour carrying `p`, compiled.
Scenario road(int lanes, double width, const LateralParameters& p, std::vector<Point> geometry = {{0, 0}, {300, 0}}) {
    ProjectDocument d;
    const auto link = addLink(d, geometry, lanes, width);
    const auto route = putRoute(d, {"route", {link}}); changeRunSettings(d, 60, .1);
    auto catalog = resolveDemandCatalog(AuthoringDefinition{}, test::root() / "data");
    for (auto& b : catalog.behaviours) { b.amberDeceleration.reset(); b.lateral = p; }
    putDemandCatalog(d, catalog);
    putInput(d, {"in", route, "car", 600, 0, 60});
    validateDocument(d);
    auto s = compileDocument(d, test::root() / "data").scenario;
    s.inputs.clear();
    return s;
}
std::size_t slot(const auto& items, const std::string& id) {
    for (std::size_t i = 0; i < items.size(); ++i) if (items[i].id == id) return i;
    throw std::runtime_error("no " + id);
}
// Vehicles placed, ids ascending, on the route that starts on the named segment (createSimulation
// re-sorts routes, so the slot is resolved on its canonical copy).
SimState place(const Scenario& s, const std::vector<std::tuple<std::string, double, double, double>>& at) {
    auto state = createSimulation(s, 42);
    std::uint64_t id = 1;
    for (const auto& [segment, distance, speed, lateral] : at) {
        const auto& routes = state.scenario->routes;
        const auto route = std::find_if(routes.begin(), routes.end(), [&](const auto& r) { return r.segmentIds.front() == segment; });
        Vehicle v; v.id = id++; v.routeIndex = static_cast<std::uint32_t>(route - routes.begin());
        v.typeIndex = static_cast<std::uint32_t>(slot(state.scenario->vehicleTypes, "car"));
        v.distance = distance; v.speed = speed; v.lateral = lateral; v.desiredSpeed = 15; v.driverFactor = .5;
        state.vehicles.push_back(v);
    }
    state.nextVehicleId = id;
    return state;
}
std::vector<OccupiedSpan> spansOf(const SimState& state) {
    return occupiedSpans(*state.scenario, state.vehicles, *state.index,
                         resolveRefs(*state.scenario, state.vehicles, *state.index));
}
// The route slot whose single segment is the Link's lane `k` (lane order of the Link).
std::size_t laneSegment(const Scenario& s, std::size_t k) {
    std::vector<std::pair<double, std::size_t>> byCentre;
    for (std::size_t i = 0; i < s.segments.size(); ++i) byCentre.emplace_back(-s.segmentCrossSections[i].centre, i);
    std::sort(byCentre.begin(), byCentre.end());
    return byCentre.at(k).second;
}
// Two bodies overlap in two dimensions when they share a carriageway, overlap along it and across it.
bool overlap2d(const SimState& state) {
    const auto spans = spansOf(state);
    for (std::size_t i = 0; i < spans.size(); ++i)
        for (std::size_t j = i + 1; j < spans.size(); ++j) {
            const auto& a = spans[i]; const auto& b = spans[j];
            if (a.vehicleId == b.vehicleId || a.segmentIndex != b.segmentIndex) continue;
            if (a.rear < b.front && b.rear < a.front && a.right < b.left && b.right < a.left) return true;
        }
    return false;
}
}
}
