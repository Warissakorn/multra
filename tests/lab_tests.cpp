// D165 (docs/reference/BEHAVIOUR_LAB.md §4, rows BL9-BL11): the lab ring and its run.
#include "test.hpp"
#include "../src/lab/lab_run.hpp"
using namespace trafficsim;
namespace {
lab::LabSettings settings() { return lab::defaultSettings(test::root() / "data"); }
lab::LabRun run(const lab::LabSettings& s) { return lab::LabRun(s, test::root() / "data", 42); }
void steps(lab::LabRun& r, int n) { for (int i = 0; i < n; ++i) r.step(); }
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
