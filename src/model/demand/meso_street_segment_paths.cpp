#include "meso_street_segment_paths.hpp"
#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <queue>
#include <stdexcept>

namespace multra {
std::optional<std::size_t> mesoZoneEndpoint(const MesoGraph& graph, const ZoneConnector& c) {
    std::optional<std::size_t> found;
    const auto& source = c.streetSegmentId.empty() ? c.linkId : c.streetSegmentId;
    const bool forward = c.streetSegmentId.empty() || c.streetSegmentForward;
    const bool origin = c.direction == ZoneDirection::origin;
    for (std::size_t i = 0; i < graph.links.size(); ++i) {
        const auto& l = graph.links[i];
        if (l.source != source || (l.toStation > l.fromStation) != forward) continue;
        if (!found) { found = i; continue; }
        const auto& previous = graph.links[*found];
        const double at = origin ? l.fromStation : l.toStation;
        const double before = origin ? previous.fromStation : previous.toStation;
        if ((forward == origin) ? at < before : at > before) found = i;
    }
    return found;
}
std::vector<MesoPath> mesoShortestOdPaths(const MesoGraph& graph, std::size_t from, std::size_t to, const std::string& mode) {
    const auto n = graph.links.size();
    if (from >= n || to >= n) return {};
    const double inf = std::numeric_limits<double>::infinity();
    std::vector<double> free(n, inf), distance(n, inf);
    for (std::size_t i = 0; i < n; ++i) {
        const auto& l = graph.links[i];
        const bool allowed = mode.empty() || l.modes.empty() || std::find(l.modes.begin(), l.modes.end(), mode) != l.modes.end();
        if (allowed && l.length > 0 && l.freeSpeed > 0) {
            const double time = l.length / l.freeSpeed;
            if (std::isfinite(time) && time > 0) free[i] = time;
        }
    }
    if (!std::isfinite(free[from]) || !std::isfinite(free[to])) return {};
    std::vector<std::vector<std::size_t>> ins(n), outs(n);
    for (std::size_t i = 0; i < graph.turns.size(); ++i) {
        const auto& t = graph.turns[i];
        if (t.in >= n || t.out >= n || !std::isfinite(t.freeTime) || t.freeTime < 0) continue;
        ins[t.out].push_back(i); outs[t.in].push_back(i);
    }
    using Entry = std::pair<double, std::size_t>;
    std::priority_queue<Entry, std::vector<Entry>, std::greater<>> queue;
    distance[to] = free[to]; queue.push({distance[to], to});
    while (!queue.empty()) {
        const auto [cost, link] = queue.top(); queue.pop();
        if (cost != distance[link]) continue;
        for (const auto index : ins[link]) {
            const auto& t = graph.turns[index];
            const double next = free[t.in] + t.freeTime + cost;
            if (next < distance[t.in]) { distance[t.in] = next; queue.push({next, t.in}); }
        }
    }
    if (!std::isfinite(distance[from])) return {};
    // Iterative DFS over the shortest-path edges. The on-path guard also excludes cycles if
    // adding a very small positive time rounds to the same double at an extreme time scale.
    struct Frame { std::size_t link{}, next{}; };
    std::vector<Frame> stack{{from, 0}};
    std::vector<bool> onPath(n); onPath[from] = true;
    MesoPath path; path.links.push_back(from);
    std::vector<MesoPath> result;
    const auto pop = [&] {
        onPath[stack.back().link] = false; stack.pop_back(); path.links.pop_back();
        if (!path.turns.empty()) path.turns.pop_back();
    };
    while (!stack.empty()) {
        auto& frame = stack.back();
        if (frame.link == to) {
            if (result.size() == kMesoOdPathLimit) throw std::length_error("MESO_OD_PATH_LIMIT");
            result.push_back(path); pop(); continue;
        }
        bool descended = false;
        while (frame.next < outs[frame.link].size()) {
            const auto index = outs[frame.link][frame.next++];
            const auto& t = graph.turns[index];
            if (onPath[t.out] || !std::isfinite(distance[t.out]) ||
                free[frame.link] + t.freeTime + distance[t.out] > distance[frame.link] * (1 + kMesoOdTieTolerance)) continue;
            onPath[t.out] = true; path.links.push_back(t.out); path.turns.push_back(index);
            stack.push_back({t.out, 0}); descended = true; break;
        }
        if (!descended) pop();
    }
    return result;
}
}
