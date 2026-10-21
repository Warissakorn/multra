#include "sections.hpp"
#include <algorithm>
#include <cmath>
#include <map>

namespace multra {
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
    piecewise_ = !s.speedDecisions.empty() || !s.reducedSpeedAreas.empty();
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
            const bool reversed = !std::isnan(a) && !std::isnan(b) && a >= b;
            // A provisional route may contain the start and choose its end only later.
            start_[k].push_back(reversed ? none : a); finish_[k].push_back(reversed ? none : b);
        }
    sectionsOfRoute_.assign(s.routes.size(), {});
    for (std::size_t r = 0; r < s.routes.size(); ++r)
        for (std::size_t k = 0; k < sections_.size(); ++k)
            if (!std::isnan(start_[k][r]) || !std::isnan(finish_[k][r])) sectionsOfRoute_[r].push_back(k);
}
bool SectionAccumulator::inPeriod(double time) const {
    constexpr double slack = 1e-9; // the period's bounds, as MovementAccumulator applies them
    return time >= warmup_ - slack && (!periodEnd_ || time <= *periodEnd_ + slack);
}
void SectionAccumulator::close(std::size_t k, Track& track, double time, double travelled, double freeFlowAt) {
    const double opened = track.open[k];
    track.open[k] = none;
    if (!inPeriod(byRelease_ ? track.scheduledTime : time)) return;
    const double travel = time - opened;
    const double freeFlow = piecewise_ ? freeFlowAt - track.openedAtFreeFlow[k]
                                       : (travelled - track.openedAtDistance[k]) / track.desiredSpeed;
    ++count_[k]; travel_[k] += travel; delay_[k] += std::max(0.0, travel - freeFlow);
}
void SectionAccumulator::observe(const SimState& state) {
    if (sections_.empty() || !state.scenario) return;
    bind(state);
    const auto slot = [&](const std::string& id) {
        const auto& routes = state.scenario->routes;
        return static_cast<std::uint32_t>(std::find_if(routes.begin(), routes.end(),
            [&](const auto& r) { return r.id == id; }) - routes.begin());
    };
    std::map<std::uint64_t, DepartedEvent> departed;
    std::map<std::uint64_t, const ArrivedEvent*> arrived;
    std::map<std::uint64_t, std::vector<const LaneChangeEvent*>> changes;
    for (const auto& event : state.events) {
        if (const auto* d = std::get_if<DepartedEvent>(&event)) departed[d->vehicleId] = *d;
        if (const auto* a = std::get_if<ArrivedEvent>(&event)) arrived[a->vehicleId] = a;
        if (const auto* c = std::get_if<LaneChangeEvent>(&event)) changes[c->vehicleId].push_back(c);
    }
    for (const auto& event : state.events) if (const auto* r = std::get_if<RoutingEvent>(&event)) {
        const auto d = departed.find(r->vehicleId);
        if (d != departed.end() && r->time == d->second.time && d->second.routeId == r->fromRouteId)
            d->second.routeId = r->toRouteId; // source-zero recognition precedes motion
    }
    const auto makeTrack = [&](std::uint64_t id, std::uint32_t route, double time, double desired, double scheduled) {
        Track t{id, route, 0, 0, time, desired, std::vector<double>(sections_.size(), none), scheduled};
        t.openedAtDistance.assign(sections_.size(), none); t.openedAtFreeFlow.assign(sections_.size(), none);
        return t;
    };
    const auto byId = [](const Track& t, std::uint64_t id) { return t.id < id; };
    // A vehicle's step, from its last observed state (or its entry) to now, for every section.
    const auto advance = [&](Track& t, std::uint32_t route, double distance, double time, bool entry) {
        double from = entry ? 0 : t.distance;
        auto fromRoute = t.route;
        if (const auto c = changes.find(t.id); c != changes.end()) for (const auto* change : c->second) {
            if (slot(change->fromRouteId) != fromRoute) continue;
            const auto to = slot(change->toRouteId);
            std::optional<double> mapped;
            const auto consider = [&](const auto& spans) {
                const double rear = from - state.scenario->vehicleTypes[t.type].length;
                for (const auto& span : spans) if (span.target == to && rear >= span.fromStart - 1e-9 && from <= span.fromEnd + 1e-9) {
                    const double value = mappedOnto(span, from);
                    if (mapped && std::abs(*mapped - value) > 1e-9) return false;
                    mapped = value;
                }
                return true;
            };
            if (!consider(state.index->laneChangesOfRoute[fromRoute]) ||
                !consider(state.index->discretionaryOfRoute[fromRoute]) || !mapped) {
                std::fill(t.open.begin(), t.open.end(), none); return;
            }
            from = *mapped; fromRoute = to; // mapping coordinates does not add travelled metres
        }
        const double moved = std::max(0.0, distance - from);
        for (const std::size_t k : sectionsOfRoute_[route]) {
            const auto passage = [&](const auto& lines) {
                const double before = std::isnan(lines[k][fromRoute]) ? lines[k][route] : lines[k][fromRoute];
                return crossing(from, before, distance, lines[k][route], t.time, time, entry);
            };
            const auto travelledAt = [&](double at) {
                const double fraction = time > t.time ? std::clamp((at - t.time) / (time - t.time), 0.0, 1.0) : 0;
                return t.travelled + moved * fraction;
            };
            const auto freeFlowAt = [&](double travelled) { return t.freeFlowAt + (travelled - t.travelled) / t.desiredSpeed; };
            const double started = passage(start_);
            if (!std::isnan(started)) {
                t.open[k] = started; t.openedAtDistance[k] = travelledAt(started); t.openedAtFreeFlow[k] = freeFlowAt(t.openedAtDistance[k]);
            }
            const double ended = passage(finish_);
            if (!std::isnan(ended) && !std::isnan(t.open[k])) close(k, t, ended, travelledAt(ended), freeFlowAt(travelledAt(ended)));
        }
        t.travelled += moved; t.freeFlowAt += moved / t.desiredSpeed;
    };
    // Known tracks advance in place and new vehicles' tracks wait in `added_`, so a tick moves no
    // Track; the calls into close() keep the fleet's order, so the sums are unchanged (D279).
    added_.clear();
    seen_.assign(tracks_.size(), 0);
    for (const auto& v : state.vehicles) {
        const auto it = std::lower_bound(tracks_.begin(), tracks_.end(), v.id, byId);
        const bool known = it != tracks_.end() && it->id == v.id;
        if (known) seen_[static_cast<std::size_t>(it - tracks_.begin())] = 1;
        else added_.push_back(makeTrack(v.id, v.routeIndex, v.enteredTime, v.desiredSpeed, v.scheduledTime));
        Track& t = known ? *it : added_.back();
        t.type = v.typeIndex;
        advance(t, v.routeIndex, v.distance, state.time, !known);
        if (piecewise_) t.desiredSpeed = v.areaSpeed > 0 && v.areaSpeed < v.desiredSpeed ? v.areaSpeed : v.desiredSpeed; // in force (§4, §7.3)
        t.route = v.routeIndex; t.distance = v.distance; t.speed = v.speed; t.time = state.time;
    }
    // Terminal evidence brackets the final crossing at the selected route's end (within one tick).
    for (std::size_t i = 0; i < tracks_.size(); ++i) {
        auto& t = tracks_[i];
        if (seen_[i] || !arrived.contains(t.id)) continue;
        const auto route = slot(arrived.at(t.id)->routeId);
        advance(t, route, routeLength_[route], state.time, false);
    }
    for (const auto& [id, arrival] : arrived) {
        if (std::any_of(tracks_.begin(), tracks_.end(), [&](const auto& t) { return t.id == id; }) ||
            std::any_of(added_.begin(), added_.end(), [&](const auto& t) { return t.id == id; })) continue;
        const auto d = departed.find(id);
        if (d == departed.end()) continue;
        auto t = makeTrack(id, slot(d->second.routeId), d->second.time, d->second.desiredSpeed, d->second.scheduledTime);
        const auto route = slot(arrival->routeId);
        advance(t, route, routeLength_[route], arrival->time, true);
    }
    // The tracks of vehicles still in the fleet, in id order: the engine adds vehicles in id order,
    // and a hand-built state need not.
    std::size_t kept = 0;
    for (std::size_t i = 0; i < tracks_.size(); ++i)
        if (seen_[i]) { if (kept != i) tracks_[kept] = std::move(tracks_[i]); ++kept; }
    tracks_.resize(kept);
    const auto byTrackId = [](const Track& a, const Track& b) { return a.id < b.id; };
    if (!std::is_sorted(added_.begin(), added_.end(), byTrackId)) std::sort(added_.begin(), added_.end(), byTrackId);
    const bool appended = tracks_.empty() || added_.empty() || tracks_.back().id < added_.front().id;
    for (auto& t : added_) tracks_.push_back(std::move(t));
    if (!appended) std::inplace_merge(tracks_.begin(), tracks_.end() - static_cast<std::ptrdiff_t>(added_.size()), tracks_.end(), byTrackId);
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
            if (!std::isnan(t.open[k]) && !std::isnan(finish_[k][t.route]) && finish_[k][t.route] > t.distance &&
                (!byRelease_ || inPeriod(t.scheduledTime))) ++row.unfinished;
        rows.push_back(std::move(row));
    }
    return rows;
}
}
