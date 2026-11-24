#include "test.hpp"
#include "lane_fixture.hpp"
#include "../src/core/conflicts.hpp"
#include "../src/core/following.hpp"
#include "../src/core/lanes.hpp"
#include "../src/core/routes.hpp"
#include "../src/core/w74.hpp"
#include "../src/core/w99.hpp"
#include <algorithm>
#include <cmath>
#include <map>
using namespace multra;
// M3.3.3b (D642): W99 composed into the tick -- rows BA226, BA230 and BA232 of
// docs/plans/DRIVING_BEHAVIOUR.md against docs/reference/W99.md §6-§7, on hand-built scenarios.
namespace {
const W99Parameters kP{1.5, 1.3, 4, -12, -0.25, 0.35, 6, 0.25, 2, 1.5}; // W99 §5's fixture values
const W74Parameters kW74{.ax = 2, .bxAdd = 2, .bxMult = 1, .exAdd = 2, .exMult = 0, .cxAdd = 16,
    .cxMult = 0, .opdvAdd = 0.5, .opdvMult = 0, .dMax = 20, .bMaxAdd = 1, .bMaxMult = 1,
    .bMaxSpeedRoot = 3, .bNullAdd = 0.25, .bNullMult = 0, .bMinAdd = -1,
    .leaderAccelerationWeight = 0.5, .emergencyLeaderWeight = 0.25};
DriverBehaviour w99(const std::string& id) { DriverBehaviour b{id}; b.w99 = kP; return b; }
DriverBehaviour w74(const std::string& id) { DriverBehaviour b{id}; b.w74 = kW74; return b; }
DriverBehaviour prototype(const std::string& id) { return {id, 2, 2, 3, 1.5, .2}; }
// One route over the given segments; "car" uses w99, "truck" the prototype, "truckW74" w74.
Scenario road(std::vector<std::string> segments, double length = 50) {
    Scenario s; s.duration = 600; s.timeStep = 0.1;
    for (std::size_t i = 0; i < segments.size(); ++i)
        s.segments.push_back({segments[i], length, i + 1 < segments.size() ? std::vector<std::string>{segments[i + 1]} : std::vector<std::string>{}});
    s.routes = {{"route", segments}};
    s.behaviours = {w99("w99"), w74("w74"), prototype("proto")};
    s.vehicleTypes = {{"car", 4, 2, {15, 15}, 3, 2, 6, "w99"}, {"truck", 4, 2, {15, 15}, 3, 2, 6, "proto"},
                      {"truckW74", 4, 2, {15, 15}, 3, 2, 6, "w74"}};
    return s;
}
// Hand-placed vehicles with the traits generation would have given them (D137, D641).
void traits(SimState& state) {
    for (auto& v : state.vehicles) {
        v.w99Trait = w99Trait(state.seed, v.id);
        if (state.index->w74) v.w74Traits = w74Traits(state.seed, v.id, v.driverFactor);
    }
}
struct Placed { std::uint64_t id; double distance, speed, acceleration{}; std::string type{"car"};
                std::optional<W74State> state{}; };
SimState placed(const Scenario& s, const std::vector<Placed>& list) {
    std::vector<test::Placement> placements;
    for (const auto& p : list) placements.push_back({p.id, "route", p.distance, p.speed, p.type});
    auto state = test::withVehicles(s, placements);
    traits(state);
    for (auto& v : state.vehicles)
        for (const auto& p : list) if (p.id == v.id) { v.acceleration = p.acceleration; v.w74State = p.state; }
    return state;
}
const Vehicle& find(const SimState& s, std::uint64_t id) {
    for (const auto& v : s.vehicles) if (v.id == id) return v;
    throw std::runtime_error("vehicle left");
}
bool clamped(const SimState& s) {
    return std::any_of(s.events.begin(), s.events.end(), [](const auto& e) { return std::holds_alternative<SafetyClampEvent>(e); });
}
}

