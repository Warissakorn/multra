#include "parking.hpp"
#include "conflicts.hpp"
#include "speed_decision.hpp"
#include "detail.hpp"
#include "routes.hpp"
#include <algorithm>
#include <cmath>
#include <map>
#include <set>

namespace multra {
namespace {
std::size_t slotOf(const auto& items, const std::string& id) {
    return static_cast<std::size_t>(&detail::byId(items, id) - items.data());
}
double routeLength(const ScenarioIndex& index, std::size_t route) {
    const auto& parts = index.parts[route];
    return parts.back().start + parts.back().length;
}
// One uniform draw, inverted on the cumulative curve; the end rounded up to a tick boundary.
double dwellEnd(SimState& state, const ParkingLotSpec& lot, double admitted) {
    const double u = detail::random(state.randomState);
    const auto& d = lot.dwell;
    double seconds = d.front().seconds;
    for (std::size_t i = 1; i < d.size() && u > d[i - 1].cumulative; ++i)
        if (u <= d[i].cumulative) {
            seconds = d[i - 1].seconds + (u - d[i - 1].cumulative) / (d[i].cumulative - d[i - 1].cumulative) *
                                             (d[i].seconds - d[i - 1].seconds);
            break;
        }
    const double dt = state.scenario->timeStep;
    return std::ceil((admitted + seconds) / dt - 1e-9) * dt;
}
void park(SimState& state, std::size_t lot, const PendingVehicle& vehicle, double admitted) {
    auto& parked = state.lots[lot].parked;
    const ParkedVehicle entry{vehicle, admitted, dwellEnd(state, state.scenario->parkingLots[lot], admitted)};
    parked.insert(std::upper_bound(parked.begin(), parked.end(), entry, [](const auto& a, const auto& b) {
        return a.dwellEnd == b.dwellEnd ? a.vehicle.id < b.vehicle.id : a.dwellEnd < b.dwellEnd;
    }), entry);
}
// The trip into the lot ends at admission, as any trip ends at its route's end.
void arrive(SimState& state, const Vehicle& vehicle, std::size_t route, double time) {
    const double length = routeLength(*state.index, route);
    ++state.completed;
    state.events.emplace_back(ArrivedEvent{time, vehicle.id, state.scenario->routes[route].id,
        time - vehicle.enteredTime, vehicle.enteredTime - vehicle.scheduledTime, freeFlowTime(vehicle, length)});
}
std::size_t lotOf(const SimState& state, std::size_t route) {
    return state.index->lotOfRoute.empty() ? SIZE_MAX : state.index->lotOfRoute[route];
}
std::uint64_t serviceTicks(const SimState& state, const ParkingLotSpec& lot) {
    return static_cast<std::uint64_t>(std::ceil(lot.gateServiceTime / state.scenario->timeStep - 1e-9));
}
}

void initializeLots(SimState& state) {
    const auto& scenario = *state.scenario;
    if (scenario.parkingLots.empty()) return;
    state.lots.resize(scenario.parkingLots.size());
    for (std::size_t k = 0; k < scenario.parkingLots.size(); ++k) {
        const auto& lot = scenario.parkingLots[k];
        double total = 0;
        for (const auto& t : lot.initialTypes) total += t.share;
        for (std::uint32_t n = 0; n < lot.initialOccupancy; ++n) {
            // Type first, then the vehicle's own draws, then its dwell: one fixed order.
            const double u = detail::random(state.randomState) * total;
            std::size_t pick = 0;
            for (double sum = lot.initialTypes[0].share; pick + 1 < lot.initialTypes.size() && u >= sum;)
                sum += lot.initialTypes[++pick].share;
            const auto type = static_cast<std::uint32_t>(slotOf(scenario.vehicleTypes, lot.initialTypes[pick].vehicleTypeId));
            const auto vehicle = detail::samplePending(state, PendingVehicle::kNoInput, 0, type, 0);
            park(state, k, vehicle, 0);
        }
    }
}
void releaseDepartures(SimState& state) {
    const auto& scenario = *state.scenario;
    for (std::size_t k = 0; k < state.lots.size(); ++k) {
        auto& lot = state.lots[k];
        const auto& spec = scenario.parkingLots[k];
        while (!lot.parked.empty() && lot.parked.front().dwellEnd <= state.time + 1e-9) {
            auto vehicle = lot.parked.front().vehicle;
            const double due = lot.parked.front().dwellEnd;
            lot.parked.erase(lot.parked.begin());
            // One draw per departure only when there is a choice.
            std::size_t pick = 0;
            if (spec.exits.size() > 1) {
                double total = 0;
                for (const auto& e : spec.exits) total += e.weight;
                const double u = detail::random(state.randomState) * total;
                for (double sum = spec.exits[0].weight; pick + 1 < spec.exits.size() && u >= sum;) sum += spec.exits[++pick].weight;
            }
            const auto& exit = spec.exits[pick];
            vehicle.routeIndex = static_cast<std::uint32_t>(slotOf(scenario.routes, exit.routeId));
            vehicle.inputIndex = static_cast<std::uint32_t>(slotOf(scenario.inputs, exit.inputId));
            vehicle.scheduledTime = due;
            auto& queue = state.inputs[vehicle.inputIndex].queue;
            queue.insert(std::upper_bound(queue.begin(), queue.end(), vehicle, [](const auto& a, const auto& b) {
                return a.scheduledTime == b.scheduledTime ? a.id < b.id : a.scheduledTime < b.scheduledTime;
            }), vehicle);
            lot.leaving.push_back(vehicle.id);
        }
    }
}
void noteInserted(SimState& state, std::uint64_t vehicleId) {
    for (auto& lot : state.lots) std::erase(lot.leaving, vehicleId);
}
void admitAtGates(SimState& state, std::vector<Vehicle>& vehicles, std::uint64_t startTick, double startTime) {
    if (state.lots.empty()) return;
    const auto& scenario = *state.scenario;
    const auto& index = *state.index;
    for (std::size_t k = 0; k < state.lots.size(); ++k) {
        auto& lot = state.lots[k];
        const auto& spec = scenario.parkingLots[k];
        // Who stands at the line now, keeping the tick each one first came to it.
        std::map<std::uint64_t, std::uint64_t> before;
        for (const auto& g : lot.gate) before[g.vehicleId] = g.since;
        lot.gate.clear();
        for (const auto& v : vehicles) {
            if (lotOf(state, v.routeIndex) != k) continue;
            if (!atStopLine(v, routeLength(index, v.routeIndex), scenario.behaviours[effectiveBehaviour(index, v)])) continue;
            const auto found = before.find(v.id);
            lot.gate.push_back({v.id, found == before.end() ? startTick : found->second});
        }
        std::sort(lot.gate.begin(), lot.gate.end(), [](const auto& a, const auto& b) {
            return a.since == b.since ? a.vehicleId < b.vehicleId : a.since < b.since;
        });
        const auto service = serviceTicks(state, spec);
        while (!lot.gate.empty() && lot.occupied() < spec.capacity && startTick - lot.gate.front().since >= service) {
            const auto id = lot.gate.front().vehicleId;
            lot.gate.erase(lot.gate.begin());
            const auto at = std::find_if(vehicles.begin(), vehicles.end(), [&](const auto& v) { return v.id == id; });
            arrive(state, *at, at->routeIndex, startTime);
            park(state, k, *at, startTime);
            vehicles.erase(at);
        }
    }
}
std::optional<double> gateHold(const SimState& state, const Vehicle& vehicle, std::size_t route) {
    const auto k = lotOf(state, route);
    if (k == SIZE_MAX) return std::nullopt;
    const auto& spec = state.scenario->parkingLots[k];
    if (state.lots[k].occupied() < spec.capacity && spec.gateServiceTime <= 0) return std::nullopt;
    return std::max(0.0, routeLength(*state.index, route) - vehicle.distance);
}
bool gateArrival(SimState& state, const Vehicle& vehicle, std::size_t route, double time, bool held) {
    const auto k = lotOf(state, route);
    if (held || state.lots[k].occupied() >= state.scenario->parkingLots[k].capacity) return false;
    arrive(state, vehicle, route, time);
    park(state, k, vehicle, time);
    return true;
}
void validateParkingLots(const Scenario& s, std::vector<ValidationIssue>& issues) {
    const auto add = [&](const std::string& path) { issues.push_back({"PARKING_LOT_INVALID", path}); };
    const auto find = [](const auto& items, const std::string& id) {
        return std::find_if(items.begin(), items.end(), [&](const auto& item) { return item.id == id; });
    };
    std::set<std::string> ids, gates;
    for (std::size_t k = 0; k < s.parkingLots.size(); ++k) {
        const auto& lot = s.parkingLots[k];
        const auto p = "parkingLots[" + std::to_string(k) + "]";
        if (lot.id.empty() || !ids.insert(lot.id).second) add(p + ".id");
        if (lot.capacity < 1) add(p + ".capacity");
        if (lot.initialOccupancy > lot.capacity) add(p + ".initialOccupancy");
        if (lot.initialOccupancy > 0 && lot.initialTypes.empty()) add(p + ".initialTypes");
        for (std::size_t j = 0; j < lot.initialTypes.size(); ++j)
            if (find(s.vehicleTypes, lot.initialTypes[j].vehicleTypeId) == s.vehicleTypes.end() ||
                !std::isfinite(lot.initialTypes[j].share) || lot.initialTypes[j].share <= 0)
                add(p + ".initialTypes[" + std::to_string(j) + "]");
        if (!std::isfinite(lot.gateServiceTime) || lot.gateServiceTime < 0) add(p + ".gateServiceTime");
        bool dwell = !lot.dwell.empty() && lot.dwell.back().cumulative == 1;
        for (std::size_t i = 0; i < lot.dwell.size(); ++i) {
            const auto& d = lot.dwell[i];
            dwell = dwell && std::isfinite(d.seconds) && d.seconds >= 0 && d.cumulative >= 0 && d.cumulative <= 1 &&
                    (i == 0 || (d.seconds > lot.dwell[i - 1].seconds && d.cumulative > lot.dwell[i - 1].cumulative));
        }
        if (!dwell) add(p + ".dwell");
        // No gate route is legal: a lot that only empties (initial occupants, nobody driving in).
        for (std::size_t j = 0; j < lot.gateRouteIds.size(); ++j)
            if (find(s.routes, lot.gateRouteIds[j]) == s.routes.end() || !gates.insert(lot.gateRouteIds[j]).second)
                add(p + ".gateRouteIds[" + std::to_string(j) + "]");
        if (lot.exits.empty()) add(p + ".exits");
        for (std::size_t j = 0; j < lot.exits.size(); ++j) {
            const auto& e = lot.exits[j];
            const auto route = find(s.routes, e.routeId);
            const auto input = find(s.inputs, e.inputId);
            const auto source = input == s.inputs.end() ? s.routes.end() : find(s.routes, input->routeId);
            // The departure waits in that input's queue, so it must start where the input starts.
            if (route == s.routes.end() || source == s.routes.end() || route->segmentIds.empty() ||
                source->segmentIds.empty() || route->segmentIds.front() != source->segmentIds.front() ||
                !std::isfinite(e.weight) || e.weight <= 0)
                add(p + ".exits[" + std::to_string(j) + "]");
        }
    }
}
}
