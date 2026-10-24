#include "node_detail_commands.hpp"
#include "connector_commands.hpp"
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
std::string nodeDetailRefusal(const ProjectDocument& d, const Node& node, const std::vector<CrossSection>& templates) {
    const auto& n = d.network;
    for (const auto& leg : nodeRoadLegs(n, node))
        if (const auto* r = roadOf(n, leg.roadId); r && r->links.empty()) return "FILL_NODE_ROAD_NOT_GENERATED";
    for (const auto& leg : nodeLinkLegs(n, node))
        if (!templateGeometry(d, linkOf(n, leg.linkId), templates)) return "FILL_NODE_ARM_NOT_GENERATED";
    const auto control = node.coarse && node.coarse->control ? *node.coarse->control : CoarseControl::priority;
    if (control == CoarseControl::allWayStop) return "FILL_NODE_ALL_WAY_STOP";
    if (control == CoarseControl::signalised) return "FILL_NODE_SIGNALISED";
    std::string reason;
    spansAt(d, generatedNodes(n, node.id), templates, reason);
    if (!reason.empty()) return reason;
    if (nodeTurnConnectors(n, node).empty()) return "FILL_NODE_NO_TURN";
    return {};
}
std::vector<std::string> generateNodeDetail(ProjectDocument& d, const std::string& nodeId, const std::vector<CrossSection>& templates) {
    const auto found = std::find_if(d.network.nodes.begin(), d.network.nodes.end(), [&](const auto& x) { return x.id == nodeId; });
    if (found == d.network.nodes.end()) throw std::invalid_argument("EDIT_UNKNOWN_OBJECT");
    const auto node = *found;
    if (!nodeDetailListed(d, node)) throw std::invalid_argument("FILL_NODE_HAS_DETAIL");
    if (const auto reason = nodeDetailRefusal(d, node, templates); !reason.empty()) throw std::invalid_argument(reason);
    std::string reason;
    const auto spans = spansAt(d, generatedNodes(d.network, node.id), templates, reason);
    std::set<std::string> legs;
    for (const auto& leg : nodeLinkLegs(d.network, node)) legs.insert(leg.linkId);
    applySpans(d, spans, legs);
    reanchorConnectors(d);
    std::vector<std::string> ids;
    for (const auto& t : nodeTurnConnectors(d.network, node)) {
        const auto& from = linkOf(d.network, t.inLink);
        const auto& to = linkOf(d.network, t.outLink);
        const LaneReference a{from.id, from.lanes[static_cast<std::size_t>(t.fromLane)].id}, b{to.id, to.lanes[static_cast<std::size_t>(t.toLane)].id};
        ids.push_back(addConnectorRange(d, a, b, t.fromCount, t.toCount));
        editableConnector(d, ids.back()).generated = true;
    }
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
void settleGenerated(ProjectDocument& after, const ProjectDocument& before) {
    if (after.network.connectors == before.network.connectors) return;
    for (auto& c : after.network.connectors) {
        if (!c.generated) continue;
        const auto was = std::find_if(before.network.connectors.begin(), before.network.connectors.end(), [&](const auto& x) { return x.id == c.id; });
        if (was != before.network.connectors.end() && *was != c) c.generated = false;
    }
}
}