TEST(w99run, the_accessors_are_cc0_and_sdxc) { // BA230, W99 §6
    const auto b = w99("x");
    CHECK(standstillGap(b) == 1.5);
    CHECK(desiredGap(b, 0.9, 10) == 1.5 + 1.3 * 10); CHECK(desiredGap(b, 0.1, 0) == 1.5);
    CHECK(stopLineReach(b, 0.5) == desiredGap(b, 0.5, 0.1) + 0.1);
}

TEST(w99run, a_w99_vehicle_at_rest_in_the_following_band_creeps_and_serves_a_stop_line) { // BA226
    const auto source = test::demo().scenario;
    Scenario s; s.duration = 300; s.timeStep = 0.1;
    s.segments = {{"eastUp", 50, {"east"}}, {"east", 200, {}}, {"north", 200, {}}};
    s.routes = {{"majorRoute", {"eastUp", "east"}}, {"minorRoute", {"north"}}};
    s.vehicleTypes = source.vehicleTypes; s.behaviours = {w99("w99")};
    for (auto& t : s.vehicleTypes) t.behaviourId = "w99";
    s.conflictZones = {{"zone", {{"east"}, 98, 102}, {{"north"}, 98, 102}, 90, 3, 10, ZoneControl::stop}};
    const auto serve = [&](double distance, double speed, double aPrev) {
        auto state = test::withVehicles(s, {{1, "minorRoute", distance, speed}});
        traits(state); state.vehicles.front().acceleration = aPrev;
        int atLine = 0; bool crossed = false;
        for (int t = 0; t < 3000 && !crossed; ++t) {
            const auto& v = state.vehicles.front();
            if (v.distance > 90 + 1e-9) crossed = true;
            else if (v.speed == 0 && 90 - v.distance <= stopLineReach(s.behaviours.front(), v.driverFactor)) ++atLine;
            state = stepSimulation(state);
        }
        return std::pair{crossed, atLine};
    };
    const auto [arrived, arrivedAtLine] = serve(40, 10, 0);
    CHECK(arrived); CHECK(arrivedAtLine >= 2);
    // §4's case: at rest 3.3 m short, inside (SDXc, SDXo) = (1.5, 5.5) and beyond stopLineReach, after
    // braking (aPrev < 0). Without the standstill creep it would keep aPrev and never be served.
    CHECK(90 - 86.7 > stopLineReach(s.behaviours.front(), 0.5));
    const auto [crept, creptAtLine] = serve(86.7, 0, -0.3);
    CHECK(crept); CHECK(creptAtLine >= 2);
}

TEST(w99run, behind_a_red_head_the_creep_stops_within_sdxc_and_d105_holds_it) { // BA226
    auto red = road({"a", "b", "c", "d"});
    red.signalPrograms = {{"p", 0, {{600, SignalColor::red}}}};
    red.signalHeads = {{"h", "c", 1, "p"}};
    auto state = placed(red, {{1, 101 - 3.3, 0, -0.3}}); // the front 3.3 m short of the head at 101
    for (int i = 0; i < 600; ++i) state = stepSimulation(state);
    const auto& v = find(state, 1);
    CHECK(v.speed == 0); CHECK(v.distance <= 101 + 1e-9); CHECK(101 - v.distance <= 1.5 + 1e-9);
    CHECK(v.distance > 101 - 3.3); // it did creep
}

