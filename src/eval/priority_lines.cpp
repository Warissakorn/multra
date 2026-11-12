#include "priority_lines.hpp"
#include <algorithm>
#include <cmath>

// D528 (docs/reference/PRIORITY_LINES.md): the priority-line counter.
namespace multra {
PriorityLineAccumulator::PriorityLineAccumulator(const EvaluationSpec& spec)
    : beginSpeed_(spec.queue.beginSpeed), maxGap_(spec.queue.maxGap), warmup_(spec.warmup), end_(spec.end) {}
void PriorityLineAccumulator::observe(const SimState& state) {
    if (!state.scenario) return;
    const auto& s = *state.scenario;
    if (bound_ != &s) {
        bound_ = &s; duration_ = s.duration;
        zones_.clear();
        for (const auto& z : s.conflictZones) zones_.push_back({z.id, z.control, {}, {}, 0});
        previous_.clear(); done_.clear(); queuedSince_.clear();
    }
    const double dt = s.timeStep, t = state.time;
    std::set<std::uint64_t> departed, remapped;
    for (const auto& event : state.events) {
        if (const auto* d = std::get_if<DepartedEvent>(&event)) departed.insert(d->vehicleId);
        if (const auto* r = std::get_if<RoutingEvent>(&event)) remapped.insert(r->vehicleId);
        if (const auto* c = std::get_if<LaneChangeEvent>(&event)) remapped.insert(c->vehicleId);
    }
    // Queued (Vissim's condition): standing short of the line, within maxGap of it or of the rear of
    // the next minor vehicle ahead, all measured as distance to the line.
    struct Short { double remaining{}, length{}; std::uint64_t id{}; bool standing{}; };
    std::vector<std::vector<Short>> shortOf(zones_.size());
    for (const auto& v : state.vehicles)
        for (const auto& z : state.index->routeZones[v.routeIndex])
            if (z.role == ZoneRole::minor && v.distance < z.waitAt)
                shortOf[z.zoneIndex].push_back({z.waitAt - v.distance, s.vehicleTypes[v.typeIndex].length, v.id, v.speed < beginSpeed_});
    for (std::size_t i = 0; i < shortOf.size(); ++i) {
        auto& q = shortOf[i];
        std::sort(q.begin(), q.end(), [](const auto& a, const auto& b) { return a.remaining < b.remaining || (a.remaining == b.remaining && a.id < b.id); });
        for (std::size_t k = 0; k < q.size(); ++k) {
            if (!q[k].standing) continue;
            const double gap = k == 0 ? q[k].remaining : q[k].remaining - q[k - 1].remaining - q[k - 1].length;
            if (gap <= maxGap_) queuedSince_.try_emplace({q[k].id, i}, t);
        }
    }
    std::map<std::uint64_t, Position> now;
    for (const auto& v : state.vehicles) {
        now.emplace(v.id, Position{static_cast<std::uint32_t>(v.routeIndex), v.distance});
        auto prior = previous_.find(v.id);
        std::optional<Position> from;
        if (prior != previous_.end()) from = prior->second;
        else if (departed.contains(v.id)) from = Position{static_cast<std::uint32_t>(v.routeIndex), 0.0};
        const bool proved = from && from->route == v.routeIndex && !remapped.contains(v.id);
        for (const auto& z : state.index->routeZones[v.routeIndex]) {
            const int role = z.role == ZoneRole::minor ? 0 : 1;
            const double at = role == 0 ? z.waitAt : z.entryAt;
            auto& zone = zones_[z.zoneIndex];
            const auto key = std::tuple{v.id, z.zoneIndex, role};
            if (v.distance < at || done_.contains(key) || !from) continue;
            done_.insert(key);
            if (!proved) { ++zone.excluded; continue; } // a route change cannot prove the passage
            if (from->distance >= at) continue; // already past when first seen
            const double moved = v.distance - from->distance;
            const double time = moved > 0 ? t - dt + dt * (at - from->distance) / moved : t;
            if (role == 1) { zone.major.push_back(time); continue; }
            const auto queued = queuedSince_.find({v.id, z.zoneIndex});
            zone.minor.push_back({time, queued == queuedSince_.end() ? std::nullopt : std::optional{queued->second}});
        }
    }
    // Forget vehicles that left: their ids never return.
    std::erase_if(done_, [&](const auto& k) { return !now.contains(std::get<0>(k)); });
    std::erase_if(queuedSince_, [&](const auto& q) { return !now.contains(q.first.first); });
    previous_ = std::move(now);
}
PriorityLineReport PriorityLineAccumulator::report() const {
    PriorityLineReport r;
    if (!bound_) return r;
    const double end = end_.value_or(duration_), window = std::max(0.0, end - warmup_);
    const auto inWindow = [&](double time) { return time >= warmup_ && time <= end; };
    for (std::size_t i = 0; i < zones_.size(); ++i) {
        auto minor = zones_[i].minor;
        std::stable_sort(minor.begin(), minor.end(), [](const auto& a, const auto& b) { return a.time < b.time; });
        auto major = zones_[i].major;
        std::sort(major.begin(), major.end());
        PriorityLineRow row{zones_[i].id, zones_[i].control, window};
        row.excluded = zones_[i].excluded;
        for (const auto& c : minor) row.minorCrossed += inWindow(c.time);
        for (const double m : major) row.majorEntered += inWindow(m);
        if (window > 0) {
            row.minorPerHour = static_cast<double>(row.minorCrossed) * 3600 / window;
            row.majorPerHour = static_cast<double>(row.majorEntered) * 3600 / window;
        }
        std::vector<double> gaps;
        for (std::size_t k = 1; k < minor.size(); ++k) {
            const auto& leader = minor[k - 1]; const auto& follower = minor[k];
            if (!inWindow(leader.time) || !inWindow(follower.time)) continue;
            if (!follower.queuedSince || *follower.queuedSince > leader.time) continue;
            // No major entry strictly between the two departures: the same major gap.
            const auto between = std::upper_bound(major.begin(), major.end(), leader.time);
            if (between != major.end() && *between < follower.time) continue;
            gaps.push_back(follower.time - leader.time);
        }
        row.followUps = gaps.size();
        if (!gaps.empty()) {
            double sum = 0;
            for (const double g : gaps) sum += g;
            const double mean = sum / static_cast<double>(gaps.size());
            row.followUpMean = mean;
            if (gaps.size() >= 2) {
                double squares = 0;
                for (const double g : gaps) squares += (g - mean) * (g - mean);
                row.followUpSd = std::sqrt(squares / static_cast<double>(gaps.size() - 1));
            }
        }
        r.rows.push_back(std::move(row));
    }
    return r;
}
}
