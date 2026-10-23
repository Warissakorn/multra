#include "node_membership.hpp"
#include "polygon.hpp"
#include <algorithm>
#include <cmath>
#include <functional>
#include <set>
#include <tuple>

namespace multra {
namespace {
constexpr double kEpsilon = 1e-9;
std::vector<Point> connectorLine(const Network& network, const Connector& c) {
    if (!c.geometry.empty()) return c.geometry;
    try { return connectorCentreline(network, c); } catch (...) { return {}; }
}
std::string label(const Link& link) { return link.name.empty() ? link.id : link.name; }
}
NodeMembership nodeMembership(const Network& network, const Node& node) {
    NodeMembership m;
    const auto& polygon = node.polygon;
    if (!simplePolygon(polygon)) return m;
    const auto linkOf = [&](const std::string& id) -> const Link* {
        const auto l = std::find_if(network.links.begin(), network.links.end(), [&](const auto& x) { return x.id == id; });
        return l == network.links.end() ? nullptr : &*l;
    };
    const auto connectorOf = [&](const std::string& id) -> const Connector* {
        const auto c = std::find_if(network.connectors.begin(), network.connectors.end(), [&](const auto& x) { return x.id == id; });
        return c == network.connectors.end() ? nullptr : &*c;
    };
    // Where a control path's station lies: Link stations on the reference polyline, Connector
    // stations on the stored first-lane polyline (control.hpp).
    const auto pathPoint = [&](const ControlPathRef& path, double station) -> std::optional<Point> {
        if (!path.connectorId.empty()) {
            const auto* c = connectorOf(path.connectorId); if (!c) return {};
            const auto line = connectorLine(network, *c); if (line.empty()) return {};
            return pointAlong(line, station);
        }
        const auto* l = linkOf(path.linkId); if (!l || l->geometry.empty()) return {};
        return pointAlong(l->geometry, station);
    };
    const auto inside = [&](std::optional<Point> p) { return p && pointInPolygon(polygon, *p); };

    std::set<std::string> issues;
    std::set<std::string> internalConnectors;
    for (const auto& c : network.connectors) {
        const auto r = polygonRuns(connectorLine(network, c), polygon);
        if (r.size() == 1 && r.front().inside) { internalConnectors.insert(c.id); m.connectors.push_back(c.id); }
        else if (r.size() > 1) issues.insert("NODE_CONNECTOR_CROSSES");
    }
    for (const auto& a : network.rightOfWay.conflictAreas)
        if (inside(pathPoint(a.first.path, a.first.entryStation)) && inside(pathPoint(a.second.path, a.second.entryStation)))
            m.conflictAreas.push_back(a.id);
    for (const auto& w : network.rightOfWay.waitingLines)
        if (inside(pathPoint(w.point.path, w.point.station))) m.waitingLines.push_back(w.id);
    const auto areaInside = [&](const std::string& id) { return std::find(m.conflictAreas.begin(), m.conflictAreas.end(), id) != m.conflictAreas.end(); };

    // Each Link's inside runs, kept only when something inside connects to it (NODES §3).
    std::map<std::string, std::vector<PolygonRun>> insideRuns;
    for (const auto& link : network.links) {
        const auto r = polygonRuns(link.geometry, polygon);
        std::vector<PolygonRun> in;
        for (const auto& run : r) if (run.inside) in.push_back(run);
        if (in.empty()) continue;
        const double length = polylineLength(link.geometry);
        const auto onRun = [&](double s) { return std::any_of(in.begin(), in.end(), [&](const auto& x) { return s >= x.from - 1e-6 && s <= x.to + 1e-6; }); };
        bool connected = r.size() == 1;
        for (const auto& c : network.connectors) {
            if (!internalConnectors.count(c.id)) continue;
            if (c.from.linkId == link.id && onRun(c.from.station.value_or(length))) connected = true;
            if (c.to.linkId == link.id && onRun(c.to.station.value_or(0.0))) connected = true;
        }
        for (const auto& a : network.rightOfWay.conflictAreas)
            if (areaInside(a.id))
                for (const auto* side : {&a.first, &a.second})
                    if (side->path.connectorId.empty() && side->path.linkId == link.id) connected = true;
        if (!connected) continue;
        if (r.size() == 1) { m.links.push_back(link.id); insideRuns[link.id] = in; continue; }
        // One entry followed by one exit, or only one of the two; anything else re-enters.
        if (in.size() > 1) { issues.insert("NODE_LINK_REENTERS"); continue; }
        insideRuns[link.id] = in;
        if (in.front().from > kEpsilon) m.inLegs.push_back({link.id, label(link), in.front().from});
        if (in.front().to < length - 1e-6) m.outLegs.push_back({link.id, label(link), in.front().to});
    }
    for (const auto& h : network.signalHeads) {
        bool counts = false;
        if (!h.connectorId.empty()) counts = internalConnectors.count(h.connectorId) > 0;
        else if (const auto* l = linkOf(h.lane.linkId)) {
            try {
                const auto p = pointAlong(laneGeometry(*l, h.lane.laneId, network.drivingSide), h.position);
                const auto leg = std::find_if(m.inLegs.begin(), m.inLegs.end(), [&](const auto& x) { return x.linkId == l->id; });
                // Inside, or on an in-leg upstream of its entry: within the delay window (choice 1a).
                counts = pointInPolygon(polygon, p) || (leg != m.inLegs.end() && stationOfClosestPoint(l->geometry, p) <= leg->station);
            } catch (...) {}
        }
        if (counts) m.signalHeads.push_back(h.id);
    }

    // Turns: walk from each in-leg over internal Connectors and Links until an exit crossing.
    const ConnectorPathTable table(network);
    std::map<std::pair<std::size_t, std::size_t>, std::size_t> turnOf;
    for (std::size_t in = 0; in < m.inLegs.size(); ++in) {
        std::vector<TurnSpan> path; std::set<std::string> visited;
        std::function<void(const std::string&, double)> walk = [&](const std::string& linkId, double at) {
            const auto* link = linkOf(linkId);
            const auto found = insideRuns.find(linkId);
            if (!link || found == insideRuns.end() || path.size() > 32) return;
            const auto run = std::find_if(found->second.begin(), found->second.end(), [&](const auto& x) { return at >= x.from - 1e-6 && at <= x.to + 1e-6; });
            if (run == found->second.end()) return;
            const double length = polylineLength(link->geometry);
            const auto out = std::find_if(m.outLegs.begin(), m.outLegs.end(), [&](const auto& x) { return x.linkId == linkId; });
            if (out != m.outLegs.end() && out->station >= at) {
                path.push_back({linkId, at, out->station});
                std::vector<std::string> chain;
                for (const auto& span : path) if (chain.empty() || chain.back() != span.objectId) chain.push_back(span.objectId);
                if (!routeLaneChains(network, table, chain).empty()) {
                    const auto key = std::make_pair(in, static_cast<std::size_t>(out - m.outLegs.begin()));
                    auto t = turnOf.find(key);
                    if (t == turnOf.end()) {
                        t = turnOf.emplace(key, m.turns.size()).first;
                        m.turns.push_back({key.first, key.second, m.inLegs[in].name + " → " + out->name, {}});
                    }
                    m.turns[t->second].paths.push_back(path);
                }
                path.pop_back();
            }
            for (const auto& c : network.connectors) {
                if (c.from.linkId != linkId || !internalConnectors.count(c.id) || visited.count(c.id)) continue;
                const double leave = c.from.station.value_or(length);
                if (leave < at - 1e-6 || leave > run->to + 1e-6) continue;
                visited.insert(c.id);
                path.push_back({linkId, at, leave}); path.push_back({c.id, 0.0, 0.0});
                walk(c.to.linkId, c.to.station.value_or(0.0));
                path.pop_back(); path.pop_back();
                visited.erase(c.id);
            }
        };
        walk(m.inLegs[in].linkId, m.inLegs[in].station);
    }
    std::sort(m.turns.begin(), m.turns.end(), [](const auto& a, const auto& b) { return std::tie(a.in, a.out) < std::tie(b.in, b.out); });

    // Conflicts: each side on the turns whose paths carry it.
    const auto turnsOn = [&](const ConflictSide& side) {
        std::vector<std::size_t> result;
        for (std::size_t t = 0; t < m.turns.size(); ++t)
            for (const auto& p : m.turns[t].paths)
                if (std::any_of(p.begin(), p.end(), [&](const TurnSpan& s) {
                        return side.path.connectorId.empty()
                            ? s.objectId == side.path.linkId && side.entryStation < s.to && side.exitStation > s.from
                            : s.objectId == side.path.connectorId;
                    })) { result.push_back(t); break; }
        return result;
    };
    for (const auto& a : network.rightOfWay.conflictAreas) {
        if (!areaInside(a.id)) continue;
        for (const auto first : turnsOn(a.first))
            for (const auto second : turnsOn(a.second))
                if (first != second) m.conflicts.push_back({a.id, first, second, a.kind, a.priority});
    }

    const auto& row = network.rightOfWay;
    const auto lineInside = [&](const std::string& id) { return std::find(m.waitingLines.begin(), m.waitingLines.end(), id) != m.waitingLines.end(); };
    if (!m.signalHeads.empty()) m.control = SectionControl::signalised;
    else if (std::any_of(row.priorityRules.begin(), row.priorityRules.end(), [&](const auto& r) { return areaInside(r.conflictAreaId); })
             || std::any_of(row.stopControls.begin(), row.stopControls.end(), [&](const auto& s) { return lineInside(s.waitingLineId); })
             || std::any_of(row.conflictAreas.begin(), row.conflictAreas.end(), [&](const auto& a) { return areaInside(a.id) && a.priority != ConflictPriority::undetermined; }))
        m.control = SectionControl::unsignalised;

    if (issues.empty() && m.turns.empty()) issues.insert("NODE_NO_TURN");
    if (!issues.empty()) { m.inLegs.clear(); m.outLegs.clear(); m.turns.clear(); m.conflicts.clear(); m.control.reset(); }
    m.issues.assign(issues.begin(), issues.end());
    return m;
}
std::vector<ValidationIssue> nodeMembershipIssues(const Network& network) {
    std::vector<ValidationIssue> result;
    for (std::size_t i = 0; i < network.nodes.size(); ++i) {
        if (!simplePolygon(network.nodes[i].polygon)) continue;
        for (const auto& code : nodeMembership(network, network.nodes[i]).issues)
            result.push_back({code, "nodes[" + std::to_string(i) + "]"});
    }
    return result;
}
std::set<std::string> turnSignalPrograms(const Network& network, const NodeMembership& m, const NodeTurn& turn) {
    std::set<std::string> programs;
    const auto& leg = m.inLegs[turn.in].linkId;
    for (const auto& id : m.signalHeads) {
        const auto h = std::find_if(network.signalHeads.begin(), network.signalHeads.end(), [&](const auto& x) { return x.id == id; });
        if (h == network.signalHeads.end()) continue;
        bool governs = false;
        for (const auto& path : turn.paths) {
            const auto first = std::find_if(path.begin(), path.end(), [&](const auto& s) { return s.from == 0 && s.to == 0 && s.objectId != leg; });
            const Connector* c = nullptr;
            if (first != path.end())
                for (const auto& x : network.connectors) if (x.id == first->objectId) c = &x;
            if (!h->connectorId.empty()) {
                governs = governs || std::any_of(path.begin(), path.end(), [&](const auto& s) { return s.objectId == h->connectorId; });
            } else if (h->lane.linkId == leg) {
                if (!c || c->from.linkId != leg) { governs = true; continue; }
                const auto l = std::find_if(network.links.begin(), network.links.end(), [&](const auto& x) { return x.id == leg; });
                const auto lane = [&](const std::string& laneId) {
                    return static_cast<int>(std::find_if(l->lanes.begin(), l->lanes.end(), [&](const auto& x) { return x.id == laneId; }) - l->lanes.begin());
                };
                const int at = lane(h->lane.laneId), from = lane(c->from.laneId);
                governs = governs || (at >= from && at < from + c->fromLaneCount);
            } else {
                governs = governs || std::any_of(path.begin(), path.end(), [&](const auto& s) { return s.objectId == h->lane.linkId; });
            }
        }
        if (governs) programs.insert(headProgramId(*h));
    }
    return programs;
}
}