TEST(w99run, the_hard_cap_source_rule_and_d105_hold_for_w99) { // BA230
    auto red = road({"a", "b", "c", "d"});
    red.signalPrograms = {{"p", 0, {{600, SignalColor::red}}}};
    red.signalHeads = {{"h", "c", 1, "p"}};
    auto state = stepSimulation(placed(red, {{1, 100, 15}}));
    CHECK(clamped(state)); CHECK(find(state, 1).distance <= 101 + 1e-9); CHECK(find(state, 1).speed == 0);
    // D105: at rest within cc0 of a leader pulling away, no positive acceleration.
    for (const double gap : {1.5, 1.0}) {
        const auto next = stepSimulation(placed(red, {{1, 96 - gap, 0}, {2, 100, 5, 2, "truck"}}));
        CHECK(find(next, 1).acceleration <= 0); CHECK(find(next, 1).speed == 0);
    }
    // Source (D108 with cc0): a w99 input does not depart while the entry vehicle's rear is within cc0.
    auto source = road({"a", "b"}); source.duration = 1;
    source.inputs = {{"in", "route", "car", 3600, 0, 1}};
    auto s = createSimulation(source, 42);
    Vehicle blocker; blocker.id = 99; blocker.typeIndex = 1; blocker.distance = 5.4; blocker.desiredSpeed = 0;
    s.vehicles.push_back(blocker); s.nextVehicleId = 100;
    for (int i = 0; i < 10; ++i) {
        s = stepSimulation(s);
        CHECK(std::none_of(s.events.begin(), s.events.end(), [](const auto& e) { return std::holds_alternative<DepartedEvent>(e); }));
    }
}

TEST(w99run, a_signalised_queue_discharges_and_an_open_platoon_never_clamps) { // BA230
    const auto run = [](const Scenario& scenario, bool expectNoClamp) {
        auto state = createSimulation(scenario, 42);
        std::size_t following = 0;
        while (state.tick < totalTicks(*state.scenario)) {
            state = stepSimulation(std::move(state));
            if (expectNoClamp) CHECK(!clamped(state));
            for (const auto& v : state.vehicles) {
                CHECK(v.w99Trait.has_value()); CHECK(!v.w74State);
                if (v.mode == FollowingMode::following) ++following;
            }
        }
        return std::pair{following, state.completed};
    };
    auto signalised = road({"main"}, 400);
    signalised.inputs = {{"in", "route", "car", 900, 0, 300}};
    signalised.signalPrograms = {{"p", 0, {{60, SignalColor::red}, {540, SignalColor::green}}}};
    signalised.signalHeads = {{"h", "main", 300, "p"}};
    const auto [queueFollowing, discharged] = run(signalised, false);
    CHECK(discharged > 50);
    auto open = road({"main"}, 2000);
    open.inputs = {{"in", "route", "car", 600, 0, 600}};
    const auto [platoonFollowing, completed] = run(open, true);
    CHECK(completed > 0); CHECK(platoonFollowing + queueFollowing > 0);
}

TEST(w99run, multi_anticipation_and_reaction_time_compose_with_w99) { // BA230, PERCEPTION §3, §5
    // Two vehicles ahead: the near one at speed, the far one standing. Seeing two can only brake more.
    auto one = road({"a"}, 1000), two = one;
    two.behaviours[0].perception.observedVehiclesAhead = 2;
    const std::vector<Placed> list{{1, 100, 10}, {2, 130, 10, 0, "truck"}, {3, 140, 0, 0, "truck"}};
    const auto a1 = find(stepSimulation(placed(one, list)), 1).acceleration, a2 = find(stepSimulation(placed(two, list)), 1).acceleration;
    CHECK(a2 < a1);
    // reactionTime: the published acceleration is the applied FIFO entry, which is next tick's aPrev.
    auto delayed = road({"a"}, 1000);
    delayed.behaviours[0].perception.reactionTime = 0.3;
    auto state = placed(delayed, {{1, 100, 10}});
    auto& v = state.vehicles.front();
    v.reactionHistory.assign(3, FollowingResult{0.123, FollowingMode::free, std::nullopt});
    CHECK(find(stepSimulation(state), 1).acceleration == 0.123);
}

