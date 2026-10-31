#pragma once
#include "../src/core/simulation.hpp"
#include "../src/core/validate.hpp"
#include "../src/project/load.hpp"
#include "../src/project/json.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>
#include <functional>
#include <stdexcept>

namespace test {
struct Case { std::string group, name; std::function<void()> run; };
std::vector<Case>& cases();
struct Register {
    Register(const char* group, const char* name, std::function<void()> run) {
        cases().push_back({group, name, std::move(run)});
    }
};
inline void require(bool value, const char* expression, const char* file, int line) {
    if (!value) throw std::runtime_error(std::string(file) + ":" + std::to_string(line) + ": " + expression);
}
inline void near(double a, double b, double tolerance = 1e-7) {
    if (!std::isfinite(a) || !std::isfinite(b) || std::abs(a - b) > tolerance)
        throw std::runtime_error("Numeric mismatch: " + std::to_string(a) + " vs " + std::to_string(b));
}
template<class F> void throws(F action, const std::string& message = {}) {
    try { action(); } catch (const std::exception& error) {
        if (std::string(error.what()).find(message) == std::string::npos)
            throw std::runtime_error("Wrong exception: " + std::string(error.what()));
        return;
    }
    throw std::runtime_error("Expected an exception: " + message);
}
inline std::filesystem::path root() { return MULTRA_SOURCE_DIR; }
inline multra::LoadedScenario demo() {
    auto loaded = multra::loadScenario(root() / "data/scenarios/crossing.json", root() / "data");
    // The historical crossing/physics fixture keeps its original car/heavy pair: manual
    // placements use slot 0 as car and the frozen trajectories describe those two types.
    // Tests of the installed catalog loadScenario directly so additions remain covered.
    std::erase_if(loaded.scenario.vehicleTypes, [](const auto& type) { return type.id != "car" && type.id != "heavy-vehicle"; });
    return loaded;
}
inline multra::Scenario straight() {
    const auto source = demo().scenario;
    multra::Scenario s;
    s.duration = 120; s.timeStep = 0.1;
    s.segments = {{"road", 150, {}}}; s.routes = {{"route", {"road"}}};
    s.vehicleTypes = source.vehicleTypes; s.behaviours = source.behaviours;
    s.inputs = {{"input", "route", "car", 900, 0, 60}};
    return s;
}
// A vehicle to place on the network by hand. It names its route and type by ID, because that is
// what a test can read; withVehicles turns those into the canonical scenario's slots, which is
// the only place the two representations meet. Placed vehicles come from no input, so they carry
// PendingVehicle::kNoInput and nothing erases a queue entry for them.
struct Placement {
    std::uint64_t id{}; std::string routeId; double distance{}, speed{};
    std::string vehicleTypeId{"car"};
};
inline Placement vehicle(std::uint64_t id, double distance, double speed = 0) {
    return {id, "route", distance, speed};
}
inline multra::SimState withVehicles(multra::Scenario s, const std::vector<Placement>& placements) {
    s.inputs.clear(); auto state = multra::createSimulation(s, 42);
    // createSimulation canonicalises, so resolve against the scenario the state actually holds.
    const auto slot = [](const auto& items, const std::string& id) {
        for (std::uint32_t i = 0; i < items.size(); ++i) if (items[i].id == id) return i;
        throw std::invalid_argument("Unknown test id: " + id);
    };
    for (const auto& p : placements) {
        multra::Vehicle v;
        v.id = p.id; v.distance = p.distance; v.speed = p.speed;
        v.routeIndex = slot(state.scenario->routes, p.routeId);
        v.typeIndex = slot(state.scenario->vehicleTypes, p.vehicleTypeId);
        v.desiredSpeed = 15; v.driverFactor = 0.5;
        state.vehicles.push_back(v);
        state.nextVehicleId = std::max(state.nextVehicleId, v.id + 1);
    }
    return state;
}
inline multra::SimState finish(multra::SimState state) {
    while (state.tick < multra::totalTicks(*state.scenario)) state = multra::stepSimulation(state);
    return state;
}
// A saved document as a file of an older schema holds it: below 59 the street-segment keys are
// `roads`, `roadId` and `roadDirection` (D389). Only the version and those keys change.
inline multra::Json atSchema(multra::Json j, int version) {
    j["schemaVersion"] = version;
    if (version >= 59) return j;
    const auto rename = [](multra::Json& o, const char* from, const char* to) {
        if (o.is_object() && o.contains(from)) { o[to] = o.at(from); o.erase(from); }
    };
    rename(j["network"], "streetSegments", "roads");
    for (auto& l : j["network"]["links"]) rename(l, "streetSegmentId", "roadId");
    if (j.contains("zoneConnectors"))
        for (auto& c : j["zoneConnectors"]) { rename(c, "streetSegmentId", "roadId"); rename(c, "streetSegmentDirection", "roadDirection"); }
    return j;
}
}
#define CHECK(...) test::require(static_cast<bool>((__VA_ARGS__)), #__VA_ARGS__, __FILE__, __LINE__)
#define TEST(group, name) static void group##_##name(); \
    static test::Register register_##group##_##name(#group, #name, group##_##name); \
    static void group##_##name()
