#include "lots.hpp"
#include <algorithm>
#include <limits>

namespace trafficsim {
namespace {
constexpr auto npos = std::numeric_limits<std::size_t>::max();
}
LotAccumulator::LotAccumulator(std::vector<LotSpec> lots, double warmup, std::optional<double> end)
    : lots_(std::move(lots)), warmup_(warmup), periodEnd_(end), gateSince_(lots_.size()), leaving_(lots_.size()),
      occupancySum_(lots_.size()), waitSum_(lots_.size()), samples_(lots_.size()) {
    for (const auto& lot : lots_) rows_.push_back({lot.name});
}
bool LotAccumulator::inPeriod(double time) const {
    constexpr double slack = 1e-9;
    return time >= warmup_ - slack && (!periodEnd_ || time <= *periodEnd_ + slack);
}
void LotAccumulator::bind(const SimState& state) {
    if (bound_ == state.scenario.get()) return;
    bound_ = state.scenario.get();
    const auto& lots = state.scenario->parkingLots;
    slot_.assign(lots_.size(), npos);
    for (std::size_t k = 0; k < lots_.size(); ++k)
        for (std::size_t i = 0; i < lots.size(); ++i)
            if (lots[i].id == lots_[k].id) {
                slot_[k] = i; rows_[k].capacity = lots[i].capacity;
                for (const auto& route : lots[i].gateRouteIds) gateOf_[route] = k;
            }
}
void LotAccumulator::observe(const SimState& state) {
    if (lots_.empty() || !state.scenario) return;
    bind(state);
    const auto& scenario = *state.scenario;
    const bool inside = inPeriod(state.time);
    // Admissions: this step's ArrivedEvents on a gate route, timed from the last snapshot's gate.
    for (const auto& event : state.events) {
        const auto* arrived = std::get_if<ArrivedEvent>(&event);
        if (!arrived) continue;
        const auto gate = gateOf_.find(arrived->routeId);
        if (gate == gateOf_.end()) continue;
        const auto k = gate->second;
        if (!inPeriod(arrived->time)) continue;
        double wait = 0;
        if (const auto at = gateSince_[k].find(arrived->vehicleId); at != gateSince_[k].end())
            wait = std::max(0.0, arrived->time - (static_cast<double>(at->second) * scenario.timeStep +
                                                  scenario.parkingLots[slot_[k]].gateServiceTime));
        auto& row = rows_[k];
        ++row.admitted; waitSum_[k] += wait;
        row.gateWaitMax = std::max(row.gateWaitMax.value_or(0), wait);
        row.waitedOverTick += wait > scenario.timeStep + 1e-9;
    }
    for (std::size_t k = 0; k < lots_.size(); ++k) {
        if (slot_[k] == npos || slot_[k] >= state.lots.size()) continue;
        const auto& lot = state.lots[slot_[k]];
        auto& row = rows_[k];
        // Departures: ids the lot held (parked or released) and holds no more -- inserted from the exit
        // queue. A vehicle released and inserted in one step is never seen in `leaving`, so both count.
        std::vector<std::uint64_t> held = lot.leaving;
        for (const auto& p : lot.parked) held.push_back(p.vehicle.id);
        std::sort(held.begin(), held.end());
        if (inside)
            for (const auto id : leaving_[k]) row.departed += !std::binary_search(held.begin(), held.end(), id);
        leaving_[k] = std::move(held);
        gateSince_[k].clear();
        for (const auto& g : lot.gate) gateSince_[k][g.vehicleId] = g.since;
        if (!inside) continue;
        const auto occupied = static_cast<std::uint64_t>(lot.occupied());
        occupancySum_[k] += static_cast<double>(occupied); ++samples_[k];
        row.occupancyPeak = std::max(row.occupancyPeak, occupied);
        row.occupancyEnd = occupied;
    }
}
std::vector<LotRow> LotAccumulator::report(const SimState& end) const {
    auto rows = rows_;
    for (std::size_t k = 0; k < rows.size(); ++k) {
        auto& row = rows[k];
        if (samples_[k]) row.occupancyMean = occupancySum_[k] / static_cast<double>(samples_[k]);
        if (row.admitted) row.gateWaitMean = waitSum_[k] / static_cast<double>(row.admitted);
        if (k < slot_.size() && slot_[k] != npos && slot_[k] < end.lots.size()) {
            const auto& lot = end.lots[slot_[k]];
            row.endParked = lot.parked.size(); row.endAtGate = lot.gate.size(); row.endInExitQueue = lot.leaving.size();
        }
    }
    return rows;
}
std::size_t parkedCount(const SimState& state) {
    std::size_t n = 0;
    for (const auto& lot : state.lots) n += lot.parked.size();
    return n;
}
}
