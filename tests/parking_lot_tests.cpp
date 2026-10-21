// M8.4a rows (docs/reference/ZONES_AND_PARKING.md §4, §7): the engine's parking lot on hand-built
// scenarios. A full lot queues at its gate and never deletes a vehicle.
#include "test.hpp"
#include <algorithm>
#include <map>
#include <set>
using namespace multra;
namespace {
// Road `in` ends at the lot's gate; road `out` leaves it. `arrivals` feeds the gate, `exit` (0 veh/h)
// is the queue departures wait in.
Scenario lotScenario(std::uint32_t capacity, double dwell, double arrivalsPerHour, double duration = 300) {
    const auto source = test::demo().scenario;
    Scenario s;
    s.duration = duration; s.timeStep = 0.1;
    s.segments = {{"in", 150, {}}, {"out", 150, {}}};
    s.routes = {{"toLot", {"in"}}, {"fromLot", {"out"}}};
    s.vehicleTypes = source.vehicleTypes; s.behaviours = source.behaviours;
    s.inputs = {{"arrivals", "toLot", "car", arrivalsPerHour, 0, duration}, {"exit", "fromLot", "car", 0, 0, duration}};
    ParkingLotSpec lot;
    lot.id = "lot"; lot.capacity = capacity;
    lot.dwell = {{dwell, 0}, {dwell + 1, 1}};
    lot.gateRouteIds = {"toLot"};
    lot.exits = {{"fromLot", "exit", 1}};
    s.parkingLots = {lot};
    return s;
}
// Every vehicle id is in exactly one place: on the road, in an input queue, parked, or gone through
// an ordinary route end. `exited` collects the last from the events.
void checkIdentity(const SimState& state, std::set<std::uint64_t>& exited) {
    for (const auto& event : state.events)
        if (const auto* a = std::get_if<ArrivedEvent>(&event); a && a->routeId != "toLot") exited.insert(a->vehicleId);
    std::map<std::uint64_t, int> seen;
    for (const auto& v : state.vehicles) ++seen[v.id];
    for (const auto& input : state.inputs) for (const auto& v : input.queue) ++seen[v.id];
    for (const auto& lot : state.lots) for (const auto& p : lot.parked) ++seen[p.vehicle.id];
    for (const auto id : exited) ++seen[id];
    CHECK(seen.size() == state.nextVehicleId - 1);
    for (const auto& [id, count] : seen) CHECK(count == 1);
    for (std::size_t k = 0; k < state.lots.size(); ++k)
        CHECK(state.lots[k].occupied() <= state.scenario->parkingLots[k].capacity);
}
const InputState& queueOf(const SimState& state, const std::string& id) {
    for (std::size_t i = 0; i < state.inputs.size(); ++i) if (state.scenario->inputs[i].id == id) return state.inputs[i];
    throw std::runtime_error("no input " + id);
}
std::size_t admittedIn(const SimState& state) {
    std::size_t n = 0;
    for (const auto& e : state.events) if (const auto* a = std::get_if<ArrivedEvent>(&e); a && a->routeId == "toLot") ++n;
    return n;
}
}
TEST(parkinglot, pl1_full_lot_queues_and_never_deletes) {
    const auto s = lotScenario(2, 100000, 1800);
    auto state = createSimulation(s, 3141592653u);
    std::set<std::uint64_t> exited;
    std::size_t admitted = 0;
    while (state.tick < totalTicks(s)) {
        state = stepSimulation(std::move(state));
        admitted += admittedIn(state);
        checkIdentity(state, exited);
    }
    CHECK(state.lots[0].occupied() == 2); // forcing: the lot is full
    CHECK(admitted == 2);
    CHECK(state.nextVehicleId - 1 >= 5); // forcing: at least three more arrived
    std::size_t onRoad = 0;
    for (const auto& v : state.vehicles) if (state.scenario->routes[v.routeIndex].id == "toLot") ++onRoad;
    const std::size_t queued = queueOf(state, "arrivals").queue.size();
    CHECK(onRoad + queued == state.nextVehicleId - 1 - 2); // every other arrival is still waiting
    CHECK(onRoad >= 3);
    // The one at the gate stands at the line, not past it.
    double front = 0;
    for (const auto& v : state.vehicles) front = std::max(front, v.distance);
    CHECK(front <= 150 + 1e-9);
}
TEST(parkinglot, pl2_occupancy_never_exceeds_capacity) {
    const auto s = lotScenario(3, 20, 900, 200);
    for (std::uint32_t k = 1; k <= 40; ++k) {
        auto state = createSimulation(s, k * 2654435761u);
        std::set<std::uint64_t> exited;
        std::size_t admitted = 0;
        while (state.tick < totalTicks(s)) {
            state = stepSimulation(std::move(state));
            admitted += admittedIn(state);
            checkIdentity(state, exited);
        }
        CHECK(admitted >= 3); // forcing: the lot filled at least once per seed
    }
}
TEST(parkinglot, pl2b_two_reach_the_gate_in_one_tick_with_one_space) {
    // Two lanes into one lot with one space and no gate service: both drive in at once, side by side.
    auto s = lotScenario(1, 100000, 0, 30);
    s.segments.push_back({"in2", 150, {}});
    s.routes.push_back({"toLot2", {"in2"}});
    s.parkingLots[0].gateRouteIds.push_back("toLot2");
    auto state = createSimulation(s, 1);
    for (const auto* route : {"toLot", "toLot2"}) {
        Vehicle v;
        v.id = state.nextVehicleId++;v.distance = 140;v.speed = 10;v.desiredSpeed = 15;v.driverFactor = 0.5;
        for (std::uint32_t i = 0; i < state.scenario->routes.size(); ++i) if (state.scenario->routes[i].id == route) v.routeIndex = i;
        state.vehicles.push_back(v);
    }
    std::size_t admitted = 0, clamps = 0;
    while (state.tick < totalTicks(s)) {
        state = stepSimulation(std::move(state));
        for (const auto& e : state.events) {
            if (const auto* a = std::get_if<ArrivedEvent>(&e); a && a->routeId.starts_with("toLot")) ++admitted;
            if (std::get_if<SafetyClampEvent>(&e)) ++clamps;
        }
        CHECK(state.lots[0].occupied() <= 1);
    }
    CHECK(admitted == 1);CHECK(state.vehicles.size() == 1); // the second stays at the line, not deleted
    CHECK(state.vehicles[0].distance == 150);CHECK(state.vehicles[0].speed == 0);
    CHECK(clamps == 1); // and the engine reports that it stopped it
}
TEST(parkinglot, pl3_gate_queue_spills_back_and_blocks_through_traffic) {
    auto s = lotScenario(1, 100000, 1800);
    s.segments = {{"approach", 100, {"in", "past"}}, {"in", 30, {}}, {"past", 100, {}}, {"out", 150, {}}};
    s.routes = {{"toLot", {"approach", "in"}}, {"fromLot", {"out"}}, {"through", {"approach", "past"}}};
    s.inputs.push_back({"passing", "through", "car", 300, 0, s.duration});
    auto state = createSimulation(s, 2718281828u);
    while (state.tick < totalTicks(s)) state = stepSimulation(std::move(state));
    CHECK(state.lots[0].occupied() == 1); // forcing: full from the first arrival on
    const auto& routes = state.scenario->routes;
    bool spilled = false, throughStuck = false;
    for (const auto& v : state.vehicles) {
        const auto& id = routes[v.routeIndex].id;
        if (id == "toLot" && v.distance < 100 && v.speed < 0.1) spilled = true;   // standing on the street
        if (id == "through" && v.distance < 100 && v.speed < 0.1) throughStuck = true;
    }
    CHECK(spilled);CHECK(throughStuck);
}
TEST(parkinglot, pl4_gate_service_time) {
    for (const double service : {10.0, 0.0}) {
        auto s = lotScenario(5, 100000, 60, 200);
        s.parkingLots[0].gateServiceTime = service;
        auto state = createSimulation(s, 1234567891u);
        std::optional<double> cameToLine, admitted;
        double speedBefore = -1;
        while (state.tick < totalTicks(s) && !admitted) {
            const auto before = state;
            state = stepSimulation(std::move(state));
            for (const auto& e : state.events)
                if (const auto* a = std::get_if<ArrivedEvent>(&e); a && a->routeId == "toLot") {
                    admitted = a->time;
                    for (const auto& v : before.vehicles) if (v.id == a->vehicleId) speedBefore = v.speed;
                }
            if (!cameToLine && !state.lots[0].gate.empty()) cameToLine = static_cast<double>(state.lots[0].gate.front().since) * s.timeStep;
        }
        CHECK(admitted); // forcing: the first arrival reached the lot
        if (service > 0) { CHECK(cameToLine);CHECK(*admitted - *cameToLine >= service - 1e-9); }
        else CHECK(speedBefore > 1); // drove in without stopping
    }
}
TEST(parkinglot, pl5_departure_keeps_its_space_until_inserted) {
    for (const bool blocked : {true, false}) {
        auto s = lotScenario(2, 5, 1200, 240);
        if (blocked) {
            s.signalPrograms = {{"red", 0, {{60, SignalColor::red}}}};
            s.signalHeads = {{"h", "out", 2, "red"}};
        }
        auto state = createSimulation(s, 1618033988u);
        std::set<std::uint64_t> exited;
        std::size_t admitted = 0;
        while (state.tick < totalTicks(s)) {
            state = stepSimulation(std::move(state));
            admitted += admittedIn(state);
            checkIdentity(state, exited);
        }
        const auto& lot = state.lots[0];
        const auto& queue = queueOf(state, "exit").queue;
        if (blocked) {
            CHECK(!queue.empty()); // forcing: a departure cannot get onto the blocked exit
            for (const auto& v : queue) CHECK(std::find(lot.leaving.begin(), lot.leaving.end(), v.id) != lot.leaving.end());
            CHECK(lot.occupied() == 2);
            CHECK(admitted <= 3); // two, then one more after the first departure got out
        } else {
            CHECK(queue.empty());CHECK(admitted >= 6);
        }
    }
}
TEST(parkinglot, pl6_deterministic) {
    const auto s = lotScenario(2, 15, 900, 200);
    const auto run = [&] {
        std::vector<SimEvent> events;std::vector<LotState> lots;
        auto state = createSimulation(s, 99);
        while (state.tick < totalTicks(s)) {
            state = stepSimulation(std::move(state));
            events.insert(events.end(), state.events.begin(), state.events.end());
            lots.push_back(state.lots[0]);
        }
        return std::pair{events, lots};
    };
    const auto a = run(), b = run();
    CHECK(!a.first.empty());CHECK(a == b);
    // Admissions happen in arrival order at the gate: the admitted ids ascend on a single lane.
    std::vector<std::uint64_t> order;
    for (const auto& e : a.first) if (const auto* x = std::get_if<ArrivedEvent>(&e); x && x->routeId == "toLot") order.push_back(x->vehicleId);
    CHECK(order.size() >= 4);CHECK(std::is_sorted(order.begin(), order.end()));
}
TEST(parkinglot, pl7_initial_occupants_leave_by_their_dwell) {
    auto s = lotScenario(4, 10, 0, 120);
    s.parkingLots[0].initialOccupancy = 3;
    s.parkingLots[0].initialTypes = {{"car", 1}};
    const auto state0 = createSimulation(s, 5);
    CHECK(state0.lots[0].parked.size() == 3);CHECK(state0.nextVehicleId == 4);
    for (const auto& p : state0.lots[0].parked) {
        CHECK(state0.scenario->vehicleTypes[p.vehicle.typeIndex].id == "car");
        CHECK(p.dwellEnd >= 10 - 1e-9);CHECK(p.dwellEnd <= 11 + 1e-9);
    }
    auto state = state0;std::set<std::uint64_t> departed;
    while (state.tick < totalTicks(s)) {
        state = stepSimulation(std::move(state));
        for (const auto& e : state.events)
            if (const auto* d = std::get_if<DepartedEvent>(&e)) { CHECK(d->routeId == "fromLot");departed.insert(d->vehicleId); }
    }
    CHECK(departed == std::set<std::uint64_t>{1, 2, 3});CHECK(state.lots[0].occupied() == 0);
}
TEST(parkinglot, pl9_no_lot_runs_as_before) {
    // The lot fields exist but are empty: the straight scenario runs exactly its old event stream,
    // which core.deterministic_replay and the frozen baselines pin; here, one tick's state shape.
    const auto s = test::straight();
    auto state = createSimulation(s, 42);
    CHECK(state.lots.empty());
    state = stepSimulation(std::move(state));
    CHECK(state.lots.empty());
}
TEST(parkinglot, validation) {
    const auto rejects = [](const std::function<void(Scenario&)>& change, const std::string& path) {
        auto s = lotScenario(2, 10, 100);change(s);
        const auto issues = validateScenario(s);
        if (std::none_of(issues.begin(), issues.end(), [&](const auto& i) { return i.code == "PARKING_LOT_INVALID" && i.path == path; }))
            throw std::runtime_error("missing PARKING_LOT_INVALID at " + path);
    };
    CHECK(validateScenario(lotScenario(2, 10, 100)).empty()); // forcing: the base is valid
    rejects([](auto& s) { s.parkingLots[0].capacity = 0; }, "parkingLots[0].capacity");
    rejects([](auto& s) { s.parkingLots[0].initialOccupancy = 3; s.parkingLots[0].initialTypes = {{"car", 1}}; }, "parkingLots[0].initialOccupancy");
    rejects([](auto& s) { s.parkingLots[0].initialOccupancy = 1; }, "parkingLots[0].initialTypes");
    rejects([](auto& s) { s.parkingLots[0].initialOccupancy = 1; s.parkingLots[0].initialTypes = {{"bus?", 1}}; }, "parkingLots[0].initialTypes[0]");
    rejects([](auto& s) { s.parkingLots[0].gateServiceTime = -1; }, "parkingLots[0].gateServiceTime");
    rejects([](auto& s) { s.parkingLots[0].dwell = {{10, 0}, {5, 1}}; }, "parkingLots[0].dwell");
    rejects([](auto& s) { s.parkingLots[0].dwell = {{10, 0}, {20, 0.5}}; }, "parkingLots[0].dwell");
    rejects([](auto& s) { s.parkingLots[0].dwell = {}; }, "parkingLots[0].dwell");
    rejects([](auto& s) { s.parkingLots[0].gateRouteIds = {"nowhere"}; }, "parkingLots[0].gateRouteIds[0]");
    rejects([](auto& s) { s.parkingLots[0].exits = {}; }, "parkingLots[0].exits");
    rejects([](auto& s) { s.parkingLots[0].exits[0].routeId = "nowhere"; }, "parkingLots[0].exits[0]");
    rejects([](auto& s) { s.parkingLots[0].exits[0].inputId = "arrivals"; }, "parkingLots[0].exits[0]"); // other source
    rejects([](auto& s) { s.parkingLots[0].exits[0].weight = 0; }, "parkingLots[0].exits[0]");
    rejects([](auto& s) { auto twin = s.parkingLots[0]; twin.id = "twin"; s.parkingLots.push_back(twin); }, "parkingLots[1].gateRouteIds[0]");
    rejects([](auto& s) { s.parkingLots.push_back(s.parkingLots[0]); }, "parkingLots[1].id");
}
