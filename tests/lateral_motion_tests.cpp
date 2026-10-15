#include "lateral_test_support.hpp"
#include <cmath>
using namespace trafficsim;
using namespace lateral_test;
// M3.7 slice 3a (D189): rows BA43 and BA44 of docs/plans/DRIVING_BEHAVIOUR.md -- lateral motion
// toward a target, its admission in vehicle-id order after the longitudinal step, and the crossing
// into the adjacent lane (docs/reference/LATERAL.md §7). Nothing in the product writes a target
// yet, so a test sets `lateralTarget` on a placed vehicle. Keys 0/0 and maxLateralSpeed 1 m/s at
// dt 0.1: a move is at most 0.1 m a tick.
namespace {
constexpr double kStep = 0.1;
const Vehicle& byId(const SimState& state, std::uint64_t id) {
    for (const auto& v : state.vehicles) if (v.id == id) return v;
    throw std::runtime_error("no vehicle " + std::to_string(id));
}
// The body centre across the carriageway: its front section's centre plus `lateral`.
double across(const SimState& state, const Vehicle& v) {
    const auto& part = frontPart(state.index->parts[v.routeIndex], v.distance);
    return state.scenario->segmentCrossSections[part.segmentIndex].centre + v.lateral;
}
double carWidth(const Scenario& s) { return s.vehicleTypes[slot(s.vehicleTypes, "car")].width; }
}

TEST(lateralmotion, ba43_the_lower_id_takes_contested_space_and_the_other_keeps_its_place) { // BA43
    // One 3.5 m lane and 0.5 m bodies side by side at the same distance, 0.15 m of road between them;
    // each targets the other's side, so both moves (0.1 m each) cannot fit, but either alone can.
    auto s = road(1, 3.5, keys(0, 0));
    s.vehicleTypes[slot(s.vehicleTypes, "car")].width = 0.5;
    const auto id = s.segments[0].id;
    auto state = place(s, {{id, 100, 0, -0.325}, {id, 100, 0, 0.325}});
    state.vehicles[0].lateralTarget = 1; state.vehicles[1].lateralTarget = -1;
    // The forcing: after vehicle 1's move the bodies would overlap by 0.05 m, before it they would not.
    CHECK(std::abs((-0.325 + kStep + 0.25) - (0.325 - kStep - 0.25) - 0.05) < 1e-12);
    state = stepSimulation(std::move(state));
    CHECK(std::abs(byId(state, 1).lateral - (-0.325 + kStep)) < 1e-12);
    CHECK(std::abs(byId(state, 1).lateralSpeed - 1) < 1e-12);
    CHECK(byId(state, 2).lateral == 0.325);
    CHECK(byId(state, 2).lateralSpeed == 0);
    CHECK(!overlap2d(state));
}

TEST(lateralmotion, ba43_touching_bodies_may_stand_side_by_side) { // BA43, admission is strict
    // 0.25 m of road; each target is 0.125 m away and inside a 0.3 m step, so both bodies end on y = 0
    // exactly (binary-exact values): touching, which is admitted.
    auto s = road(1, 3.5, keys(0, 0));
    s.vehicleTypes[slot(s.vehicleTypes, "car")].width = 0.5;
    for (auto& b : s.behaviours) b.lateral.maxLateralSpeed = 3;
    const auto id = s.segments[0].id;
    auto state = place(s, {{id, 100, 0, -0.375}, {id, 100, 0, 0.375}});
    state.vehicles[0].lateralTarget = -0.25; state.vehicles[1].lateralTarget = 0.25;
    state = stepSimulation(std::move(state));
    CHECK(byId(state, 1).lateral == -0.25);
    CHECK(byId(state, 2).lateral == 0.25);
    CHECK(byId(state, 1).lateral + 0.25 == byId(state, 2).lateral - 0.25); // the forcing: exactly touching
}

TEST(lateralmotion, ba43_a_move_into_a_published_body_is_refused_whatever_the_ids) { // BA43
    // The mover has the LOWER id: the published body of a later vehicle still refuses it.
    auto s = road(1, 3.5, keys(0, 0));
    s.vehicleTypes[slot(s.vehicleTypes, "car")].width = 0.5;
    const auto id = s.segments[0].id;
    auto state = place(s, {{id, 100, 0, 0.29}, {id, 100, 0, -0.29}});
    state.vehicles[0].lateralTarget = -1; // 0.08 m of road, a 0.1 m step
    state = stepSimulation(std::move(state));
    CHECK(byId(state, 1).lateral == 0.29);
    CHECK(byId(state, 1).lateralSpeed == 0);
    // Further away, the same target is admitted: the refusal was the body, not the target.
    auto free = place(s, {{id, 100, 0, 0.6}, {id, 100, 0, -0.6}});
    free.vehicles[0].lateralTarget = -1;
    free = stepSimulation(std::move(free));
    CHECK(std::abs(byId(free, 1).lateral - (0.6 - kStep)) < 1e-12);
}

TEST(lateralmotion, ba43_a_copied_state_replays_exactly) { // BA43
    const auto s = road(2, 3.5, keys(0.2, 0.5));
    const auto a = s.segments[laneSegment(s, 0)].id, b = s.segments[laneSegment(s, 1)].id;
    auto state = place(s, {{a, 60, 8, 0}, {b, 50, 9, 0}, {a, 30, 10, 0}, {b, 20, 7, 0}});
    for (std::size_t i = 0; i < state.vehicles.size(); ++i) state.vehicles[i].lateralTarget = i % 2 ? 3.5 : -3.5;
    for (int t = 0; t < 12; ++t) state = stepSimulation(std::move(state));
    CHECK(std::any_of(state.vehicles.begin(), state.vehicles.end(), [](const auto& v) { return v.lateral != 0; }));
    auto copy = state;
    for (int t = 0; t < 60; ++t) {
        state = stepSimulation(std::move(state)); copy = stepSimulation(std::move(copy));
        CHECK(state.vehicles == copy.vehicles); CHECK(state.events == copy.events);
    }
}

