#include "node_detail_commands.hpp"
#include "connector_commands.hpp"
#include "right_of_way_commands.hpp"
#include "../model/demand/coarse_compile.hpp"
#include "network_commands.hpp"
#include "road_commands.hpp"
#include "../model/network/coarse_nodes.hpp"
#include "../model/network/node_detail.hpp"
#include "../model/network/polygon.hpp"
#include <algorithm>
#include <limits>
#include <set>
#include <stdexcept>

namespace multra {
namespace {
using Geometry = std::vector<Point>;
const Road* roadOf(const Network& n, const std::string& id) {
    const auto r = std::find_if(n.roads.begin(), n.roads.end(), [&](const auto& x) { return x.id == id; });
    return r == n.roads.end() ? nullptr : &*r;
}
const Link& linkOf(const Network& n, const std::string& id) {
    return *std::find_if(n.links.begin(), n.links.end(), [&](const auto& l) { return l.id == id; });
}
// A generated leg's uncut shape: what Apply template gives its Road for its direction.
std::optional<Geometry> templateGeometry(const ProjectDocument& d, const Link& l, const std::vector<CrossSection>& templates) {
    const auto* road = l.roadId ? roadOf(d.network, *l.roadId) : nullptr;
    const auto* t = road ? roadTemplate(d, *road, templates) : nullptr;
    if (!t) return std::nullopt;
    const auto k = static_cast<std::size_t>(std::find(road->links.begin(), road->links.end(), l.id) - road->links.begin());
    const auto links = templateLinks(d.network, *road, *t);
    if (k >= links.size()) return std::nullopt;
    return links[k].geometry;
}
// Each cut Link's uncut shape and the stations it keeps, over every Node in `nodes`.
struct Span { Geometry full; double from{}, to{std::numeric_limits<double>::infinity()}; };
std::map<std::string, Span> spansAt(const ProjectDocument& d, const std::vector<const Node*>& nodes,
                                    const std::vector<CrossSection>& templates, std::string& reason) {
    std::map<std::string, Span> spans;
    for (const auto* node : nodes) {
        std::map<std::string, Geometry> full;
        for (const auto& leg : nodeLinkLegs(d.network, *node))
            if (auto g = templateGeometry(d, linkOf(d.network, leg.linkId), templates)) full[leg.linkId] = std::move(*g);
        const auto cuts = nodeLegCuts(d.network, *node, full);
        if (!cuts.reason.empty()) { reason = cuts.reason; return {}; }
        for (const auto& cut : cuts.cuts) {
            auto& span = spans[cut.linkId];
            if (span.full.empty()) span.full = full.contains(cut.linkId) ? full.at(cut.linkId) : linkOf(d.network, cut.linkId).geometry;
            (cut.in ? span.to : span.from) = cut.station;
        }
    }
    return spans;
}
void applySpans(ProjectDocument& d, const std::map<std::string, Span>& spans, const std::set<std::string>& only) {
    for (const auto& [id, span] : spans) {
        if (!only.contains(id)) continue;
        const double length = polylineLength(span.full);
        editableLink(d, id).geometry = polylineSpan(span.full, span.from, std::min(span.to, length)); // keeps roadId: the program's cut
    }
}
std::vector<const Node*> generatedNodes(const Network& n, const std::string& also) {
    std::vector<const Node*> nodes;
    for (const auto& node : n.nodes) if (node.id == also || holdsGeneratedDetail(n, node)) nodes.push_back(&node);
    return nodes;
}
}
bool holdsGeneratedDetail(const Network& n, const Node& node) {
    if (!simplePolygon(node.polygon)) return false;
    for (const auto& c : n.connectors) {
        if (!c.generated || c.geometry.size() < 2) continue;
        const auto runs = polygonRuns(c.geometry, node.polygon);
        if (std::any_of(runs.begin(), runs.end(), [](const auto& r) { return r.inside; })) return true;
    }
    return false;
}
bool nodeDetailListed(const ProjectDocument& d, const Node& node) {
    const auto& n = d.network;
    if (!simplePolygon(node.polygon) || nodeHasDetail(n, node)) return false;
    std::set<std::string> arms;
    for (const auto& leg : nodeLinkLegs(n, node)) arms.insert(legArm(linkOf(n, leg.linkId)));
    for (const auto& leg : nodeRoadLegs(n, node)) arms.insert(leg.roadId);
    return arms.size() >= 2;
}
namespace {
std::set<std::string> nodeArms(const Network& n, const Node& node) {
    std::set<std::string> arms;
    for (const auto& leg : nodeLinkLegs(n, node)) arms.insert(legArm(linkOf(n, leg.linkId)));
    return arms;
}
// The major arms: the Node's `major`, else majorByRank over each arm's link type (its first leg
// Link's, else its Road template's). Nothing when that decides nothing.
std::optional<std::set<std::string>> majorArms(const ProjectDocument& d, const Node& node, const NodeFillCatalogs& c) {
    const auto& n = d.network;
    if (node.coarse && !node.coarse->major.empty()) return std::set<std::string>(node.coarse->major.begin(), node.coarse->major.end());
    std::map<std::string, std::optional<std::string>> typeOf;
    for (const auto& leg : nodeLinkLegs(n, node)) {
        const auto& l = linkOf(n, leg.linkId);
        auto& type = typeOf[legArm(l)];
        if (type) continue;
        type = l.linkTypeId;
        if (const auto* r = l.roadId ? roadOf(n, *l.roadId) : nullptr; !type && r)
            if (const auto* t = roadTemplate(d, *r, c.templates)) type = t->linkType;
    }
    std::vector<std::pair<std::string, std::optional<int>>> ranks;
    for (const auto& [arm, type] : typeOf) {
        std::optional<int> rank;
        if (type) for (const auto& t : c.types) if (t.id == *type) rank = t.rank;
        ranks.emplace_back(arm, rank);
    }
    const auto major = majorByRank(ranks);
    if (!major) return std::nullopt;
    return std::set<std::string>(major->begin(), major->end());
}
}
std::string nodeDetailRefusal(const ProjectDocument& d, const Node& node, const NodeFillCatalogs& catalogs) {
    const auto& n = d.network;
    const auto& templates = catalogs.templates;
    for (const auto& leg : nodeRoadLegs(n, node))
        if (const auto* r = roadOf(n, leg.roadId); r && r->links.empty()) return "FILL_NODE_ROAD_NOT_GENERATED";
    for (const auto& leg : nodeLinkLegs(n, node))
        if (!templateGeometry(d, linkOf(n, leg.linkId), templates)) return "FILL_NODE_ARM_NOT_GENERATED";
    const auto control = node.coarse && node.coarse->control ? *node.coarse->control : CoarseControl::priority;
    if (control == CoarseControl::allWayStop) return "FILL_NODE_ALL_WAY_STOP";
    if (control == CoarseControl::signalised) return "FILL_NODE_SIGNALISED";
    if (const auto arms = nodeArms(n, node); arms.size() >= 3) {
        const auto major = majorArms(d, node, catalogs);
        if (!major) return "FILL_NODE_PRIORITY_UNDETERMINED";
        if (major->size() != 2 || arms.size() > 4) return "FILL_NODE_MAJOR_ARMS";
    }
    std::string reason;
    spansAt(d, generatedNodes(n, node.id), templates, reason);
    if (!reason.empty()) return reason;
    if (nodeTurnConnectors(n, node).empty()) return "FILL_NODE_NO_TURN";
    return {};
}
namespace {
template<class T> std::set<std::string> idsOf(const std::vector<T>& list) {
    std::set<std::string> ids;
    for (const auto& x : list) ids.insert(x.id);
    return ids;
}
template<class T> void markNew(std::vector<T>& list, const std::set<std::string>& before) {
    for (auto& x : list) if (!before.contains(x.id)) x.generated = true;
}
// S4.5b (§13.4): who gives way between the generated turns, by HCM rank.
struct GeneratedTurn { std::string inArm; int rank{}; bool minor{}; };
void authorPriority(ProjectDocument& d, const std::map<std::string, GeneratedTurn>& turns, const PriorityDefaults& defaults) {
    auto& row = d.network.rightOfWay;
    const auto lines = idsOf(row.waitingLines), areas = idsOf(row.conflictAreas), rules = idsOf(row.priorityRules), stops = idsOf(row.stopControls);
    const auto turnOf = [&](const ConflictSide& s) -> const GeneratedTurn* {
        const auto t = turns.find(s.path.connectorId);
        return t == turns.end() ? nullptr : &t->second;
    };
    const auto decides = [&](const ConflictSide& a, const ConflictSide& b) {
        const auto *x = turnOf(a), *y = turnOf(b);
        return x && y && x->inArm != y->inArm && x->rank != y->rank;
    };
    for (const auto& conflict : automaticConflicts(d.network)) {
        if (conflict.kind == ConflictKind::branching || !decides(conflict.first, conflict.second)) continue;
        try { authorAutomaticConflict(d, conflict, defaults); }
        catch (const std::invalid_argument& e) { // a merge already taken over whole by an earlier pair
            const std::string code = e.what();
            if (code != "EDIT_NO_MERGE" && code != "EDIT_NO_CROSSING" && code != "EDIT_ALREADY_EXPLICIT") throw;
        }
    }
    std::vector<std::string> stopping;
    const auto added = row.conflictAreas; // setConflictControl edits the list
    for (const auto& a : added) {
        if (areas.contains(a.id) || !decides(a.first, a.second)) continue; // equal ranks keep the fallback's order
        const bool firstYields = turnOf(a.first)->rank > turnOf(a.second)->rank;
        setConflictControl(d, a.id, a.name, firstYields ? ConflictPriority::firstYields : ConflictPriority::secondYields,
                           defaults.gapTime, defaults.headway);
        if ((firstYields ? turnOf(a.first) : turnOf(a.second))->minor) stopping.push_back(a.id);
    }
    for (const auto& id : stopping) setAreaControl(d, id, StopMode::stop);
    markNew(row.waitingLines, lines); markNew(row.conflictAreas, areas); markNew(row.priorityRules, rules); markNew(row.stopControls, stops);
}
}
std::vector<std::string> generateNodeDetail(ProjectDocument& d, const std::string& nodeId, const NodeFillCatalogs& catalogs) {
    const auto& templates = catalogs.templates;
    const auto found = std::find_if(d.network.nodes.begin(), d.network.nodes.end(), [&](const auto& x) { return x.id == nodeId; });
    if (found == d.network.nodes.end()) throw std::invalid_argument("EDIT_UNKNOWN_OBJECT");
    const auto node = *found;
    if (!nodeDetailListed(d, node)) throw std::invalid_argument("FILL_NODE_HAS_DETAIL");
    if (const auto reason = nodeDetailRefusal(d, node, catalogs); !reason.empty()) throw std::invalid_argument(reason);
    const auto arms = nodeArms(d.network, node);
    const auto major = arms.size() >= 3 ? majorArms(d, node, catalogs) : std::nullopt;
    std::string reason;
    const auto spans = spansAt(d, generatedNodes(d.network, node.id), templates, reason);
    std::set<std::string> legs;
    for (const auto& leg : nodeLinkLegs(d.network, node)) legs.insert(leg.linkId);
    applySpans(d, spans, legs);
    reanchorConnectors(d);
    std::vector<std::string> ids;
    std::map<std::string, GeneratedTurn> turns;
    for (const auto& t : nodeTurnConnectors(d.network, node)) {
        const auto& from = linkOf(d.network, t.inLink);
        const auto& to = linkOf(d.network, t.outLink);
        const LaneReference a{from.id, from.lanes[static_cast<std::size_t>(t.fromLane)].id}, b{to.id, to.lanes[static_cast<std::size_t>(t.toLane)].id};
        ids.push_back(addConnectorRange(d, a, b, t.fromCount, t.toCount));
        editableConnector(d, ids.back()).generated = true;
        if (major) {
            const auto in = legArm(linkOf(d.network, t.inLink)), out = legArm(linkOf(d.network, t.outLink));
            turns[ids.back()] = {in, movementRank(major->contains(in), major->contains(out), t.sweep, arms.size()), !major->contains(in)};
        }
    }
    if (major) authorPriority(d, turns, catalogs.priority);
    // Refined here as settleNodes would: the coarse values go as the detail is drawn (NODES §5).
    for (auto& n : d.network.nodes) if (n.id == nodeId) n.coarse.reset();
    return ids;
}
std::map<std::string, std::vector<Point>> generatedCuts(const ProjectDocument& d, const std::vector<std::string>& links,
                                                       const std::vector<CrossSection>& templates) {
    std::map<std::string, std::vector<Point>> result;
    const auto nodes = generatedNodes(d.network, {});
    if (nodes.empty()) return result;
    std::string reason;
    const auto spans = spansAt(d, nodes, templates, reason);
    if (!reason.empty()) return result;
    for (const auto& id : links)
        if (const auto s = spans.find(id); s != spans.end())
            result[id] = polylineSpan(s->second.full, s->second.from, std::min(s->second.to, polylineLength(s->second.full)));
    return result;
}
namespace {
// A generated object this Command changed is the person's from then on.
template<class T> void settle(std::vector<T>& after, const std::vector<T>& before) {
    if (after == before) return;
    for (auto& x : after) {
        if (!x.generated) continue;
        const auto was = std::find_if(before.begin(), before.end(), [&](const auto& y) { return y.id == x.id; });
        if (was != before.end() && *was != x) x.generated = false;
    }
}
}
void settleGenerated(ProjectDocument& after, const ProjectDocument& before) {
    auto& a = after.network; const auto& b = before.network;
    settle(a.connectors, b.connectors);
    settle(a.rightOfWay.waitingLines, b.rightOfWay.waitingLines);
    settle(a.rightOfWay.conflictAreas, b.rightOfWay.conflictAreas);
    settle(a.rightOfWay.priorityRules, b.rightOfWay.priorityRules);
    settle(a.rightOfWay.stopControls, b.rightOfWay.stopControls);
}
}
