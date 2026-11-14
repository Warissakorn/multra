// INPUT_STREAMS §6 rows IS1-IS7 and IS10-IS11 (D496, D505, D512): under `perInput` each input and lot draws from its own
// stream and a routing draw is hashed, so an edit to one object leaves every other one's draws alone.
// Each perInput row runs the same check under `shared` and expects it to fail: that is its forcing.
#include "test.hpp"
#include "../src/core/w74.hpp"
#include "../src/project/evaluation.hpp"
#include "../src/project/parallel_seeds.hpp"
#include "../src/project/run.hpp"
#include <cmath>
#include <fstream>
#include <map>
#include <tuple>
using namespace multra;
namespace {
using Draw = std::tuple<double, double, double>; // scheduledTime, desiredSpeed, driverFactor
// Road `road` carries input `input`; `side` is a disjoint road for the inputs a row adds or edits.
Scenario twoRoads(RandomStreams streams) {
    auto s = test::straight();
    s.duration = 300; s.inputs[0].endTime = 300;
    s.segments.push_back({"side", 150, {}}); s.routes.push_back({"sideRoute", {"side"}});
    s.randomStreams = streams;
    return s;
}
// Every vehicle input `id` released, in scheduled order, read from the queues before insertion.
std::vector<Draw> drawsOf(const Scenario& s, const std::string& id, std::uint32_t seed = 42) {
    auto state = createSimulation(s, seed);
    std::map<std::uint64_t, Draw> seen;
    while (state.tick < totalTicks(s)) {
        for (std::size_t i = 0; i < state.inputs.size(); ++i)
            if (state.scenario->inputs[i].id == id)
                for (const auto& v : state.inputs[i].queue) seen.emplace(v.id, Draw{v.scheduledTime, v.desiredSpeed, v.driverFactor});
        for (const auto& v : upcomingArrivals(state))
            if (v.inputIndex != PendingVehicle::kNoInput && state.scenario->inputs[v.inputIndex].id == id)
                seen.emplace(v.id, Draw{v.scheduledTime, v.desiredSpeed, v.driverFactor});
        state = stepSimulation(std::move(state));
    }
    std::vector<Draw> draws;
    for (const auto& [vehicleId, draw] : seen) draws.push_back(draw);
    std::sort(draws.begin(), draws.end());
    return draws;
}
Scenario withSide(Scenario s, double vehiclesPerHour) {
    s.inputs.push_back({"added", "sideRoute", "car", vehiclesPerHour, 0, s.duration});
    return s;
}
// BA14's fork: road (100) -> left | right, one even decision at 20 on route `a`, plus a disjoint `side`.
Scenario fork(RandomStreams streams, double sidePerHour) {
    auto s = test::straight();
    s.duration = 200;
    s.segments = {{"road", 100, {"left", "right"}}, {"left", 100, {}}, {"right", 100, {}}, {"side", 100, {}}};
    s.routes = {{"a", {"road", "left"}}, {"b", {"road", "right"}}, {"sideRoute", {"side"}}};
    s.inputs = {{"in", "a", "car", 900, 0, 150}};
    if (sidePerHour > 0) s.inputs.push_back({"busy", "sideRoute", "car", sidePerHour, 0, 150});
    s.routeDecisions = {{"line", "a", 20, {}, {{"a", 1, {}}, {"b", 1, {}}}}};
    s.randomStreams = streams;
    return s;
}
// Input `in`'s vehicles in order of release, each with the route its decision chose.
std::vector<std::pair<double, std::string>> choices(const Scenario& s) {
    auto state = createSimulation(s, 42);
    std::map<std::uint64_t, std::pair<double, std::string>> chosen;
    while (state.tick < totalTicks(s)) {
        state = stepSimulation(std::move(state));
        for (const auto& e : state.events)
            if (const auto* r = std::get_if<RoutingEvent>(&e))
                for (const auto& v : state.vehicles)
                    if (v.id == r->vehicleId) chosen[v.id] = {v.scheduledTime, r->toRouteId};
    }
    std::vector<std::pair<double, std::string>> result;
    for (const auto& [id, c] : chosen) result.push_back(c);
    std::sort(result.begin(), result.end());
    return result;
}
}
TEST(inputstreams, is1_shared_touches_no_stream_field) {
    // The frozen fixtures and baselines in the rest of the suite are the row's main evidence; this
    // checks the switch's default and that shared leaves every new field at zero.
    CHECK(Scenario{}.randomStreams == RandomStreams::shared);
    auto s = fork(RandomStreams::shared, 600);
    auto state = createSimulation(s, 42);
    CHECK(state.index->keyOfInput.empty());
    while (state.tick < 600) state = stepSimulation(std::move(state));
    for (const auto& input : state.inputs) { CHECK(input.stream == 0); CHECK(input.generated == 0); }
    for (const auto& v : state.vehicles) { CHECK(v.streamKey == 0); CHECK(v.ordinal == 0); }
}
TEST(inputstreams, is2_an_added_input_leaves_the_others_draws_alone) {
    for (const auto streams : {RandomStreams::perInput, RandomStreams::shared}) {
        const auto base = drawsOf(twoRoads(streams), "input");
        const auto added = drawsOf(withSide(twoRoads(streams), 600), "input");
        CHECK(base.size() > 30);
        CHECK((base == added) == (streams == RandomStreams::perInput));
    }
}
TEST(inputstreams, is3_one_inputs_volume_leaves_another_alone) {
    for (const auto streams : {RandomStreams::perInput, RandomStreams::shared}) {
        const auto low = drawsOf(withSide(twoRoads(streams), 300), "input");
        const auto high = drawsOf(withSide(twoRoads(streams), 1200), "input");
        CHECK((low == high) == (streams == RandomStreams::perInput));
    }
}
TEST(inputstreams, is4_equal_inputs_draw_different_sequences) {
    auto s = withSide(twoRoads(RandomStreams::perInput), 900);
    s.inputs[0].vehiclesPerHour = 900;
    const auto a = drawsOf(s, "input"), b = drawsOf(s, "added");
    CHECK(!a.empty()); CHECK(a != b);
    // Keys of every input and lot in the shipped projects and this suite's fixtures are distinct.
    CHECK(detail::streamKey(detail::kInputStream, "input") != detail::streamKey(detail::kInputStream, "added"));
    CHECK(detail::streamKey(detail::kInputStream, "lot") != detail::streamKey(detail::kLotStream, "lot"));
}
TEST(inputstreams, is5_each_streams_first_arrival_is_not_late_over_the_d88_seeds) {
    // SM4's band (D159), per stream: 27 veh/h, mean headway 133.3 s, 40 seeds.
    auto s = twoRoads(RandomStreams::perInput);
    s.duration = 3600; s.inputs[0].vehiclesPerHour = 27; s.inputs[0].endTime = 3600;
    s = withSide(s, 27);
    for (std::size_t input = 0; input < 2; ++input) {
        double sum = 0;
        for (std::uint32_t seed = 42; seed <= 81; ++seed) {
            const auto state = createSimulation(s, seed);
            CHECK(state.inputs[input].nextArrival.has_value());
            sum += *state.inputs[input].nextArrival;
        }
        const double headway = 3600.0 / 27;
        CHECK(std::abs(sum / 40 - headway) <= 1.96 * headway / std::sqrt(40.0));
    }
}
TEST(inputstreams, is6_a_routing_choice_does_not_depend_on_another_road) {
    for (const auto streams : {RandomStreams::perInput, RandomStreams::shared}) {
        const auto quiet = choices(fork(streams, 0)), busy = choices(fork(streams, 1800));
        CHECK(quiet.size() > 20);
        const auto toB = std::count_if(quiet.begin(), quiet.end(), [](const auto& c) { return c.second == "b"; });
        CHECK(toB > 0); CHECK(toB < static_cast<std::ptrdiff_t>(quiet.size())); // both choices are drawn
        CHECK((quiet == busy) == (streams == RandomStreams::perInput));
    }
}
TEST(inputstreams, is6b_a_lots_draws_do_not_depend_on_an_added_input) {
    const auto lotRun = [](RandomStreams streams, bool added) {
        auto s = test::straight();
        s.duration = 120;
        s.segments = {{"in", 150, {}}, {"out", 150, {}}, {"side", 150, {}}};
        s.routes = {{"toLot", {"in"}}, {"fromLot", {"out"}}, {"sideRoute", {"side"}}};
        s.inputs = {{"exit", "fromLot", "car", 0, 0, 120}};
        if (added) s.inputs.push_back({"added", "sideRoute", "car", 900, 0, 120});
        ParkingLotSpec lot;
        lot.id = "lot"; lot.capacity = 20; lot.initialOccupancy = 12;
        lot.dwell = {{10, 0}, {100, 1}};
        lot.gateRouteIds = {"toLot"}; lot.exits = {{"fromLot", "exit", 1}};
        lot.initialTypes = {{"car", 1}, {"heavy-vehicle", 1}};
        s.parkingLots = {lot};
        s.randomStreams = streams;
        std::vector<std::tuple<double, double, double, std::uint32_t>> parked;
        const auto state = createSimulation(s, 42); // a named state: the range must outlive the loop
        for (const auto& p : state.lots[0].parked)
            parked.emplace_back(p.dwellEnd, p.vehicle.desiredSpeed, p.vehicle.driverFactor, p.vehicle.typeIndex);
        return parked;
    };
    for (const auto streams : {RandomStreams::perInput, RandomStreams::shared}) {
        const auto base = lotRun(streams, false), added = lotRun(streams, true);
        CHECK(base.size() == 12);
        CHECK((base == added) == (streams == RandomStreams::perInput));
    }
}
TEST(inputstreams, is7_replay_is_exact_and_threads_change_nothing) {
    std::ifstream file(test::root() / "data/projects/four-leg-signalised.traffic.json"); Json j; file >> j;
    const auto d = parseDocument(j);
    const auto data = test::root() / "data";
    auto snapshot = compileDocument(d, data);
    const auto spec = evaluationSpec(d, snapshot, data);
    snapshot.scenario.randomStreams = RandomStreams::perInput;
    const auto a = test::finish(createSimulation(snapshot.scenario, 42)), b = test::finish(createSimulation(snapshot.scenario, 42));
    CHECK(a.vehicles == b.vehicles); CHECK(a.inputs == b.inputs); CHECK(a.completed == b.completed);
    CHECK(a.completed > 0);
    const std::vector<std::uint32_t> seeds{44, 42, 43, 45};
    const auto inline_ = runSeedsParallel(snapshot.scenario, spec, seeds, {}, 1);
    CHECK(runSeedsParallel(snapshot.scenario, spec, seeds, {}, 4) == inline_);
    auto shared = snapshot.scenario; shared.randomStreams = RandomStreams::shared;
    CHECK(!(runSeedsParallel(shared, spec, seeds, {}, 1) == inline_)); // the switch is really on
}
namespace {
// twoRoads with an unused w74 behaviour (as w74traits' fixture), so every vehicle carries traits.
Scenario withTraits(Scenario s) {
    DriverBehaviour w74{"w74"}; w74.w74 = W74Parameters{2, 2, 1, 2, .5, 16, 4, .5, .5, 20, 1, 1, 3, .25, .5, -1, .5, .25};
    s.behaviours.push_back(w74);
    return s;
}
// Input `input`'s vehicles in scheduled order, each with its W74 traits, read before insertion.
std::vector<std::pair<double, W74Traits>> traitsOf(const Scenario& s) {
    auto state = createSimulation(s, 42);
    std::map<std::uint64_t, std::pair<double, W74Traits>> seen;
    while (state.tick < totalTicks(s)) {
        for (const auto& v : upcomingArrivals(state))
            if (v.inputIndex != PendingVehicle::kNoInput && state.scenario->inputs[v.inputIndex].id == "input" && v.w74Traits)
                seen.emplace(v.id, std::pair{v.scheduledTime, *v.w74Traits});
        state = stepSimulation(std::move(state));
    }
    std::vector<std::pair<double, W74Traits>> result;
    for (const auto& [id, t] : seen) result.push_back(t);
    std::sort(result.begin(), result.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
    return result;
}
// A head 100 m along `road`, red 15 s then green 15 s, and half the drivers runners for 3 s.
Scenario withRunners(Scenario s) {
    s.signalPrograms = {{"p", 0, {{15, SignalColor::red}, {15, SignalColor::green}}}};
    s.signalHeads = {{"h", "road", 100, "p"}};
    for (auto& b : s.behaviours) { b.redRunningShare = 0.5; b.redRunningTime = 3; }
    return s;
}
// The scheduled times of the vehicles on `road` that ran the red.
std::vector<double> runnersOf(const Scenario& s) {
    auto state = createSimulation(s, 42);
    std::vector<double> runners;
    while (state.tick < totalTicks(s)) {
        state = stepSimulation(std::move(state));
        for (const auto& e : state.events)
            if (const auto* r = std::get_if<RedRunningEvent>(&e))
                for (const auto& v : state.vehicles) if (v.id == r->vehicleId) runners.push_back(v.scheduledTime);
    }
    std::sort(runners.begin(), runners.end());
    return runners;
}
}
TEST(inputstreams, is10_an_added_input_leaves_the_others_w74_traits_alone) {
    for (const auto streams : {RandomStreams::perInput, RandomStreams::shared}) {
        const auto base = traitsOf(withTraits(twoRoads(streams)));
        const auto added = traitsOf(withTraits(withSide(twoRoads(streams), 600)));
        CHECK(base.size() > 30);
        CHECK((base == added) == (streams == RandomStreams::perInput));
    }
    // A vehicle with no origin key (shared, or placed by hand) keeps hashing on its id, to the bit.
    CHECK(detail::traitIdentity(7, 0, 3) == 7);
    CHECK(detail::traitIdentity(7, detail::streamKey(detail::kInputStream, "input"), 3) != 7);
}
TEST(inputstreams, is11_an_added_input_leaves_the_others_red_runners_alone) {
    for (const auto streams : {RandomStreams::perInput, RandomStreams::shared}) {
        const auto base = runnersOf(withRunners(twoRoads(streams)));
        const auto added = runnersOf(withRunners(withSide(twoRoads(streams), 600)));
        CHECK(base.size() >= 3); // the head really is run
        CHECK((base == added) == (streams == RandomStreams::perInput));
    }
}
