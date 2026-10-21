// M8.5 row PL11 (docs/reference/ZONES_AND_PARKING.md §5): the lot report -- occupancy, flows, gate
// waits and the gate queue -- reconciled with the event stream, in JSON, CSV, checkpoints and batches.
#include "parking_fixture.hpp"
#include "../src/project/evaluation.hpp"
#include "../src/project/batch_output.hpp"
#include "../src/runner/batch.hpp"
#include <algorithm>
#include <set>
using namespace multra;
using namespace lotfix;
namespace {
// The M8.4b study with a 5-10 min dwell, so the full lot also releases vehicles inside the hour.
ProjectDocument shortDwell() {
    auto d = fixture(3, 40, 1).d;
    d.zones.parkingLots[0].dwell = {{5, 0}, {10, 1}};
    return d;
}
struct Observed {
    MovementReport report; SimState end;
    std::uint64_t gateArrivals{}, released{};
    std::vector<std::pair<double, std::size_t>> occupancy; // per snapshot: time, occupied
    std::vector<double> admissions;                        // event times
};
Observed observe(const ProjectDocument& d, const std::function<void(EvaluationSpec&)>& adjust = {}) {
    const auto data = test::root() / "data";
    const auto snapshot = compileDocument(d, data);
    auto spec = evaluationSpec(d, snapshot, data);
    if (adjust) adjust(spec);
    MovementAccumulator m(spec);
    Observed o;
    auto state = createSimulation(snapshot.scenario, 42);
    std::set<std::string> gates;
    for (const auto& lot : state.scenario->parkingLots) gates.insert(lot.gateRouteIds.begin(), lot.gateRouteIds.end());
    std::set<std::uint64_t> held; // parked or released, as of the last snapshot
    std::uint64_t completed = 0;
    const auto look = [&] {
        m.observe(state);
        for (const auto& e : state.events) {
            completed += std::holds_alternative<ArrivedEvent>(e);
            if (const auto* a = std::get_if<ArrivedEvent>(&e); a && gates.contains(a->routeId)) {
                ++o.gateArrivals; o.admissions.push_back(a->time);
            }
            if (const auto* dep = std::get_if<DepartedEvent>(&e)) o.released += held.contains(dep->vehicleId);
        }
        CHECK(state.nextVehicleId - 1 == state.vehicles.size() + pendingCount(state) + parkedCount(state) + completed - o.gateArrivals);
        if (state.lots.empty()) return;
        held.clear();
        for (const auto& p : state.lots[0].parked) held.insert(p.vehicle.id);
        held.insert(state.lots[0].leaving.begin(), state.lots[0].leaving.end());
        o.occupancy.push_back({state.time, state.lots[0].occupied()});
    };
    look();
    while (state.tick < totalTicks(*state.scenario)) { state = stepSimulation(std::move(state)); look(); }
    o.report = m.report(state); o.end = state;
    return o;
}
}
TEST(parkingoutputs, pl11a_report_reconciles_with_the_events) {
    const auto o = observe(shortDwell());
    const auto& r = o.report;
    CHECK(r.lots.size() == 1);
    const auto& row = r.lots[0];
    CHECK(row.name == "lot"); CHECK(row.capacity == 3);
    CHECK(o.gateArrivals > 3); CHECK(o.released > 0); // forcing: the lot fills and turns over
    CHECK(row.admitted == o.gateArrivals); CHECK(row.departed == o.released);
    const auto& lot = o.end.lots[0];
    CHECK(row.occupancyEnd == lot.occupied());
    CHECK(row.endParked == lot.parked.size()); CHECK(row.endInExitQueue == lot.leaving.size());
    CHECK(row.endAtGate == lot.gate.size());
    CHECK(row.occupancyPeak == 3);
    double sum = 0; for (const auto& [t, n] : o.occupancy) sum += static_cast<double>(n);
    test::near(row.occupancyMean, sum / static_cast<double>(o.occupancy.size()), 1e-9);
    CHECK(row.gateWaitMax && *row.gateWaitMax > 0); CHECK(row.waitedOverTick > 0);
    CHECK(*row.gateWaitMean <= *row.gateWaitMax);
    const auto gate = std::find_if(r.queues.begin(), r.queues.end(), [](const auto& q) { return q.name == "lot gate"; });
    CHECK(gate != r.queues.end()); CHECK(gate->maxLength > 0);
    // Every id is on the road, waiting at a source, parked, or through an ordinary trip end.
    CHECK(r.parked == lot.parked.size());
    CHECK(o.end.nextVehicleId - 1 == r.active + r.pending + r.parked + r.completed - row.admitted);
}
TEST(parkingoutputs, pl11b_window_selects_flows_and_occupancy) {
    const auto o = observe(shortDwell(), [](EvaluationSpec& s) { s.warmup = 600; s.end = 3000; });
    const auto& row = o.report.lots[0];
    const auto inside = [](double t) { return t >= 600 - 1e-9 && t <= 3000 + 1e-9; };
    const auto admitted = std::count_if(o.admissions.begin(), o.admissions.end(), inside);
    CHECK(admitted > 0 && static_cast<std::size_t>(admitted) < o.admissions.size()); // forcing
    CHECK(row.admitted == static_cast<std::uint64_t>(admitted));
    double sum = 0; std::size_t n = 0, last = 0;
    for (const auto& [t, k] : o.occupancy) if (inside(t)) { sum += static_cast<double>(k); ++n; last = k; }
    test::near(row.occupancyMean, sum / static_cast<double>(n), 1e-9);
    CHECK(row.occupancyEnd == last);
}
TEST(parkingoutputs, pl11c_json_and_csv_only_with_lots) {
    const auto o = observe(shortDwell());
    const auto j = movementJson(o.report);
    CHECK(j.contains("lots")); CHECK(j["lots"]["validated"] == false);
    CHECK(j["lots"]["rows"][0]["lot"] == "lot"); CHECK(j["parked"] == o.report.parked);
    const auto csv = movementCsv(o.report);
    CHECK(csv.find("\nlot,capacity,occupancyEnd,occupancyMean,occupancyPeak,admitted,departed,") != std::string::npos);
    CHECK(csv.find("\nparked,") != std::string::npos);
    // A lot-free project keeps its bytes: no lot key, no lot block.
    auto plain = shortDwell(); plain.zones.parkingLots.clear();
    auto& m = plain.zones.odMatrices[0]; m.cells[1][0] = 0; // no lot: the site's row has no departures
    const auto p = observe(plain);
    CHECK(p.report.lots.empty());
    CHECK(!movementJson(p.report).contains("lots")); CHECK(!movementJson(p.report).contains("parked"));
    CHECK(movementCsv(p.report).find("lot,") == std::string::npos);
}
TEST(parkingoutputs, pl11d_checkpoint_carries_lots) {
    const auto o = observe(shortDwell());
    const auto c = checkpointJson(o.end);
    CHECK(c.contains("lots")); CHECK(c["lots"][0]["id"] == "lot");
    CHECK(c["lots"][0]["parked"].size() == o.end.lots[0].parked.size());
    CHECK(c == checkpointJson(observe(shortDwell()).end));
    CHECK(!checkpointJson(createSimulation(test::demo().scenario, 42)).contains("lots"));
}
TEST(parkingoutputs, pl11e_batch_aggregates_lot_rows) {
    const auto d = shortDwell();
    const auto data = test::root() / "data";
    const auto snapshot = compileDocument(d, data);
    const auto spec = evaluationSpec(d, snapshot, data);
    auto runs = runSeeds(snapshot.scenario, spec, {42, 43, 44});
    const auto r = aggregate(runs);
    CHECK(r.lots.size() == 1); CHECK(r.lots[0].name == "lot");
    CHECK(r.lots[0].admitted.n == 3); CHECK(r.lots[0].occupancyPeak.mean.has_value());
    CHECK(batchJson(r, runs).contains("lots"));
    CHECK(batchCsv(r, runs).find("\nlot,") != std::string::npos);
    runs[1].report.lots[0].name = "other";
    test::throws([&] { aggregate(runs); }, "aggregate");
}
TEST(parkingoutputs, accounting_uses_all_time_admissions_and_includes_initial_identities) {
    auto d = shortDwell(); d.zones.parkingLots[0].initialOccupancy = 2;
    d.zones.parkingLots[0].initialCompositionId = "car-only";
    d.zones.parkingLots[0].dwell = {{0, 1}};
    const auto o = observe(d, [](EvaluationSpec& spec) { spec.warmup = 600; spec.end = 3000; });
    CHECK(o.gateArrivals > o.report.lots[0].admitted); // excluded admissions really exist
    const auto& r = o.report;
    CHECK(o.end.nextVehicleId - 1 == r.active + r.pending + r.parked + r.completed - o.gateArrivals);
    CHECK(o.end.nextVehicleId - 1 != r.active + r.pending + r.parked + r.completed - r.lots[0].admitted);
}