TEST(w99run, a_courtesy_hold_composes_and_writes_nothing) { // BA230, the second obstacle
    auto lanes = test::lanes();
    for (auto& b : lanes.behaviours) b = w99(b.id);
    std::vector<test::Placement> list{test::on(1, "stubA", 198, 0)};
    for (std::uint64_t k = 0; k < 15; ++k) list.push_back(test::on(10 + k, "full", 190 - 12 * static_cast<double>(k), 10));
    auto s = test::withVehicles(lanes, list);
    traits(s);
    std::size_t won = 0;
    for (int t = 0; t < 600; ++t) {
        const auto refs = resolveRefs(*s.scenario, s.vehicles, *s.index);
        const auto spans = occupiedSpans(*s.scenario, s.vehicles, *s.index, refs);
        const auto holds = courtesyHolds(*s.scenario, *s.index, s.vehicles, refs, spans, bucketSpans(spans, s.scenario->segments.size()));
        std::map<std::uint64_t, double> held;
        const auto before = s.vehicles;
        for (std::size_t v = 0; v < holds.size(); ++v)
            if (std::isfinite(holds[v].gap)) {
                const auto& x = s.vehicles[v];
                held[x.id] = follow(x.speed, x, x.w74State, x.acceleration, s.scenario->timeStep, s.scenario->vehicleTypes[refs[v].type],
                                    s.scenario->behaviours[refs[v].behaviour], Leader{holds[v].gap, holds[v].speed, holds[v].acceleration}).acceleration;
            }
        CHECK(s.vehicles == before); // hypothetical evaluations write nothing
        s = stepSimulation(s);
        for (const auto& v : s.vehicles)
            if (const auto h = held.find(v.id); h != held.end() && v.acceleration == h->second) ++won;
    }
    CHECK(won > 0); // the courtesy obstacle really was the kept result
}

TEST(w99run, model_switches_carry_the_published_acceleration) { // BA232, W99 §7
    // a: proto, b: w99, c: w74, d: w99, e: proto -- one route, the car's own behaviour the prototype.
    auto s = road({"a", "b", "c", "d", "e", "f"});
    s.vehicleTypes[0].behaviourId = "proto";
    s.segmentBehaviours = {{"b", "car", "w99"}, {"c", "car", "w74"}, {"d", "car", "w99"}};
    // A leader 14 m ahead (gap 10) at the same 4 m/s: W99 following (SDXc 6.7 < 10 < SDXo 10.7) keeps aPrev's sign.
    const auto step = [&](double front, double aPrev, std::optional<W74State> before = {}) {
        return find(stepSimulation(placed(s, {{1, front, 4, aPrev, "car", before}, {2, front + 14, 4, 0, "truck"}})), 1);
    };
    for (const double front : {50.0, 150.0}) { // entering b from proto, and d from w74
        for (const double aPrev : {-0.3, 0.3, 0.0}) {
            const auto state = placed(s, {{1, front, 4, aPrev}, {2, front + 14, 4, 0, "truck"}});
            const auto& v = state.vehicles.front().id == 1 ? state.vehicles.front() : state.vehicles.back();
            const auto expected = w99Acceleration(4, v.desiredSpeed, aPrev, s.timeStep, v.driverFactor, *v.w99Trait,
                                                  state.scenario->vehicleTypes[v.typeIndex], kP, Leader{10, 4, 0});
            const auto next = step(front, aPrev, W74State{W74Regime::following, -1});
            CHECK(next.acceleration == expected.acceleration); CHECK(!next.w74State); // leaving w74 clears its state
        }
        CHECK(step(front, -0.3).acceleration == -0.3); CHECK(step(front, 0.3).acceleration == 0.3);
    }
    CHECK(step(100, 0).w74State.has_value()); // on c the W74 state is written again
}

TEST(w99run, a_w99_run_replays_exactly) { // BA230
    auto s = road({"main"}, 400);
    s.duration = 120; s.inputs = {{"in", "route", "car", 900, 0, 120}};
    s.signalPrograms = {{"p", 0, {{30, SignalColor::red}, {90, SignalColor::green}}}};
    s.signalHeads = {{"h", "main", 300, "p"}};
    auto a = createSimulation(s, 42);
    for (int i = 0; i < 400; ++i) a = stepSimulation(a);
    auto b = a;
    while (a.tick < totalTicks(*a.scenario)) {
        a = stepSimulation(a); b = stepSimulation(b);
        CHECK(a.vehicles == b.vehicles); CHECK(a.events == b.events);
    }
    CHECK(test::finish(createSimulation(s, 42)).vehicles == test::finish(createSimulation(s, 42)).vehicles);
}

