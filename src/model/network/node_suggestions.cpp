#include "node_suggestions.hpp"
#include "polygon.hpp"
#include "right_of_way.hpp"
#include <algorithm>
#include <cmath>
#include <map>
#include <numeric>
#include <set>
#include <tuple>

namespace trafficsim {
namespace {
// `sources` are the Links traffic reaches the conflicts from: a Connector's `from` Link, or the
// Link a side lies on when no Connector of the cluster arrives on it at or before that side. One
// source is a lane added or dropped on one road, not a junction.
struct Cluster {
    std::vector<Point> points; double pad{}; std::size_t order{}; std::set<std::string> sources;
    std::vector<ConflictSide> linkSides; std::map<std::string, double> arrivals; // Link -> first arrival
};
std::vector<Point> hull(std::vector<Point> p) {
    std::sort(p.begin(), p.end(), [](Point a, Point b) { return std::tie(a.x, a.y) < std::tie(b.x, b.y); });
    p.erase(std::unique(p.begin(), p.end()), p.end());
    if (p.size() < 3) return p;
    const auto cross = [](Point o, Point a, Point b) { return (a.x - o.x) * (b.y - o.y) - (a.y - o.y) * (b.x - o.x); };
    std::vector<Point> h(2 * p.size());
    std::size_t k = 0;
    for (std::size_t i = 0; i < p.size(); ++i) {
        while (k >= 2 && cross(h[k - 2], h[k - 1], p[i]) <= 0) --k;
        h[k++] = p[i];
    }
    for (std::size_t i = p.size() - 1, t = k + 1; i-- > 0;) {
        while (k >= t && cross(h[k - 2], h[k - 1], p[i]) <= 0) --k;
        h[k++] = p[i];
    }
    h.resize(k - 1);
    return h;
}
std::vector<Point> padded(const Cluster& c) {
    std::vector<Point> around;
    const double d = c.pad / std::sqrt(2.0);
    for (const auto p : c.points)
        for (const auto [dx, dy] : {std::pair{c.pad, 0.0}, {-c.pad, 0.0}, {0.0, c.pad}, {0.0, -c.pad}, {d, d}, {d, -d}, {-d, d}, {-d, -d}})
            around.push_back({p.x + dx, p.y + dy});
    return hull(std::move(around));
}
}
std::vector<Node> nodeSuggestions(const Network& network) {
    const auto& connectors = network.connectors;
    std::vector<ConflictSide> sides; // pairs: 2k, 2k+1 are one conflict
    for (const auto& a : network.rightOfWay.conflictAreas) { sides.push_back(a.first); sides.push_back(a.second); }
    try { // a branching is a display kind only (control.hpp): nothing is decided there
        for (const auto& a : automaticConflicts(network))
            if (a.kind != ConflictKind::branching) { sides.push_back(a.first); sides.push_back(a.second); }
    } catch (...) {}
    const std::size_t conflicts = sides.size() / 2, n = connectors.size() + conflicts;
    std::vector<std::size_t> parent(n);
    std::iota(parent.begin(), parent.end(), 0);
    const auto find = [&](std::size_t x) { while (parent[x] != x) x = parent[x] = parent[parent[x]]; return x; };
    const auto join = [&](std::size_t a, std::size_t b) { a = find(a); b = find(b); if (a != b) parent[std::max(a, b)] = std::min(a, b); };
    const auto linkLength = [&](const std::string& id) {
        for (const auto& l : network.links) if (l.id == id) return polylineLength(l.geometry);
        return 0.0;
    };
    // Connectors leaving, or arriving at, the same place on a Link belong to one junction.
    for (std::size_t i = 0; i < connectors.size(); ++i)
        for (std::size_t j = 0; j < i; ++j) {
            const auto &a = connectors[i], &b = connectors[j];
            const auto same = [&](const LaneReference& x, const LaneReference& y, bool end) {
                if (x.linkId != y.linkId) return false;
                const double fallback = end ? linkLength(x.linkId) : 0.0;
                return std::abs(x.station.value_or(fallback) - y.station.value_or(fallback)) < 1e-6;
            };
            if (same(a.from, b.from, true) || same(a.to, b.to, false)) join(i, j);
        }
    const auto connectorIndex = [&](const std::string& id) {
        for (std::size_t i = 0; i < connectors.size(); ++i) if (connectors[i].id == id) return i;
        return connectors.size();
    };
    for (std::size_t k = 0; k < conflicts; ++k)
        for (const auto& side : {sides[2 * k], sides[2 * k + 1]})
            if (const auto i = connectorIndex(side.path.connectorId); !side.path.connectorId.empty() && i < connectors.size())
                join(connectors.size() + k, i);
    // Points of each cluster that holds a conflict, keyed by its root.
    std::map<std::size_t, Cluster> clusters;
    for (std::size_t k = 0; k < conflicts; ++k) {
        auto& c = clusters[find(connectors.size() + k)];
        for (const auto& side : {sides[2 * k], sides[2 * k + 1]}) {
            if (side.path.connectorId.empty()) c.linkSides.push_back(side);
            else if (const auto i = connectorIndex(side.path.connectorId); i < connectors.size()) c.sources.insert(connectors[i].from.linkId);
            const auto line = controlPathPolyline(network, side.path);
            if (line.empty()) continue;
            c.points.push_back(pointAlong(line, side.entryStation)); c.points.push_back(pointAlong(line, side.exitStation));
        }
    }
    for (std::size_t i = 0; i < connectors.size(); ++i) {
        const auto found = clusters.find(find(i));
        if (found == clusters.end()) continue;
        auto& c = found->second;
        std::vector<Point> line;
        try { line = connectorCentreline(network, connectors[i]); } catch (...) { line = connectors[i].geometry; }
        c.points.insert(c.points.end(), line.begin(), line.end());
        const auto& to = connectors[i].to;
        const double at = to.station.value_or(0.0);
        if (const auto a = c.arrivals.find(to.linkId); a == c.arrivals.end() || at < a->second) c.arrivals[to.linkId] = at;
        for (const auto* end : {&connectors[i].from, &connectors[i].to})
            for (const auto& l : network.links)
                if (l.id == end->linkId) for (const auto& lane : l.lanes) c.pad = std::max(c.pad, lane.width);
    }
    std::vector<Cluster> list;
    for (auto& [root, c] : clusters) {
        c.order = root;
        for (const auto& side : c.linkSides)
            if (const auto a = c.arrivals.find(side.path.linkId); a == c.arrivals.end() || a->second > side.entryStation + 1e-6)
                c.sources.insert(side.path.linkId); if (c.pad > 0 && !c.points.empty() && c.sources.size() > 1) list.push_back(std::move(c)); }
    // Overlapping suggestions are one junction: merge until none overlap.
    for (bool merged = true; merged;) {
        merged = false;
        for (std::size_t i = 0; i < list.size() && !merged; ++i)
            for (std::size_t j = i + 1; j < list.size() && !merged; ++j)
                if (polygonsOverlap(padded(list[i]), padded(list[j]))) {
                    list[i].points.insert(list[i].points.end(), list[j].points.begin(), list[j].points.end());
                    list[i].pad = std::max(list[i].pad, list[j].pad);
                    list[i].sources.insert(list[j].sources.begin(), list[j].sources.end());
                    list.erase(list.begin() + static_cast<std::ptrdiff_t>(j));
                    merged = true;
                }
    }
    std::vector<Node> result;
    for (const auto& c : list) {
        auto polygon = padded(c);
        if (!simplePolygon(polygon)) continue;
        if (std::any_of(network.nodes.begin(), network.nodes.end(), [&](const auto& node) {
                return simplePolygon(node.polygon) && polygonsOverlap(node.polygon, polygon); })) continue;
        result.push_back({"", "", std::move(polygon), true});
    }
    return result;
}
}