TEST(lateralmotion, ba44_a_covered_crossing_remaps_the_route_and_keeps_the_body_in_place) { // BA44
    // The left lane (centre +1.75) toward the right lane's centre (-1.75): a target of -3.5 from here.
    const auto s = road(2, 3.5, keys(0, 0));
    CHECK(!s.laneChanges.empty()); // the forcing: the two lanes' routes are joined by spans
    const auto left = laneSegment(s, 0), right = laneSegment(s, 1);
    auto state = place(s, {{s.segments[left].id, 50, 10, 0}});
    state.vehicles[0].lateralTarget = -3.5;
    const auto startRoute = state.vehicles[0].routeIndex;
    bool crossed = false;
    for (int t = 0; t < 60 && !state.vehicles.empty(); ++t) {
        const auto& was = state.vehicles[0];
        const double before = across(state, was), target = before - was.lateral + *was.lateralTarget;
        state = stepSimulation(std::move(state));
        const auto& v = state.vehicles[0];
        const double moved = across(state, v) - before;
        CHECK(moved <= 1e-12 && moved >= -kStep - 1e-12);                        // §7: the rate cap
        CHECK(std::abs(across(state, v) - v.lateral + *v.lateralTarget - target) < 1e-12); // the target re-expressed too
        const bool changed = std::any_of(state.events.begin(), state.events.end(), [](const auto& e) {
            return std::holds_alternative<LaneChangeEvent>(e); });
        if (!changed) continue;
        CHECK(!crossed); crossed = true;
        CHECK(v.routeIndex != startRoute);
        CHECK(std::abs(v.lateral) <= 1.75 + 1e-12);          // now measured from the right lane's centre
        CHECK(std::abs(before - 0.0) < kStep + 1e-12);       // the centre was at the boundary (y = 0)
        for (const auto& e : state.events)                    // the tick's MovedEvent names the new lane
            if (const auto* m = std::get_if<MovedEvent>(&e)) CHECK(m->segmentId == s.segments[right].id);
    }
    CHECK(crossed);
    CHECK(std::abs(state.vehicles[0].lateral) < 1e-12); // centred in the right lane
}

TEST(lateralmotion, ba44_without_a_span_the_centre_stops_on_the_lane_line) { // BA44
    auto s = road(2, 3.5, keys(0, 0));
    s.laneChanges.clear();
    auto state = place(s, {{s.segments[laneSegment(s, 0)].id, 50, 10, 0}});
    state.vehicles[0].lateralTarget = -3.5;
    const auto route = state.vehicles[0].routeIndex;
    for (int t = 0; t < 40; ++t) {
        state = stepSimulation(std::move(state));
        CHECK(state.vehicles[0].routeIndex == route);
        CHECK(state.vehicles[0].lateral >= -1.75);
    }
    CHECK(state.vehicles[0].lateral == -1.75); // the centre on the boundary, never past it
}

TEST(lateralmotion, ba44_a_body_never_passes_the_carriageway_edge) { // BA44
    // Each lane targets 5 m outward; no span leads off the carriageway, and the body's outer edge
    // stops exactly on the edge (the lane clamp alone would let the centre reach the edge line).
    const auto s = road(2, 3.5, keys(0, 0));
    const double half = carWidth(s) / 2;
    auto state = place(s, {{s.segments[laneSegment(s, 0)].id, 50, 10, 0}, {s.segments[laneSegment(s, 1)].id, 20, 10, 0}});
    state.vehicles[0].lateralTarget = 5; state.vehicles[1].lateralTarget = -5;
    for (int t = 0; t < 40; ++t) {
        state = stepSimulation(std::move(state));
        CHECK(across(state, byId(state, 1)) + half <= 3.5 + 1e-12);
        CHECK(across(state, byId(state, 2)) - half >= -3.5 - 1e-12);
    }
    CHECK(std::abs(across(state, byId(state, 1)) + half - 3.5) < 1e-12);
    CHECK(std::abs(across(state, byId(state, 2)) - half + 3.5) < 1e-12);
}

TEST(lateralmotion, moving_bodies_never_overlap_in_two_dimensions) { // LATERAL §6-§7
    // Eight cars on two lanes, every one targeting the other lane's centre and back, for 30 s.
    const auto s = road(2, 3.5, keys(0.2, 0.5));
    const auto a = s.segments[laneSegment(s, 0)].id, b = s.segments[laneSegment(s, 1)].id;
    auto state = place(s, {{a, 100, 6, 0}, {b, 95, 8, 0}, {a, 80, 9, 0}, {b, 70, 5, 0},
                           {a, 55, 10, 0}, {b, 45, 9, 0}, {a, 30, 8, 0}, {b, 20, 10, 0}});
    int crossings = 0;
    for (int t = 0; t < 300 && !state.vehicles.empty(); ++t) {
        if (t % 50 == 0)
            for (auto& v : state.vehicles) {
                const double centre = across(state, v) - v.lateral;
                v.lateralTarget = (centre > 0 ? -1.75 : 1.75) - centre; // the other lane's centre
            }
        state = stepSimulation(std::move(state));
        CHECK(!overlap2d(state));
        crossings += static_cast<int>(std::count_if(state.events.begin(), state.events.end(), [](const auto& e) {
            return std::holds_alternative<LaneChangeEvent>(e); }));
    }
    CHECK(crossings > 4); // the forcing: the lanes really were crossed
}
