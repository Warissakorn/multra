#include "coarse_compile.hpp"
#include "../network/coarse_nodes.hpp"
#include "../network/polygon.hpp"
#include <algorithm>
#include <cmath>
#include <map>
#include <numeric>

namespace multra {
std::optional<std::vector<std::string>> majorByRank(const std::vector<std::pair<std::string, std::optional<int>>>& legs) {
    if (legs.empty() || std::any_of(legs.begin(), legs.end(), [](const auto& l) { return !l.second; })) return std::nullopt;
    const int top = std::max_element(legs.begin(), legs.end(), [](const auto& a, const auto& b) { return *a.second < *b.second; })->second.value();
    std::vector<std::string> major;
    for (const auto& [id, rank] : legs) if (*rank == top) major.push_back(id);
    if (major.size() == legs.size()) return std::nullopt; // a tie: no minor leg
    return major;
}
RoadCompile compileRoads(const Network& network, const std::vector<CrossSection>& templates, const std::vector<LinkType>& types) {
    RoadCompile out;
    // The Nodes without detail, with their legs.
    std::vector<std::pair<std::size_t, std::vector<RoadLeg>>> nodes;
    std::vector<std::pair<std::size_t, std::vector<LinkLeg>>> linkLegs;
    for (std::size_t k = 0; k < network.nodes.size(); ++k)
        if (simplePolygon(network.nodes[k].polygon) && !nodeHasDetail(network, network.nodes[k])) {
            nodes.emplace_back(k, nodeRoadLegs(network, network.nodes[k]));
            linkLegs.emplace_back(k, nodeLinkLegs(network, network.nodes[k]));
        }
    std::map<std::size_t, std::size_t> joinOfNode; // network.nodes index -> join
    struct Span { double from{}, to{}; std::optional<std::size_t> start, end; const CrossSection* section{}; };
    std::map<std::size_t, Span> spans; // Link-less Roads that compile, by index
    for (std::size_t i = 0; i < network.roads.size(); ++i) {
        const auto& r = network.roads[i];
        if (!r.links.empty() || r.centreline.size() < 2) continue;
        const auto at = "network.roads[" + std::to_string(i) + "].crossSectionId";
        if (!r.crossSectionId) { out.issues.push_back({"MACRO_ROAD_TEMPLATE_MISSING", at}); continue; }
        const auto section = std::find_if(templates.begin(), templates.end(), [&](const auto& s) { return s.id == *r.crossSectionId; });
        if (section == templates.end()) { out.issues.push_back({"ROAD_CROSS_SECTION_UNKNOWN", at}); continue; }
        if (!section->linkType) out.issues.push_back({"MACRO_LINK_TYPE_MISSING", at});
        else if (std::none_of(types.begin(), types.end(), [&](const auto& t) { return t.id == *section->linkType; }))
            out.issues.push_back({"MACRO_LINK_TYPE_UNKNOWN", at});
        spans[i] = {0.0, polylineLength(r.centreline), {}, {}, &*section};
    }
    const auto nodeJoin = [&](std::size_t k) {
        if (const auto j = joinOfNode.find(k); j != joinOfNode.end()) return j->second;
        const auto& node = network.nodes[k];
        RoadJoin join{k, node.name.empty() ? node.id : node.name, "unsignalised", {}, {}, {}};
        if (node.coarse && node.coarse->control == CoarseControl::signalised) join.control = "signalised";
        out.joins.push_back(std::move(join));
        return joinOfNode[k] = out.joins.size() - 1;
    };
    for (const auto& [k, legs] : nodes)
        for (const auto& leg : legs) {
            const auto r = std::find_if(network.roads.begin(), network.roads.end(), [&](const auto& x) { return x.id == leg.roadId; });
            const auto span = spans.find(static_cast<std::size_t>(r - network.roads.begin()));
            if (span == spans.end()) continue;
            if (leg.atEnd) { span->second.to = std::min(span->second.to, leg.station); span->second.end = nodeJoin(k); }
            else { span->second.from = std::max(span->second.from, leg.station); span->second.start = nodeJoin(k); }
        }
    // Free Road ends within the tolerance of one another: union-find over (Road index, atEnd).
    std::vector<std::pair<std::size_t, bool>> ends;
    for (const auto& [i, span] : spans) {
        if (!span.start) ends.emplace_back(i, false);
        if (!span.end) ends.emplace_back(i, true);
    }
    std::vector<std::size_t> parent(ends.size());
    std::iota(parent.begin(), parent.end(), 0);
    const auto find = [&](std::size_t x) { while (parent[x] != x) x = parent[x] = parent[parent[x]]; return x; };
    const auto point = [&](const std::pair<std::size_t, bool>& e) {
        const auto& c = network.roads[e.first].centreline; return e.second ? c.back() : c.front();
    };
    for (std::size_t a = 0; a < ends.size(); ++a)
        for (std::size_t b = a + 1; b < ends.size(); ++b) {
            const auto p = point(ends[a]), q = point(ends[b]);
            if (ends[a].first != ends[b].first && std::hypot(p.x - q.x, p.y - q.y) <= roadEndTolerance) parent[find(b)] = find(a);
        }
    std::map<std::size_t, std::vector<std::size_t>> groups;
    for (std::size_t e = 0; e < ends.size(); ++e) groups[find(e)].push_back(e);
    for (const auto& [root, members] : groups) {
        if (members.size() < 2) continue;
        const auto p = point(ends[root]);
        out.joins.push_back({std::nullopt, "road-end@" + std::to_string(p.x) + "," + std::to_string(p.y), "", {}, {}, {}});
        for (const auto e : members) (ends[e].second ? spans[ends[e].first].end : spans[ends[e].first].start) = out.joins.size() - 1;
    }
    // Pieces, by Road order, forward first.
    for (const auto& [i, span] : spans) {
        if (span.to - span.from <= 1e-9) continue;
        const auto& r = network.roads[i];
        const auto* s = span.section;
        const LinkType* type = nullptr;
        if (s->linkType)
            if (const auto t = std::find_if(types.begin(), types.end(), [&](const auto& x) { return x.id == *s->linkType; }); t != types.end()) type = &*t;
        const auto piece = [&](bool forward, int lanes) {
            RoadPiece p{r.id, i, forward, forward ? span.from : span.to, forward ? span.to : span.from, span.to - span.from,
                        static_cast<double>(lanes), type, s->linkType.value_or(""), forward ? span.start : span.end, forward ? span.end : span.start};
            out.pieces.push_back(std::move(p));
        };
        piece(true, r.overrides.lanesForward.value_or(s->lanesForward));
        if (s->directions == 2) piece(false, r.overrides.lanesBackward.value_or(s->lanesBackward));
    }
    // Arms: pieces, then each Node's Link legs.
    const auto roadLabel = [&](std::size_t road) { return network.roads[road].name.empty() ? network.roads[road].id : network.roads[road].name; };
    for (std::size_t i = 0; i < out.pieces.size(); ++i) {
        const auto& p = out.pieces[i];
        if (p.end) out.joins[*p.end].ins.push_back({i, "", 0, p.roadId, roadLabel(p.road)});
        if (p.start) out.joins[*p.start].outs.push_back({i, "", 0, p.roadId, roadLabel(p.road)});
    }
    for (const auto& [k, legs] : linkLegs)
        for (const auto& leg : legs) {
            const auto& l = *std::find_if(network.links.begin(), network.links.end(), [&](const auto& x) { return x.id == leg.linkId; });
            const auto r = l.roadId ? std::find_if(network.roads.begin(), network.roads.end(), [&](const auto& x) { return x.id == *l.roadId; }) : network.roads.end();
            JoinArm arm{std::nullopt, l.id, leg.station, l.id, l.name.empty() ? l.id : l.name};
            if (r != network.roads.end()) { arm.road = r->id; arm.label = roadLabel(static_cast<std::size_t>(r - network.roads.begin())); }
            const auto j = nodeJoin(k);
            (leg.in ? out.joins[j].ins : out.joins[j].outs).push_back(arm);
            out.dropped[l.id].push_back(leg.in ? std::pair{leg.station, polylineLength(l.geometry)} : std::pair{0.0, leg.station});
        }
    // Turns per join.
    for (auto& join : out.joins) {
        const NodeCoarse* coarse = join.node && network.nodes[*join.node].coarse ? &*network.nodes[*join.node].coarse : nullptr;
        for (std::size_t a = 0; a < join.ins.size(); ++a)
            for (std::size_t b = 0; b < join.outs.size(); ++b) {
                const std::pair pair{join.ins[a].road, join.outs[b].road};
                if (pair.first == pair.second) continue; // a U-turn
                if (coarse && std::find(coarse->bannedTurns.begin(), coarse->bannedTurns.end(), pair) != coarse->bannedTurns.end()) continue;
                RoadTurn turn{a, b, join.ins[a].label + " → " + join.outs[b].label, {}, 0};
                if (coarse && coarse->control == CoarseControl::signalised && coarse->signal && coarse->signal->cycle > 0) {
                    turn.signal = "node:" + network.nodes[*join.node].id;
                    double green = 0;
                    for (const auto& stage : coarse->signal->stages)
                        if (std::find(stage.turns.begin(), stage.turns.end(), pair) != stage.turns.end()) green += stage.green;
                    turn.green = std::clamp(green / coarse->signal->cycle, 0.0, 1.0);
                }
                join.turns.push_back(std::move(turn));
            }
    }
    // Priority by hierarchy, where it would be read: over the distinct Roads of a Node's arms.
    for (const auto& join : out.joins) {
        if (!join.node) continue;
        const auto& node = network.nodes[*join.node];
        const bool priority = !node.coarse || !node.coarse->control || *node.coarse->control == CoarseControl::priority;
        std::vector<std::string> roads;
        for (const auto* arms : {&join.ins, &join.outs})
            for (const auto& arm : *arms) if (std::find(roads.begin(), roads.end(), arm.road) == roads.end()) roads.push_back(arm.road);
        if (!priority || (node.coarse && !node.coarse->major.empty()) || roads.size() < 3) continue;
        std::vector<std::pair<std::string, std::optional<int>>> ranks;
        for (const auto& id : roads) {
            std::optional<std::string> typeId;
            const auto r = std::find_if(network.roads.begin(), network.roads.end(), [&](const auto& x) { return x.id == id; });
            const auto span = spans.find(static_cast<std::size_t>(r - network.roads.begin()));
            if (span != spans.end()) typeId = span->second.section->linkType;
            else {
                const auto& first = r != network.roads.end() && !r->links.empty() ? r->links.front() : id; // a Link without a Road
                for (const auto& l : network.links) if (l.id == first) typeId = l.linkTypeId;
            }
            std::optional<int> rank;
            if (typeId) for (const auto& t : types) if (t.id == *typeId) rank = t.rank;
            ranks.emplace_back(id, rank);
        }
        if (!majorByRank(ranks)) out.advisories.push_back({"NODE_PRIORITY_UNDETERMINED", "network.nodes[" + std::to_string(*join.node) + "]"});
    }
    return out;
}
}
