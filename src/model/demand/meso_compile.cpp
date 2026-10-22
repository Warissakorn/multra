#include "meso_compile.hpp"
#include "../network/node_membership.hpp"
#include "../network/polygon.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <map>
#include <numeric>
#include <set>

namespace multra {
namespace {
using Key = std::pair<std::string, double>; // (Link id, station)
Point pointAt(const std::vector<Point>& line, double station) {
    for (std::size_t i = 0; i + 1 < line.size(); ++i) {
        const double length = std::hypot(line[i + 1].x - line[i].x, line[i + 1].y - line[i].y);
        if (station <= length || i + 2 == line.size()) {
            const double t = length > 0 ? std::clamp(station / length, 0.0, 1.0) : 0.0;
            return {line[i].x + t * (line[i + 1].x - line[i].x), line[i].y + t * (line[i + 1].y - line[i].y)};
        }
        station -= length;
    }
    return line.empty() ? Point{} : line.front();
}
struct Groups { // union-find over station keys
    std::map<Key, Key> parent;
    Key find(const Key& k) { auto& p = parent.try_emplace(k, k).first->second; return p == k ? k : (p = find(p)); }
    void join(const Key& a, const Key& b) { const auto x = find(a), y = find(b); if (x != y) parent[std::max(x, y)] = std::min(x, y); }
};
struct ValidNode { std::size_t index; NodeMembership membership; };
}
MesoCompile compileMesoNetwork(const Network& network, const std::vector<LinkType>& types, const std::vector<std::string>& typePaths,
                               const MesoSettings& settings) {
    MesoCompile out;
    auto& g = out.graph;
    std::map<std::string, std::size_t> linkIndex;
    std::map<std::string, double> lengths;
    for (std::size_t i = 0; i < network.links.size(); ++i) { linkIndex[network.links[i].id] = i; lengths[network.links[i].id] = polylineLength(network.links[i].geometry); }
    const auto typeOf = [&](const Link& l) -> const LinkType* {
        if (!l.linkTypeId) return nullptr;
        const auto t = std::find_if(types.begin(), types.end(), [&](const auto& x) { return x.id == *l.linkTypeId; });
        return t == types.end() ? nullptr : &*t;
    };
    // Nodes whose membership has no issue; the others are refused and left to the simple rule.
    std::vector<ValidNode> valid;
    for (std::size_t i = 0; i < network.nodes.size(); ++i) {
        auto m = nodeMembership(network, network.nodes[i]);
        if (!m.issues.empty()) out.issues.push_back({"MESO_NODE_REFUSED", "network.nodes[" + std::to_string(i) + "]"});
        else valid.push_back({i, std::move(m)});
    }
    const auto station = [&](const std::string& link, double s) { return std::clamp(s, 0.0, lengths[link]); };
    const auto fromKey = [&](const Connector& c) { return Key{c.from.linkId, station(c.from.linkId, c.from.station.value_or(lengths[c.from.linkId]))}; };
    const auto toKey = [&](const Connector& c) { return Key{c.to.linkId, station(c.to.linkId, c.to.station.value_or(0.0))}; };
    std::map<std::string, std::set<double>> stations;
    for (const auto& l : network.links) stations[l.id] = {0.0, lengths[l.id]};
    std::set<std::string> inside; // Connectors inside a valid Node
    for (const auto& c : network.connectors) {
        if (!lengths.contains(c.from.linkId) || !lengths.contains(c.to.linkId)) continue;
        stations[c.from.linkId].insert(fromKey(c).second); stations[c.to.linkId].insert(toKey(c).second);
    }
    for (const auto& v : valid) {
        for (const auto* legs : {&v.membership.inLegs, &v.membership.outLegs})
            for (const auto& leg : *legs) stations[leg.linkId].insert(station(leg.linkId, leg.station));
        inside.insert(v.membership.connectors.begin(), v.membership.connectors.end());
    }
    // Link pieces outside every valid Node, in (Link id, station) order.
    std::map<Key, std::size_t> endingAt, startingAt;
    for (const auto& [id, list] : stations) {
        const auto& l = network.links[linkIndex[id]];
        const auto* type = typeOf(l);
        const std::vector<double> s(list.begin(), list.end());
        for (std::size_t k = 0; k + 1 < s.size(); ++k) {
            const auto mid = pointAt(l.geometry, (s[k] + s[k + 1]) / 2);
            if (std::any_of(valid.begin(), valid.end(), [&](const auto& v) { return pointInPolygon(network.nodes[v.index].polygon, mid); })) continue;
            MesoLink m{id, s[k], s[k + 1], 0, 0, s[k + 1] - s[k], static_cast<double>(l.lanes.size()), 0, 0, 0, {}};
            if (type) {
                m.capacityPerLane = type->capacityPerLane; m.freeSpeed = type->freeSpeed; m.modes = type->modes;
                if (type->jamDensity) m.jamDensity = type->jamDensity->value;
            }
            endingAt[{id, s[k + 1]}] = startingAt[{id, s[k]}] = g.links.size();
            g.links.push_back(std::move(m));
        }
    }
    // Station groups: joined by an outside Connector, or by being one valid Node's crossings.
    Groups groups;
    for (const auto& c : network.connectors)
        if (lengths.contains(c.from.linkId) && lengths.contains(c.to.linkId) && !inside.contains(c.id)) groups.join(fromKey(c), toKey(c));
    std::map<Key, std::size_t> nodeOfRoot; // a group's root -> the valid Node it belongs to (index into valid)
    for (std::size_t n = 0; n < valid.size(); ++n) {
        std::vector<Key> keys;
        for (const auto& leg : valid[n].membership.inLegs) keys.push_back({leg.linkId, station(leg.linkId, leg.station)});
        for (const auto& leg : valid[n].membership.outLegs) keys.push_back({leg.linkId, station(leg.linkId, leg.station)});
        for (std::size_t k = 1; k < keys.size(); ++k) groups.join(keys[0], keys[k]);
        if (!keys.empty()) nodeOfRoot[groups.find(keys[0])] = n;
    }
    for (auto& [root, n] : std::map<Key, std::size_t>(nodeOfRoot)) { nodeOfRoot.erase(root); nodeOfRoot[groups.find(root)] = n; }
    // One Meso node per group a kept link touches, in the order of the group's least key.
    std::map<Key, std::size_t> nodeIndex;
    std::set<Key> touched;
    for (const auto& [key, i] : endingAt) touched.insert(key);
    for (const auto& [key, i] : startingAt) touched.insert(key);
    for (const auto& key : touched) {
        const auto root = groups.find(key);
        if (nodeIndex.contains(root)) continue;
        nodeIndex[root] = g.nodes.size();
        MesoNode node;
        if (const auto n = nodeOfRoot.find(root); n != nodeOfRoot.end()) {
            const auto& drawn = network.nodes[valid[n->second].index];
            node.name = drawn.name.empty() ? drawn.id : drawn.name; node.nodeId = drawn.id;
            const auto& control = valid[n->second].membership.control;
            node.control = !control ? "" : *control == SectionControl::signalised ? "signalised" : "unsignalised";
        } else node.name = root.first + "@" + std::to_string(root.second);
        g.nodes.push_back(std::move(node));
    }
    for (const auto& [key, i] : endingAt) g.links[i].to = nodeIndex[groups.find(key)];
    for (const auto& [key, i] : startingAt) g.links[i].from = nodeIndex[groups.find(key)];
    // Turns, grouped by node.
    std::vector<std::vector<MesoTurn>> byNode(g.nodes.size());
    std::vector<std::vector<MesoConflict>> conflicts(g.nodes.size());
    const auto speedOf = [&](const std::string& link) { const auto* t = typeOf(network.links[linkIndex[link]]); return t ? t->freeSpeed : 0.0; };
    for (const auto& v : valid) {
        const auto& m = v.membership;
        if (m.inLegs.empty()) continue;
        const auto node = nodeIndex[groups.find({m.inLegs[0].linkId, station(m.inLegs[0].linkId, m.inLegs[0].station)})];
        std::vector<std::size_t> local(m.turns.size(), SIZE_MAX); // membership turn -> position in byNode[node]
        for (std::size_t t = 0; t < m.turns.size(); ++t) {
            const auto& turn = m.turns[t];
            const auto& in = m.inLegs[turn.in]; const auto& outLeg = m.outLegs[turn.out];
            const auto a = endingAt.find({in.linkId, station(in.linkId, in.station)});
            const auto b = startingAt.find({outLeg.linkId, station(outLeg.linkId, outLeg.station)});
            if (a == endingAt.end() || b == startingAt.end()) continue;
            MesoTurn mt{node, a->second, b->second, turn.name, 0, {}};
            double length = 0;
            for (const auto& span : turn.paths.empty() ? std::vector<TurnSpan>{} : turn.paths.front()) {
                if (lengths.contains(span.objectId) && span.to != span.from) length += std::abs(span.to - span.from);
                else for (const auto& c : network.connectors) if (c.id == span.objectId) length += polylineLength(c.geometry);
            }
            for (const auto& path : turn.paths)
                for (const auto& span : path)
                    if (std::find(mt.via.begin(), mt.via.end(), span.objectId) == mt.via.end()) mt.via.push_back(span.objectId);
            if (const double v0 = speedOf(in.linkId); v0 > 0) mt.freeTime = length / v0;
            local[t] = byNode[node].size();
            byNode[node].push_back(std::move(mt));
        }
        for (const auto& c : m.conflicts)
            if (local[c.first] != SIZE_MAX && local[c.second] != SIZE_MAX)
                conflicts[node].push_back({local[c.first], local[c.second],
                    c.priority == ConflictPriority::firstYields ? MesoPriority::firstYields
                    : c.priority == ConflictPriority::secondYields ? MesoPriority::secondYields : MesoPriority::undetermined});
    }
    // Simple nodes: a Link continuing through a station, and Connectors merged per (in, out).
    std::map<std::pair<std::size_t, std::size_t>, MesoTurn> simple;
    for (const auto& [key, in] : endingAt) {
        const auto out = startingAt.find(key);
        const auto node = nodeIndex[groups.find(key)];
        if (out != startingAt.end() && g.nodes[node].nodeId.empty()) simple[{in, out->second}] = {node, in, out->second, key.first, 0, {}};
    }
    std::vector<const Connector*> connectors;
    for (const auto& c : network.connectors) connectors.push_back(&c);
    std::sort(connectors.begin(), connectors.end(), [](const auto* a, const auto* b) { return a->id < b->id; });
    for (const auto* c : connectors) {
        if (!lengths.contains(c->from.linkId) || !lengths.contains(c->to.linkId) || inside.contains(c->id)) continue;
        const auto a = endingAt.find(fromKey(*c)); const auto b = startingAt.find(toKey(*c));
        if (a == endingAt.end() || b == startingAt.end()) continue;
        auto [turn, added] = simple.try_emplace({a->second, b->second});
        if (added) {
            const double v0 = speedOf(c->from.linkId);
            turn->second = {nodeIndex[groups.find(fromKey(*c))], a->second, b->second, c->from.linkId + " → " + c->to.linkId,
                            v0 > 0 ? polylineLength(c->geometry) / v0 : 0.0, {}};
        }
        turn->second.via.push_back(c->id);
    }
    for (auto& [pair, turn] : simple) byNode[turn.node].push_back(std::move(turn));
    for (std::size_t n = 0; n < g.nodes.size(); ++n) {
        const auto base = g.turns.size();
        for (auto& t : byNode[n]) { g.nodes[n].turns.push_back(g.turns.size()); g.turns.push_back(std::move(t)); }
        for (auto c : conflicts[n]) { c.first += base; c.second += base; g.nodes[n].conflicts.push_back(c); }
    }
    std::vector<bool> fed(g.links.size()), drained(g.links.size());
    for (const auto& t : g.turns) { fed[t.out] = true; drained[t.in] = true; }
    for (std::size_t i = 0; i < g.links.size(); ++i) {
        if (!fed[i]) g.entries.push_back(i);
        if (!drained[i]) g.exits.push_back(i);
    }
    for (const auto& v : valid)
        if (!v.membership.conflicts.empty()) out.advisories.push_back({"MESO_PRIORITY_IGNORED", "network.nodes[" + std::to_string(v.index) + "]"});
    // Refusals.
    std::set<std::string> used;
    for (std::size_t i = 0; i < network.links.size(); ++i) {
        const auto& l = network.links[i];
        const auto path = "network.links[" + std::to_string(i) + "].linkType";
        if (!l.linkTypeId) out.issues.push_back({"MACRO_LINK_TYPE_MISSING", path});
        else if (!typeOf(l)) out.issues.push_back({"MACRO_LINK_TYPE_UNKNOWN", path});
        else used.insert(*l.linkTypeId);
    }
    for (std::size_t k = 0; k < types.size(); ++k)
        if (used.contains(types[k].id) && !types[k].jamDensity)
            out.issues.push_back({"MESO_JAM_DENSITY_MISSING", (k < typePaths.size() ? typePaths[k] : types[k].id) + ".jamDensity"});
    if (const double limit = mesoStepLimit(g); limit > 0 && settings.step > limit)
        out.issues.push_back({"MESO_STEP_TOO_LONG", "definition.mesoSettings.step"});
    return out;
}
}
