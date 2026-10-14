#include "sections.hpp"
#include <algorithm>
#include <cmath>
#include <map>

namespace trafficsim {
namespace {
const double none = std::nan("");
// Interpolated time a front moving from `before` to `after` reaches a line at `at`/`atAfter` (the
// line's distance on the route held before and after the step), or NaN when it does not.
// `entry` lets a line exactly at the entry distance count, as a vehicle placed there crossed it.
double crossing(double before, double at, double after, double atAfter, double t0, double t1, bool entry) {
    if (std::isnan(at) || std::isnan(atAfter)) return none;
    if (!(entry ? before <= at : before < at) || after < atAfter) return none;
    const double ahead = at - before, past = after - atAfter;
    return ahead + past > 0 ? t0 + (t1 - t0) * ahead / (ahead + past) : t0;
}
}
SectionAccumulator::SectionAccumulator(std::vector<SectionSpec> sections, double warmup, std::optional<double> end, bool byRelease)
    : sections_(std::move(sections)), warmup_(warmup), periodEnd_(end), byRelease_(byRelease),
      count_(sections_.size()), travel_(sections_.size()), delay_(sections_.size()) {}
void SectionAccumulator::bind(const SimState& state) {
    if (bound_ == state.scenario.get()) return;
    bound_ = state.scenario.get();
    const auto& s = *state.scenario;
    const auto& parts = state.index->parts;
    routeLength_.assign(s.routes.size(), 0);
    for (std::size_t r = 0; r < s.routes.size(); ++r)
        if (!parts[r].empty()) routeLength_[r] = parts[r].back().start + parts[r].back().length;
    // The first part of the route on any of the line's lanes, as a queue-counter line binds.
    const auto at = [&](const std::vector<CounterLine>& line, std::size_t r) {
        for (const auto& part : parts[r])
            for (const auto& place : line)
                if (part.segmentId == place.segmentId) return part.start + place.position;
        return none;
    };
    start_.assign(sections_.size(), {}); finish_.assign(sections_.size(), {});
    for (std::size_t k = 0; k < sections_.size(); ++k)
        for (std::size_t r = 0; r < s.routes.size(); ++r) {
            const double a = at(sections_[k].start, r), b = at(sections_[k].end, r);
            const bool applies = std::isnan(a) || std::isnan(b) || a < b;
            start_[k].push_back(applies ? a : none); finish_[k].push_back(applies ? b : none);
        }
}
bool SectionAccumulator::inPeriod(double time) const {
    constexpr double slack = 1e-9; // the period's bounds, as MovementAccumulator applies them
    return time >= warmup_ - slack && (!periodEnd_ || time <= *periodEnd_ + slack);
}
void SectionAccumulator::close(std::size_t k, Track& track, std::uint32_t route, double time) {
    const double opened = track.open[k];
    track.open[k] = none;
    if (!inPeriod(byRelease_ ? track.scheduledTime : time)) return;
    const double travel = time - opened;
    const double freeFlow = (finish_[k][route] - start_[k][route]) / track.desiredSpeed;
    if (!std::isfinite(freeFlow) || freeFlow < 0) return; // no valid ordered binding on this route
    ++count_[k]; travel_[k] += travel; delay_[k] += std::max(0.0, travel - freeFlow);
}
void SectionAccumulator::observe(const SimState& state) {
    if (sections_.empty() || !state.scenario) return;
    bind(state);
    const auto byId = [](const Track& t, std::uint64_t id) { return t.id < id; };
    // A vehicle's step, from its last observed state (or its entry) to now, for every section.
    const auto advance = [&](Track& t, std::uint32_t route, double distance, double time, bool entry) {
        for (std::size_t k = 0; k < sections_.size(); ++k) {
            const double from = entry ? 0 : t.distance;
            // Positioned routing keeps physical prefix coordinates. A line absent from the old
            // provisional suffix may bind on the selected route, but can only be crossed by
            // forward motion; merely appearing behind the front never opens/closes a trip.
            const bool routed = std::any_of(state.events.begin(), state.events.end(), [&](const auto& e) {
                const auto* r = std::get_if<RoutingEvent>(&e); return r && r->vehicleId == t.id;
            });
            const double a = std::isnan(start_[k][t.route]) && routed ? start_[k][route] : start_[k][t.route];
            const double b = std::isnan(finish_[k][t.route]) && routed ? finish_[k][route] : finish_[k][t.route];
            const double started = crossing(from, a, distance, start_[k][route], t.time, time, entry);
            if (!std::isnan(started)) t.open[k] = started;
            const double ended = crossing(from, b, distance, finish_[k][route], t.time, time, entry);
            if (!std::isnan(ended) && !std::isnan(t.open[k])) close(k, t, route, ended);
        }
    };
    std::map<std::uint64_t, const ArrivedEvent*> arrived;
    std::map<std::uint64_t, const DepartedEvent*> departed;
    for (const auto& event : state.events) {
        if (const auto* a = std::get_if<ArrivedEvent>(&event)) arrived[a->vehicleId] = a;
        if (const auto* d = std::get_if<DepartedEvent>(&event)) departed[d->vehicleId] = d;
    }
    const auto routeSlot = [&](const std::string& id) {
        const auto& routes = state.scenario->routes;
        const auto it = std::find_if(routes.begin(), routes.end(), [&](const auto& r) { return r.id == id; });
        return static_cast<std::uint32_t>(it - routes.begin());
    };
    std::vector<Track> next;
    next.reserve(state.vehicles.size());
    std::vector<bool> seen(tracks_.size());
    for (const auto& v : state.vehicles) {
        const auto it = std::lower_bound(tracks_.begin(), tracks_.end(), v.id, byId);
        const bool known = it != tracks_.end() && it->id == v.id;
        Track t;
        if (known) { seen[static_cast<std::size_t>(it - tracks_.begin())] = true; t = std::move(*it); }
        else t = {v.id, v.routeIndex, 0, 0, v.enteredTime, v.desiredSpeed, std::vector<double>(sections_.size(), none), v.scheduledTime};
        advance(t, v.routeIndex, v.distance, state.time, !known);
        t.route = v.routeIndex; t.distance = v.distance; t.speed = v.speed; t.time = state.time;
        next.push_back(std::move(t));
    }
    // A vehicle that left this step: it moved at least to its route's end, at its last speed.
    for (std::size_t i = 0; i < tracks_.size(); ++i) {
        auto& t = tracks_[i];
        if (seen[i] || !arrived.contains(t.id)) continue;
        const auto route = routeSlot(arrived.at(t.id)->routeId);
        if (route >= routeLength_.size()) continue;
        const double dt = state.time - t.time;
        advance(t, route, std::max(routeLength_[route], t.distance + std::max(0.0, t.speed) * dt), state.time, false);
    }
    // A source-sink trip can be absent from both snapshots. Its departure supplies the entry
    // time/speed and the terminal event proves the route end, without inventing an active vehicle.
    for (const auto& [id, a] : arrived) {
        if (!departed.contains(id) || std::binary_search(tracks_.begin(), tracks_.end(), Track{id},
                [](const Track& x, const Track& y) { return x.id < y.id; })) continue;
        const auto& d = *departed.at(id);
        const auto from = routeSlot(d.routeId), to = routeSlot(a->routeId);
        if (from >= routeLength_.size() || to >= routeLength_.size()) continue;
        Track t{id, from, 0, d.desiredSpeed, d.time, d.desiredSpeed,
                std::vector<double>(sections_.size(), none), d.scheduledTime};
        advance(t, to, routeLength_[to], a->time, true);
    }
    // The engine keeps its fleet in id order; a hand-built state need not.
    std::sort(next.begin(), next.end(), [](const Track& a, const Track& b) { return a.id < b.id; });
    tracks_ = std::move(next);
}
std::vector<SectionRow> SectionAccumulator::report() const {
    std::vector<SectionRow> rows;
    for (std::size_t k = 0; k < sections_.size(); ++k) {
        SectionRow row{sections_[k].name, count_[k], {}, {}, 0, sections_[k].controlType, sections_[k].approach};
        if (count_[k]) {
            row.meanTravelTime = travel_[k] / static_cast<double>(count_[k]);
            row.meanDelay = delay_[k] / static_cast<double>(count_[k]);
        }
        for (const auto& t : tracks_)
            if (!std::isnan(t.open[k]) && !std::isnan(finish_[k][t.route]) && finish_[k][t.route] >= t.distance && (!byRelease_ || inPeriod(t.scheduledTime))) ++row.unfinished;
        rows.push_back(std::move(row));
    }
    return rows;
}
}
