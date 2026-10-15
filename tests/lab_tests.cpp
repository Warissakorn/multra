// D165 (docs/reference/BEHAVIOUR_LAB.md §4, rows BL9-BL11): the lab ring and its run.
#include "test.hpp"
#include "../src/lab/lab_run.hpp"
using namespace trafficsim;
namespace {
// The D165 rows were written on the Oval; D166's Urban circuit has rows of its own below.
lab::LabSettings settings() {
    auto s = lab::defaultSettings(test::root() / "data");
    s.road.layout = lab::Layout::oval; s.road.straightLength = 300;
    return s;
}
lab::LabRun run(const lab::LabSettings& s) { return lab::LabRun(s, test::root() / "data", 42); }
void steps(lab::LabRun& r, int n) { for (int i = 0; i < n; ++i) r.step(); }
}
TEST(lab, reducing_live_desired_speed_brakes_with_consistent_motion) {
    auto s = settings(); s.road.signal = false;
    s.traffic.vehicleCount = 0; s.traffic.crossVehicleCount = 0;
    s.focus.type.desiredSpeed = {15, 15};
    auto r = run(s); steps(r, 300);
    const auto id = r.focusId(), tick = r.state().tick;
    CHECK(r.telemetry()->speed > 14.9);
    s.focus.type.desiredSpeed = {5, 5};
    CHECK(r.apply(s) == lab::LabRun::Applied::retuned);
    CHECK(r.focusId() == id && r.state().tick == tick);
    CHECK(r.state().vehicles.front().speed > 14.9 && r.state().vehicles.front().desiredSpeed == 5);
    for (int i = 0; i < 100; ++i) {
        const auto before = r.state().vehicles.front();
        r.step(); const auto& after = r.state().vehicles.front();
        const double dt = r.state().scenario->timeStep;
        // Free flow above the target slows at the comfortable rate, as W74 does (D191).
        CHECK(after.speed >= before.speed - s.focus.type.comfortableDeceleration * dt - 1e-9);
        test::near(after.speed - before.speed, after.acceleration * dt);
        // This short interval never reaches the circuit seam.
        test::near(after.distance - before.distance, before.speed * dt + .5 * after.acceleration * dt * dt);
    }
    test::near(r.telemetry()->speed, 5, .01);
    CHECK(r.safetyClamps() == 0);
}
TEST(lab, bl9_the_ring_compiles_and_runs_an_hour_without_a_clamp) {
    auto s = settings();
    for (const int lanes : {1, 2}) {
        s.road.lanes = lanes;
        auto r = run(s);
        CHECK(r.state().scenario->routes.size() == static_cast<std::size_t>(lanes));
        for (const auto& route : r.state().scenario->routes) CHECK(route.circuit);
        CHECK(r.state().vehicles.size() == static_cast<std::size_t>(s.traffic.vehicleCount + 1));
        CHECK(r.focusId() == r.state().vehicles.front().id);
        steps(r, 36000);
        CHECK(r.state().vehicles.size() == static_cast<std::size_t>(s.traffic.vehicleCount + 1));
        CHECK(r.safetyClamps() == 0);
        CHECK(r.telemetry()->laps > 10);
    }
}
TEST(lab, bl9_a_signal_holds_the_ring_without_a_clamp) {
    auto s = settings();
    s.road.signal = true;
    auto r = run(s);
    CHECK(!r.state().scenario->signalHeads.empty());
    bool stopped = false;
    for (int i = 0; i < 6000; ++i) {
        r.step();
        if (r.telemetry()->speed < 0.1 && r.state().time > 60) stopped = true;
    }
    CHECK(stopped);
    CHECK(r.safetyClamps() == 0);
}
TEST(lab, bl10_a_safety_distance_retune_changes_the_focus_gap_and_keeps_the_vehicles) {
    auto s = settings();
    s.traffic.vehicleCount = 24; // a dense ring, so everyone follows someone
    s.traffic.heavyShare = 0;
    auto a = run(s), b = run(s);
    steps(a, 3000); steps(b, 3000);
    const auto ids = [](const lab::LabRun& r) {
        std::vector<std::uint64_t> v; for (const auto& x : r.state().vehicles) v.push_back(x.id); return v;
    };
    auto tuned = s;
    // The prototype's desired gap is standstill + (bx_add + bx_mult z) sqrt(v): followingTime sets how
    // fast it is reached, not its size (core/following.cpp).
    tuned.focus.behaviour.additiveSafetyDistance *= 3;
    CHECK(b.apply(tuned) == lab::LabRun::Applied::retuned);
    CHECK(ids(a) == ids(b));
    CHECK(b.state().tick == a.state().tick);
    double gapA = 0, gapB = 0;
    for (int i = 0; i < 3000; ++i) {
        a.step(); b.step();
        if (i >= 2000) { gapA += *a.telemetry()->gap; gapB += *b.telemetry()->gap; }
    }
    CHECK(gapB > gapA * 1.2); // a larger safety distance keeps a longer gap
    // Anything that changes the vehicles or the road restarts from t = 0.
    auto road = tuned; road.road.lanes = 2;
    CHECK(b.apply(road) == lab::LabRun::Applied::restarted);
    CHECK(b.state().tick == 0);
    CHECK(b.apply(road) == lab::LabRun::Applied::unchanged);
}
TEST(lab, bl11_telemetry_gap_matches_the_geometry_across_the_seam) {
    auto s = settings();
    s.traffic.vehicleCount = 1;
    auto r = run(s);
    const auto& state = r.state();
    // Two vehicles half a lap apart: front to rear is half the lap less the leader's length.
    const auto& focus = state.vehicles[0], &other = state.vehicles[1];
    const double lap = (state.index->parts[0].back().start + state.index->parts[0].back().length) / 3;
    const double length = state.scenario->vehicleTypes[other.typeIndex].length;
    test::near(*r.telemetry()->gap, other.distance - focus.distance - length, 1e-6);
    // The other vehicle sees the focus vehicle across the seam.
    test::near(*r.telemetry(other.id)->gap,
               focus.distance + lap - other.distance - state.scenario->vehicleTypes[focus.typeIndex].length, 1e-6);
}
TEST(lab, bl11_too_many_vehicles_are_refused_and_the_run_kept) {
    auto s = settings();
    auto r = run(s);
    steps(r, 10);
    auto crowded = s; crowded.traffic.vehicleCount = lab::maxVehicles(s) + 5;
    test::throws([&] { r.apply(crowded); }, "LAB_TOO_MANY_VEHICLES");
    CHECK(r.state().tick == 10);
    CHECK(r.settings() == s);
}
namespace {
lab::LabSettings urban() { return lab::defaultSettings(test::root() / "data"); }
bool entered(const SimState& s, std::uint64_t id, const std::string& prefix) {
    for (const auto& e : s.events)
        if (const auto* x = std::get_if<SegmentEnteredEvent>(&e); x && x->vehicleId == id && x->segmentId.starts_with(prefix)) return true;
    return false;
}
}
TEST(lab, bl13_the_urban_circuit_compiles_four_signalised_loops) {
    auto s = urban();
    CHECK(s.road.layout == lab::Layout::urban); // the default (D166)
    for (const int lanes : {1, 2}) {
        s.road.lanes = lanes;
        const auto built = lab::buildLabDocument(s);
        CHECK(built.crossRoutes.size() == 3);
        CHECK(built.document.definition->signalControllers.size() == 4);
        auto r = run(s);
        const auto& scenario = *r.state().scenario;
        CHECK(scenario.routes.size() == static_cast<std::size_t>(4 * lanes));
        for (const auto& route : scenario.routes) CHECK(route.circuit);
        // A head on every lane of both approaches of each intersection.
        CHECK(scenario.signalHeads.size() == static_cast<std::size_t>(4 * 2 * lanes));
        CHECK(r.state().vehicles.size() == static_cast<std::size_t>(s.traffic.vehicleCount + 1 + 3 * s.traffic.crossVehicleCount));
    }
}
TEST(lab, bl14_the_tuned_vehicle_laps_turns_and_stops_at_red) {
    for (const int lanes : {1, 2}) {
        auto s = urban();
        s.road.lanes = lanes;
        auto r = run(s);
        const auto count = r.state().vehicles.size();
        // The X2 and X3 turns are the main loop's third and fourth Connectors.
        const auto built = lab::buildLabDocument(s);
        const auto& objects = built.document.definition->routes.front().segmentIds;
        CHECK(objects.size() == 8);
        bool turnedX2 = false, turnedX3 = false, stopped = false;
        for (int i = 0; i < 36000; ++i) {
            r.step();
            turnedX2 = turnedX2 || entered(r.state(), r.focusId(), objects[5]);
            turnedX3 = turnedX3 || entered(r.state(), r.focusId(), objects[7]);
            // Queued behind others at a red, the prototype creeps rather than standing (~0.4 m/s).
            stopped = stopped || (r.telemetry()->speed < 1 && r.state().time > 30);
        }
        CHECK(r.state().vehicles.size() == count);
        CHECK(r.telemetry()->laps > 5);
        CHECK(turnedX2); CHECK(turnedX3); CHECK(stopped);
        if (r.safetyClamps() != 0) throw std::runtime_error("urban safety clamps: " + std::to_string(r.safetyClamps()));
    }
}
TEST(lab, bl15_signal_timings_retune_and_a_layout_change_restarts) {
    auto s = urban();
    auto r = run(s);
    steps(r, 500);
    const auto tick = r.state().tick;
    auto timed = s; timed.road.green = 40;
    CHECK(r.apply(timed) == lab::LabRun::Applied::retuned);
    CHECK(r.state().tick == tick);
    auto crossed = timed; crossed.traffic.crossVehicleCount = 2;
    CHECK(r.apply(crossed) == lab::LabRun::Applied::restarted);
    auto oval = crossed; oval.road.layout = lab::Layout::oval;
    CHECK(r.apply(oval) == lab::LabRun::Applied::restarted);
    CHECK(r.state().scenario->routes.size() == 1);
    auto tiny = s; tiny.road.straightLength = 100;
    test::throws([&] { r.apply(tiny); }, "LAB_ROAD_INVALID");
}
TEST(lab, length_retune_preserves_valid_axle_proportions_and_identity) {
    auto s = settings(); s.traffic.vehicleCount = 0;
    auto r = run(s); steps(r, 10);
    const auto id = r.focusId(), tick = r.state().tick;
    const auto axles = *s.focus.type.axles;
    auto tuned = s; tuned.focus.type.length *= 1.5;
    CHECK(r.apply(tuned) == lab::LabRun::Applied::retuned);
    CHECK(r.focusId() == id && r.state().tick == tick);
    const auto actual = *r.settings().focus.type.axles;
    test::near(actual.wheelbase, 1.5 * axles.wheelbase);
    test::near(actual.frontOverhang, 1.5 * axles.frontOverhang);
    test::near(actual.rearOverhang, 1.5 * axles.rearOverhang);
    CHECK(r.apply(tuned) == lab::LabRun::Applied::unchanged);
    auto bad = tuned; bad.focus.type.axles->wheelbase = -1;
    const auto before = r.state(); test::throws([&] { r.apply(bad); }, "AXLE");
    CHECK(checkpointJson(r.state()) == checkpointJson(before));
    CHECK(r.state().scenario == before.scenario);
    auto heavy = s.traffic.heavy; const auto original = *heavy.axles; heavy.length *= 2;
    lab::rescaleAxles(heavy); test::near(heavy.axles->wheelbase, 2 * original.wheelbase);
    auto nominal = s.focus.type; nominal.axles.reset(); nominal.length = 6; lab::rescaleAxles(nominal);
    CHECK(!nominal.axles);
}
