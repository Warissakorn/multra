#include "lab_run.hpp"
#include "../core/routes.hpp"
#include "../core/validate.hpp"
#include "../model/demand/signal_control.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace trafficsim::lab {
namespace {
double lapLength(const Scenario& s, const Route& route) {
    double length = 0;
    for (const auto& id : route.segmentIds)
        for (const auto& segment : s.segments) if (segment.id == id) length += segment.length;
    return length;
}
// Whether the change needs new vehicles: the road (and so the segments) or who is on it.
bool restartNeeded(const LabSettings& a, const LabSettings& b) {
    auto ra = a.road, rb = b.road;
    ra.green = rb.green; ra.red = rb.red; ra.amber = rb.amber; // timings retune
    return !(ra == rb) || a.traffic.vehicleCount != b.traffic.vehicleCount ||
           a.traffic.heavyShare != b.traffic.heavyShare;
}
double shortestLap(const LabSettings& settings) {
    // The inner lane is the shortest. Wherever the reference line lies in the bundle, no lane's
    // curve radius is below R - lanes x width, so this never overstates the room.
    const auto& road = settings.road;
    return 2 * road.straightLength + 2 * std::numbers::pi * (road.curveRadius - road.lanes * road.laneWidth);
}
}
int maxVehicles(const LabSettings& settings) {
    const double body = std::max({settings.focus.type.length, settings.traffic.car.length, settings.traffic.heavy.length});
    const double gap = std::max(settings.focus.behaviour.standstillDistance, settings.traffic.behaviour.standstillDistance);
    return std::max(1, static_cast<int>(std::floor(shortestLap(settings) / (body + gap + 1)))) * settings.road.lanes;
}
LabRun::LabRun(LabSettings settings, std::filesystem::path dataDirectory, std::uint32_t seed)
    : settings_(std::move(settings)), data_(std::move(dataDirectory)), seed_(seed) {
    start(compile(settings_), settings_);
}
LabRun::Compiled LabRun::compile(const LabSettings& settings) const {
    if (settings.traffic.vehicleCount < 0 || settings.traffic.vehicleCount + 1 > maxVehicles(settings) ||
        !(settings.traffic.heavyShare >= 0 && settings.traffic.heavyShare <= 1))
        throw std::invalid_argument("LAB_TOO_MANY_VEHICLES");
    const auto document = buildLabDocument(settings);
    assertValidNetwork(document.network);
    // compileDocument's own steps that this document needs: the controller becomes a program and
    // the Route expands into one chain per lane. Its catalog is owned, so nothing is resolved.
    ScenarioDefinition definition = *document.definition;
    for (auto& program : signalGroupPrograms(document.definition->signalControllers))
        definition.signalPrograms.push_back(std::move(program));
    Compiled snapshot{document, buildScenario(document.network, definition)};
    // Every compiled lane chain of the one ring Route closes on itself. Spans between the lanes
    // would read route distances without a wrap rule (BEHAVIOUR_LAB.md §1.1): no lane changes.
    for (auto& route : snapshot.scenario.routes) route.circuit = true;
    snapshot.scenario.laneChanges.clear();
    snapshot.scenario.routeDeadEnds.clear();
    assertValidScenario(snapshot.scenario);
    return snapshot;
}
void LabRun::start(Compiled compiled, const LabSettings& settings) {
    auto state = createSimulation(compiled.scenario, seed_);
    // Lanes in route order; the focus vehicle is first on the first lane, so it has the lowest id.
    const auto& scenario = *state.scenario;
    const auto lanes = scenario.routes.size();
    const int traffic = settings.traffic.vehicleCount;
    std::vector<int> perLane(lanes, 0);
    for (int i = 0; i < traffic + 1; ++i) ++perLane[static_cast<std::size_t>(i) % lanes];
    std::vector<int> placed(lanes, 0);
    int heavySoFar = 0;
    for (int i = 0; i < traffic + 1; ++i) {
        const auto lane = static_cast<std::size_t>(i) % lanes;
        const auto& route = scenario.routes[lane];
        const double lap = lapLength(scenario, route);
        // Slot j of n at L + j L / n: one lap round, every vehicle in [L, 2L) as a circuit keeps it.
        const double distance = lap + lap * placed[lane]++ / perLane[lane];
        std::string type = kFocusType;
        if (i > 0) {
            // Evenly spread heavies: vehicle k is heavy when the running share first passes a whole one.
            const int heavyAfter = static_cast<int>(std::floor(i * settings.traffic.heavyShare + 1e-9));
            type = heavyAfter > heavySoFar ? kHeavyType : kCarType;
            heavySoFar = heavyAfter;
        }
        state = placeVehicle(std::move(state), {route.id, type, distance, 0});
    }
    // Nothing above changed the run, so a refused placement keeps the current one (apply's promise).
    settings_ = settings;
    focusId_ = state.vehicles.front().id;
    state_ = std::move(state);
    document_ = std::move(compiled.document);
    ++networkRevision_;
    clamps_ = 0; laps_ = 0;
    lastFocusDistance_ = state_.vehicles.front().distance;
}
LabRun::Applied LabRun::apply(const LabSettings& settings) {
    if (settings == settings_) return Applied::unchanged;
    auto compiled = compile(settings); // throws before anything changes
    const bool fresh = restartNeeded(settings_, settings);
    if (!fresh) {
        try {
            state_ = retuneSimulation(std::move(state_), compiled.scenario, {settings.rescaleDesiredSpeeds});
            settings_ = settings;
            return Applied::retuned;
        } catch (const std::invalid_argument& error) {
            if (std::string(error.what()).find("RETUNE_SHAPE_CHANGED") == std::string::npos) throw;
        }
    }
    start(std::move(compiled), settings);
    return Applied::restarted;
}
void LabRun::restart() { start(compile(settings_), settings_); }
void LabRun::restart(std::uint32_t seed) { seed_ = seed; restart(); }
void LabRun::step() {
    if (state_.tick >= totalTicks(*state_.scenario)) { restart(); return; }
    state_ = stepSimulation(std::move(state_));
    for (const auto& event : state_.events)
        if (std::holds_alternative<SafetyClampEvent>(event)) ++clamps_;
    const auto focus = std::lower_bound(state_.vehicles.begin(), state_.vehicles.end(), focusId_,
                                        [](const Vehicle& v, std::uint64_t id) { return v.id < id; });
    if (focus != state_.vehicles.end() && focus->id == focusId_) {
        if (focus->distance < lastFocusDistance_) ++laps_; // dropped back one lap at the seam
        lastFocusDistance_ = focus->distance;
    }
}
std::optional<FocusTelemetry> LabRun::telemetry(std::uint64_t vehicleId) const {
    const auto& s = *state_.scenario;
    const auto& index = *state_.index;
    const auto found = std::find_if(state_.vehicles.begin(), state_.vehicles.end(),
                                    [&](const Vehicle& v) { return v.id == vehicleId; });
    if (found == state_.vehicles.end()) return std::nullopt;
    const auto& v = *found;
    FocusTelemetry t{v.id, state_.time, v.speed, v.acceleration, v.desiredSpeed, v.distance, v.mode, {}, {},
                     vehicleId == focusId_ ? laps_ : 0};
    // The nearest rear bumper ahead on this vehicle's own parts: the engine's closestVehicle, read
    // from the same spans. Three laps of parts, so a leader just past the seam is found.
    const auto refs = resolveRefs(s, state_.vehicles, index);
    const auto spans = occupiedSpans(s, state_.vehicles, index, refs);
    for (const auto& part : index.parts[v.routeIndex]) {
        if (part.start + part.length < v.distance) continue;
        if (t.gap && part.start - v.distance >= *t.gap) break;
        for (const auto& span : spans) {
            if (span.segmentIndex != part.segmentIndex || span.vehicleId == v.id ||
                part.start + span.front < v.distance - 1e-9) continue;
            const double gap = part.start + span.rear - v.distance;
            if (!t.gap || gap < *t.gap) { t.gap = gap; t.leaderSpeed = span.speed; }
        }
    }
    return t;
}
}
