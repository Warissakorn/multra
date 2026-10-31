#include "node_detail_commands.hpp"
#include "node_detail_priority.hpp"
#include "connector_commands.hpp"
#include "right_of_way_commands.hpp"
#include "demand_commands.hpp"
#include "../model/demand/coarse_compile.hpp"
#include "network_commands.hpp"
#include "street_segment_commands.hpp"
#include "../model/network/coarse_nodes.hpp"
#include "../model/network/node_detail.hpp"
#include "../model/network/polygon.hpp"
#include "../model/network/street_segments.hpp"
#include <algorithm>
#include <limits>
#include <set>
#include <stdexcept>

namespace multra {
namespace {
using Geometry = std::vector<Point>;
const StreetSegment* streetSegmentOf(const Network& n, const std::string& id) {
    const auto r = std::find_if(n.streetSegments.begin(), n.streetSegments.end(), [&](const auto& x) { return x.id == id; });
    return r == n.streetSegments.end() ? nullptr : &*r;
}
const Link& linkOf(const Network& n, const std::string& id) {
    return *std::find_if(n.links.begin(), n.links.end(), [&](const auto& l) { return l.id == id; });
}
// A generated leg's uncut shape: what Apply template gives its StreetSegment for its direction.
std::optional<Geometry> templateGeometry(const ProjectDocument& d, const Link& l, const std::vector<CrossSection>& templates) {
    const auto* road = l.streetSegmentId ? streetSegmentOf(d.network, *l.streetSegmentId) : nullptr;
    const auto* t = road ? streetSegmentTemplate(d, *road, templates) : nullptr;
    if (!t) return std::nullopt;
    const std::size_t k = forwardOnStreetSegment(*road, l) ? 0 : 1; // D382: by geometry, not by place in `links`
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
            if (span.full.empty()) {
                span.full = full.contains(cut.linkId) ? full.at(cut.linkId) : linkOf(d.network, cut.linkId).geometry;
                // The end away from the Node stays where it is: a split there (splitLink's 0.2 m span)
                // shortened it, and growing it back would strand the split's Connector short of the end.
                const auto& now = linkOf(d.network, cut.linkId).geometry;
                const double length = polylineLength(span.full), first = stationOfClosestPoint(span.full, now.front()),
                             last = stationOfClosestPoint(span.full, now.back());
                if (first > 1e-6) span.from = first;
                if (last < length - 1e-6) span.to = last;
            }
            (cut.in ? span.to : span.from) = cut.station;
        }
    }
    return spans;
}
void applySpans(ProjectDocument& d, const std::map<std::string, Span>& spans, const std::set<std::string>& only) {
    for (const auto& [id, span] : spans) {
        if (!only.contains(id)) continue;
        const double length = polylineLength(span.full);
        editableLink(d, id).geometry = polylineSpan(span.full, span.from, std::min(span.to, length)); // keeps streetSegmentId: the program's cut
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
    if (node.junction) return false; // J3b: F4 must not bypass the selected lane recipe.
    if (!simplePolygon(node.polygon) || nodeHasDetail(n, node)) return false;
    std::set<std::string> arms;
    for (const auto& leg : nodeLinkLegs(n, node)) arms.insert(legArm(linkOf(n, leg.linkId)));
    for (const auto& leg : nodeStreetSegmentLegs(n, node)) arms.insert(leg.streetSegmentId);
    return arms.size() >= 2;
}
namespace {
// The run's time step, the grid signal times sit on.
double timeStep(const ProjectDocument& d) { return d.definition ? d.definition->timeStep : AuthoringDefinition{}.timeStep; }
std::set<std::string> nodeArms(const Network& n, const Node& node) {
    std::set<std::string> arms;
    for (const auto& leg : nodeLinkLegs(n, node)) arms.insert(legArm(linkOf(n, leg.linkId)));
    return arms;
}
// The major arms: the Node's `major`, else majorByRank over each arm's link type (its first leg
// Link's, else its StreetSegment template's). Nothing when that decides nothing.
std::optional<std::set<std::string>> majorArms(const ProjectDocument& d, const Node& node, const NodeFillCatalogs& c) {
    const auto& n = d.network;
    if (node.coarse && !node.coarse->major.empty()) return std::set<std::string>(node.coarse->major.begin(), node.coarse->major.end());
    std::map<std::string, std::optional<std::string>> typeOf;
    for (const auto& leg : nodeLinkLegs(n, node)) {
        const auto& l = linkOf(n, leg.linkId);
        auto& type = typeOf[legArm(l)];
        if (type) continue;
        type = l.linkTypeId;
        if (const auto* r = l.streetSegmentId ? streetSegmentOf(n, *l.streetSegmentId) : nullptr; !type && r)
            if (const auto* t = streetSegmentTemplate(d, *r, c.templates)) type = t->linkType;
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
    const auto streetSegmentLegs = nodeStreetSegmentLegs(n, node);
    for (const auto& leg : streetSegmentLegs)
        if (const auto* r = streetSegmentOf(n, leg.streetSegmentId); r && r->links.empty()) return "FILL_NODE_STREET_SEGMENT_NOT_GENERATED";
    // D382: a StreetSegment ending inside whose Links pass beside the polygon (a Node narrower than the
    // carriageway) would give turns without arms.
    const auto linkLegs = nodeLinkLegs(n, node);
    for (const auto& leg : streetSegmentLegs)
        if (const auto* r = streetSegmentOf(n, leg.streetSegmentId))
            for (const auto& id : r->links)
                if (std::none_of(linkLegs.begin(), linkLegs.end(), [&](const auto& l) { return l.linkId == id; })) return "FILL_NODE_ARM_OUTSIDE";
    for (const auto& leg : linkLegs)
        if (!templateGeometry(d, linkOf(n, leg.linkId), templates)) return "FILL_NODE_ARM_NOT_GENERATED";
    const auto control = node.coarse && node.coarse->control ? *node.coarse->control : CoarseControl::priority;
    if (control == CoarseControl::allWayStop) return "FILL_NODE_ALL_WAY_STOP";
    if (const auto arms = nodeArms(n, node); arms.size() >= 3) {
        const auto major = majorArms(d, node, catalogs);
        if (!major) return "FILL_NODE_PRIORITY_UNDETERMINED";
        if (major->size() != 2 || arms.size() > 4) return "FILL_NODE_MAJOR_ARMS";
    }
    std::string reason;
    spansAt(d, generatedNodes(n, node.id), templates, reason);
    if (!reason.empty()) return reason;
    const auto turns = nodeTurnConnectors(n, node);
    if (turns.empty()) return "FILL_NODE_NO_TURN";
    if (control == CoarseControl::signalised) { // S4.5c (§13.5)
        if (!node.coarse->signal) return "NODE_COARSE_SIGNAL_INVALID";
        const auto& signal = *node.coarse->signal;
        const auto layout = signalLayout(signal, timeStep(d));
        if (layout.offGrid) return "FILL_NODE_SIGNAL_GRID";
        if (layout.intergreen < SignalGroup{}.amber - 1e-9) return "FILL_NODE_INTERGREEN";
        for (const auto& t : turns)
            if (auto g = turnGreen(signal, layout, {legArm(linkOf(n, t.inLink)), legArm(linkOf(n, t.outLink))}); !g.reason.empty()) return g.reason;
    }
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
    const bool signalised = node.coarse && node.coarse->control == CoarseControl::signalised;
    const auto signal = signalised ? *node.coarse->signal : CoarseSignal{};
    const auto layout = signalLayout(signal, timeStep(d));
    SignalController controller{"", node.name.empty() ? node.id : node.name, signal.cycle, 0, {}};
    for (const auto& t : nodeTurnConnectors(d.network, node)) {
        const auto& from = linkOf(d.network, t.inLink);
        const auto& to = linkOf(d.network, t.outLink);
        const LaneReference a{from.id, from.lanes[static_cast<std::size_t>(t.fromLane)].id}, b{to.id, to.lanes[static_cast<std::size_t>(t.toLane)].id};
        ids.push_back(addConnectorRange(d, a, b, t.fromCount, t.toCount));
        editableConnector(d, ids.back()).generated = true;
        const auto in = legArm(linkOf(d.network, t.inLink)), out = legArm(linkOf(d.network, t.outLink));
        GeneratedTurn turn{in, 0, false, {}, signalised, ids.size()};
        if (major) {
            turn.rank = movementRank(major->contains(in), major->contains(out), t.sweep, arms.size());
            turn.minor = !signalised && !major->contains(in); // no Stop at a signal
        }
        if (signalised) { // one group per turn, green over its stages (F4-5a)
            const auto green = turnGreen(signal, layout, {in, out});
            turn.stages = green.stages;
            SignalGroup group; group.number = static_cast<int>(controller.groups.size()) + 1;
            group.name = in + " -> " + out; group.greenStart = green.start; group.greenEnd = green.end;
            controller.groups.push_back(group);
        }
        turns[ids.back()] = std::move(turn);
    }
    if (major) authorNodePriority(d, turns, catalogs.priority);
    if (signalised) { // S4.5c (§13.5): the controller, and a head at the start of every path of each turn
        const auto heads = idsOf(d.network.signalHeads);
        const auto controllers = d.definition ? idsOf(d.definition->signalControllers) : std::set<std::string>{};
        const auto id = putSignalController(d, controller);
        for (std::size_t k = 0; k < ids.size(); ++k) {
            const auto c = std::find_if(d.network.connectors.begin(), d.network.connectors.end(), [&](const auto& x) { return x.id == ids[k]; });
            for (const auto& path : connectorPaths(d.network, *c)) {
                NetworkSignalHead head; head.connectorId = path.id; head.controllerId = id; head.groupNumber = static_cast<int>(k) + 1;
                putSignalHead(d, std::move(head));
            }
        }
        markNew(d.network.signalHeads, heads); markNew(d.definition->signalControllers, controllers);
    }
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
    settle(a.links, b.links); // J3b's cuts and pockets, schema 62
    settle(a.connectors, b.connectors);
    settle(a.rightOfWay.waitingLines, b.rightOfWay.waitingLines);
    settle(a.rightOfWay.conflictAreas, b.rightOfWay.conflictAreas);
    settle(a.rightOfWay.priorityRules, b.rightOfWay.priorityRules);
    settle(a.rightOfWay.stopControls, b.rightOfWay.stopControls);
    settle(a.signalHeads, b.signalHeads); // S4.5c (D325)
    if (after.definition && before.definition) settle(after.definition->signalControllers, before.definition->signalControllers);
}
}
