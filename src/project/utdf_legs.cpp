#include "utdf_legs.hpp"
#include "../model/network/coarse_nodes.hpp"
#include "../model/network/polygon.hpp"
#include <algorithm>
#include <cmath>
#include <set>
#include <tuple>

namespace multra::utdf {
namespace {
const Link* linkOf(const Network& n, const std::string& id) {
    const auto l = std::find_if(n.links.begin(), n.links.end(), [&](const auto& x) { return x.id == id; });
    return l == n.links.end() ? nullptr : &*l;
}
ApproachDirection opposite(ApproachDirection d) {
    switch (d) {
    case ApproachDirection::north: return ApproachDirection::south;
    case ApproachDirection::south: return ApproachDirection::north;
    case ApproachDirection::east: return ApproachDirection::west;
    case ApproachDirection::west: return ApproachDirection::east;
    }
    return d;
}
// The out-leg's travel direction where it leaves the polygon: the dominant axis, none on a tie.
std::optional<ApproachDirection> leaving(const Link& l, double station) {
    const auto d = directionAlong(l.geometry, station, false);
    if (std::abs(std::abs(d.x) - std::abs(d.y)) < 1e-12) return {};
    if (std::abs(d.y) > std::abs(d.x)) return d.y > 0 ? ApproachDirection::north : ApproachDirection::south;
    return d.x > 0 ? ApproachDirection::east : ApproachDirection::west;
}
std::string joined(const std::vector<ValidationIssue>& issues) {
    std::string out;
    for (const auto& i : issues) out += (out.empty() ? "" : ", ") + i.code + (i.path.empty() ? "" : " (" + i.path + ")");
    return out;
}
}
Point centroid(const std::vector<Point>& p) {
    double area = 0, x = 0, y = 0;
    for (std::size_t i = 0; i < p.size(); ++i) {
        const auto& a = p[i]; const auto& b = p[(i + 1) % p.size()];
        const double cross = a.x * b.y - b.x * a.y;
        area += cross; x += (a.x + b.x) * cross; y += (a.y + b.y) * cross;
    }
    if (std::abs(area) > 1e-9) return {x / (3 * area), y / (3 * area)};
    Point mean{};
    for (const auto& q : p) { mean.x += q.x / static_cast<double>(p.size()); mean.y += q.y / static_cast<double>(p.size()); }
    return mean;
}
double walkLength(Point from, const Walk& walk, Point to) {
    std::vector<Point> line{from};
    for (const auto* l : walk.links) line.insert(line.end(), l->geometry.begin(), l->geometry.end());
    line.push_back(to);
    return polylineLength(line);
}
Plan plan(const Network& n, const std::set<std::string>& excluded) {
    Plan out;
    std::set<int> numbers;
    for (const auto& node : n.nodes) {
        if (excluded.count(node.id)) continue;
        const auto refuse = [&](const char* code, std::string text) { out.refusals.push_back({code, node.id, std::move(text)}); };
        if (!nodeHasDetail(n, node)) { refuse("UTDF_NO_DETAIL", "The Node has no Micro detail; it is left out."); continue; }
        auto layout = approachLayout(n, node);
        if (!layout.issues.empty()) { refuse("UTDF_LAYOUT_INVALID", "The approach layout refuses: " + joined(layout.issues)); continue; }
        if (!node.number) { refuse("UTDF_NODE_UNNUMBERED", "The Node has no number; number the Nodes before exporting."); continue; }
        if (!numbers.insert(*node.number).second) { refuse("UTDF_NODE_NUMBER_DUPLICATE", "Another Node has number " + std::to_string(*node.number) + "."); continue; }
        std::set<std::string> names; bool duplicate = false;
        for (const auto& m : layout.movements) duplicate = !names.insert(m.name).second || duplicate;
        if (duplicate) { refuse("UTDF_MOVEMENT_DUPLICATE", "Two turns of one approach share a movement column."); numbers.erase(*node.number); continue; }
        WrittenNode w{&node, *node.number, centroid(node.polygon), std::move(layout), {}, {}, {}};
        bool ambiguous = false;
        for (const auto& leg : w.layout.membership.outLegs) {
            const auto d = leaving(*linkOf(n, leg.linkId), leg.station);
            if (!d) { ambiguous = true; break; }
            w.outDirection.push_back(*d);
        }
        if (ambiguous) {
            refuse("UTDF_LAYOUT_INVALID", "The approach layout refuses: JUNCTION_DIRECTION_AMBIGUOUS (an out-leg)");
            numbers.erase(*node.number); continue;
        }
        out.nodes.push_back(std::move(w));
    }
    std::map<std::string, std::size_t> inLegOf, outLegOf;
    for (std::size_t i = 0; i < out.nodes.size(); ++i) {
        for (const auto& leg : out.nodes[i].layout.membership.inLegs) inLegOf[leg.linkId] = i;
        for (const auto& leg : out.nodes[i].layout.membership.outLegs) outLegOf[leg.linkId] = i;
    }
    std::set<std::string> owned; // Connectors with a run inside any Node end a walk
    for (const auto& c : n.connectors) for (const auto& node : n.nodes) {
        const auto runs = polygonRuns(c.geometry, node.polygon);
        if (std::any_of(runs.begin(), runs.end(), [](const auto& r) { return r.inside; })) { owned.insert(c.id); break; }
    }
    const auto walk = [&](const std::string& start, bool upstream) {
        Walk w; std::set<std::string> seen{start};
        const Link* current = linkOf(n, start); w.links.push_back(current);
        const auto& stop = upstream ? outLegOf : inLegOf;
        for (;;) {
            if (const auto s = stop.find(current->id); s != stop.end()) { w.node = s->second; break; }
            std::set<std::string> next;
            for (const auto& c : n.connectors) {
                if (owned.count(c.id)) continue;
                if (upstream && c.to.linkId == current->id) next.insert(c.from.linkId);
                if (!upstream && c.from.linkId == current->id) next.insert(c.to.linkId);
            }
            if (next.size() != 1 || !seen.insert(*next.begin()).second) break;
            const auto* l = linkOf(n, *next.begin());
            if (!l) break;
            current = l;
            if (upstream) w.links.insert(w.links.begin(), l); else w.links.push_back(l);
        }
        return w;
    };
    for (auto& w : out.nodes) {
        for (const auto& leg : w.layout.membership.inLegs) w.in.push_back(walk(leg.linkId, true));
        for (const auto& leg : w.layout.membership.outLegs) w.out.push_back(walk(leg.linkId, false));
    }
    // External ends: an in-leg pairs with the free out-leg leaving the opposite way (§7 contract).
    std::vector<std::tuple<std::string, std::size_t, ExternalEnd>> ends;
    for (std::size_t i = 0; i < out.nodes.size(); ++i) {
        const auto& w = out.nodes[i]; std::set<std::size_t> paired;
        for (std::size_t j = 0; j < w.in.size(); ++j) {
            if (w.in[j].node) continue;
            ExternalEnd e{0, w.in[j].links.front()->geometry.front(), i, j, std::nullopt};
            const auto back = opposite(w.layout.approaches[j].direction);
            for (std::size_t k = 0; k < w.out.size(); ++k)
                if (!w.out[k].node && !paired.count(k) && w.outDirection[k] == back) { e.out = k; paired.insert(k); break; }
            ends.emplace_back(w.in[j].links.front()->id, ends.size(), e);
        }
        for (std::size_t k = 0; k < w.out.size(); ++k) if (!w.out[k].node && !paired.count(k))
            ends.emplace_back(w.out[k].links.back()->id, ends.size(), ExternalEnd{0, w.out[k].links.back()->geometry.back(), i, std::nullopt, k});
    }
    std::sort(ends.begin(), ends.end(), [](const auto& a, const auto& b) { return std::tie(std::get<0>(a), std::get<1>(a)) < std::tie(std::get<0>(b), std::get<1>(b)); });
    int next = numbers.empty() ? 1 : *numbers.rbegin() + 1;
    for (auto& [key, order, e] : ends) {
        (void)key; (void)order; e.number = next++;
        if (e.in) out.inEnd[{e.node, *e.in}] = e.number;
        if (e.out) out.outEnd[{e.node, *e.out}] = e.number;
        out.ends.push_back(e);
    }
    return out;
}
}