TEST(w99run, a_queued_car_at_rest_is_never_clamped_and_closes_up_to_cc0) { // BA238, W99 §4 (D650); D649's clamps
    auto red = road({"a", "b", "c", "d"});
    red.signalPrograms = {{"p", 0, {{600, SignalColor::red}}}};
    red.signalHeads = {{"h", "c", 1, "p"}};
    const double cap = kP.cc7 * red.timeStep * red.timeStep;
    const auto room = [](const SimState& s) { return find(s, 2).distance - 4 - find(s, 1).distance - kP.cc0; };
    // D649's case: a w99 car at rest a rounding residue past CC0 behind a prototype truck standing at the
    // red line (101). Before D650 the creep asked +CC7 and the hard cap clamped it every tick.
    auto residue = placed(red, {{1, 101 - 4 - 1.5 - 3e-9, 0}, {2, 101, 0, 0, "truck"}});
    for (int i = 0; i < 600; ++i) { residue = stepSimulation(residue); CHECK(!clamped(residue)); }
    CHECK(find(residue, 1).speed == 0); CHECK(room(residue) > 0); CHECK(room(residue) <= cap);
    // From 3 m past CC0 it still closes up, to rest within CC7·dt² of CC0. (Its arrival may clamp once:
    // the soft closing approach, a question for the line contract, not this hold.)
    auto queued = placed(red, {{1, 101 - 4 - 1.5 - 3, 0}, {2, 101, 0, 0, "truck"}});
    int clamps = 0;
    for (int i = 0; i < 600; ++i) { queued = stepSimulation(queued); clamps += clamped(queued); }
    CHECK(find(queued, 1).speed == 0); CHECK(room(queued) >= -1e-9); CHECK(room(queued) <= cap);
    CHECK(clamps <= 1); // never the every-tick standing clamp
}

TEST(w99run, ba249_an_arrival_behind_a_standing_car_is_never_clamped) { // BA249, W99 §4 (D654)
    // From 5 cm to 3 m past CC0, at 0.1 to 1.5 m/s and accelerating, behind a prototype truck standing at a
    // red line, at dt 0.1, 0.25 and 0.5: no clamp, and the car rests within CC7·dt² of CC0.
    int closeMoves = 0; // forcing: the arrivals do come within 1 cm of CC0 while still moving
    for (const double dt : {0.1, 0.25, 0.5})
        for (const double room0 : {0.05, 0.3, 1.0, 3.0})
            for (const double v0 : {0.1, 0.5, 1.5}) {
                if (v0 * v0 > 2 * 6 * room0) continue; // a stop the type's maximum (6) cannot make
                auto red = road({"a", "b", "c", "d"}); red.timeStep = dt;
                red.signalPrograms = {{"p", 0, {{600, SignalColor::red}}}};
                red.signalHeads = {{"h", "c", 1, "p"}};
                auto s = placed(red, {{1, 101 - 4 - kP.cc0 - room0, v0, 0.4}, {2, 101, 0, 0, "truck"}});
                const auto room = [&] { return find(s, 2).distance - 4 - find(s, 1).distance - kP.cc0; };
                for (int i = 0; i < static_cast<int>(60 / dt); ++i) {
                    s = stepSimulation(s); CHECK(!clamped(s));
                    closeMoves += find(s, 1).speed > 0 && room() < 0.01;
                }
                CHECK(find(s, 1).speed == 0); CHECK(room() >= -1e-9); CHECK(room() <= kP.cc7 * dt * dt + 1e-12);
            }
    CHECK(closeMoves > 0);
}
