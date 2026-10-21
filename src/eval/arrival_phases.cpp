#include "arrival_phases.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace multra {
namespace {
double cycleOf(const Scenario& s) {
    double cycle = 0;
    for (const auto& program : s.signalPrograms) {
        double sum = 0;
        for (const auto& phase : program.phases) sum += phase.duration;
        if (cycle > 0 && std::abs(sum - cycle) > 1e-9) return 0;
        cycle = sum;
    }
    return cycle;
}
}
ArrivalPhaseAccumulator::ArrivalPhaseAccumulator(const EvaluationSpec& spec, double binWidth)
    : movementOfRoute_(spec.movementOfRoute), names_(spec.movementNames), beginSpeed_(spec.queue.beginSpeed), endSpeed_(spec.queue.endSpeed),
      binWidth_(binWidth) {
    if (!std::isfinite(binWidth_) || binWidth_ <= 0)
        throw std::invalid_argument("Arrival phase bin width must be finite and positive");
}
std::size_t ArrivalPhaseAccumulator::bin(double time) const {
    const auto b = static_cast<std::size_t>(std::fmod(time, cycle_) / binWidth_);
    return b < bins_ ? b : bins_ - 1;
}
void ArrivalPhaseAccumulator::observe(const SimState& state) {
    if (cycle_ < 0) {
        const double cycle = cycleOf(*state.scenario);
        const double bins = std::max(1.0, std::ceil(cycle / binWidth_ - 1e-9));
        // Two uint64 histograms per movement/segment row: at most 16 MiB across every row.
        constexpr std::size_t maxCounters = 2 * 1024 * 1024;
        const auto movements = std::max<std::size_t>(1, names_.size());
        const auto segments = std::max<std::size_t>(1, state.scenario->segments.size());
        const auto perRow = maxCounters / 2 / movements / segments;
        if (!std::isfinite(bins) || bins > static_cast<double>(perRow))
            throw std::invalid_argument("Arrival phase bins exceed the 16 MiB histogram limit");
        bins_ = static_cast<std::size_t>(bins);
        cycle_ = cycle; // publish only after validation; a failed bind can be retried safely
    }
    if (cycle_ <= 0) return;
    for (const auto& event : state.events) {
        if (const auto* departed = std::get_if<DepartedEvent>(&event)) {
            for (const auto& route : state.scenario->routes)
                if (route.id == departed->routeId) { open_[departed->vehicleId].entered.emplace_back(route.segmentIds.front(), departed->time); break; }
        } else if (const auto* entered = std::get_if<SegmentEnteredEvent>(&event)) {
            open_[entered->vehicleId].entered.emplace_back(entered->segmentId, entered->time);
        } else if (const auto* arrived = std::get_if<ArrivedEvent>(&event)) {
            const auto trip = open_.find(arrived->vehicleId);
            const auto m = movementOfRoute_.find(arrived->routeId);
            if (m == movementOfRoute_.end()) ++unassigned_;
            else if (trip != open_.end()) {
                const auto row = [&](const std::string& segment) -> Row& {
                    auto& r = rows_[{m->second, segment}];
                    if (r.enteredAt.empty()) { r.enteredAt.assign(bins_, 0); r.firstStopAt.assign(bins_, 0); }
                    return r;
                };
                for (const auto& [segment, time] : trip->second.entered) { auto& r = row(segment); ++r.entered; ++r.enteredAt[bin(time)]; }
                for (const auto& [segment, time] : trip->second.stopped) { auto& r = row(segment); ++r.stopped; ++r.firstStopAt[bin(time)]; }
            }
            if (trip != open_.end()) open_.erase(trip);
        }
    }
    for (const auto& v : state.vehicles) {
        auto& trip = open_[v.id];
        if (v.speed >= endSpeed_) trip.moved = true;
        if (!trip.moved || v.speed >= beginSpeed_) continue;
        // The part its front is on, as the engine locates a vehicle.
        const auto& parts = state.index->parts[v.routeIndex];
        const auto part = std::find_if(parts.begin(), parts.end(), [&](const auto& p) { return v.distance < p.start + p.length; });
        trip.stopped.emplace((part == parts.end() ? parts.back() : *part).segmentId, state.time); // keeps the first
    }
}
ArrivalPhaseReport ArrivalPhaseAccumulator::report() const {
    ArrivalPhaseReport r{cycle_ > 0 ? cycle_ : 0, binWidth_, {}, unassigned_};
    for (const auto& [key, row] : rows_)
        r.rows.push_back({names_[key.first], key.second, row.entered, row.stopped, row.enteredAt, row.firstStopAt});
    return r;
}
}
