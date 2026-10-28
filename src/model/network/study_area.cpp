#include "study_area.hpp"
#include "coarse_nodes.hpp"
#include "node_membership.hpp"
#include <algorithm>
#include <map>
#include <set>

namespace multra {
namespace {
using Ids = std::set<std::string>;
// Every Link reached from `start` through Connectors (forward: from -> to), not entering `stop`.
Ids reach(const Network& n, const Ids& start, const Ids& stop, bool forward) {
    std::multimap<std::string, std::string> next;
    for (const auto& c : n.connectors)
        forward ? next.emplace(c.from.linkId, c.to.linkId) : next.emplace(c.to.linkId, c.from.linkId);
    Ids seen;
    std::vector<std::string> open(start.begin(), start.end());
    while (!open.empty()) {
        const auto at = open.back(); open.pop_back();
        for (auto [it, end] = next.equal_range(at); it != end; ++it)
            if (!stop.contains(it->second) && seen.insert(it->second).second) open.push_back(it->second);
    }
    return seen;
}
bool kept(const Ids& links, const Ids& connectors, const ControlPathRef& p) {
    return p.connectorId.empty() ? links.contains(p.linkId) : connectors.contains(p.connectorId); // a Connector path names no Link
}
}
StudyAreaCut studyAreaCut(const Network& n, const std::vector<std::string>& nodes) {
    StudyAreaCut cut;
    Ids links, connectors, outLegs, inLegs, nodeLinks;
    for (const auto& node : n.nodes) {
        const auto m = nodeMembership(n, node);
        nodeLinks.insert(m.links.begin(), m.links.end());
        for (const auto& l : m.inLegs) nodeLinks.insert(l.linkId);
        for (const auto& l : m.outLegs) nodeLinks.insert(l.linkId);
    }
    for (std::size_t i = 0; i < nodes.size(); ++i) {
        const auto path = "studyArea.nodes[" + std::to_string(i) + "]";
        const auto node = std::find_if(n.nodes.begin(), n.nodes.end(), [&](const Node& x) { return x.id == nodes[i]; });
        if (node == n.nodes.end()) continue; // the codec's STUDY_AREA_NODE_UNKNOWN
        // Detail first: a Node without it always reads NODE_NO_TURN, which is not the fix to name.
        if (!nodeHasDetail(n, *node) || node->coarse) { cut.issues.push_back({"MICRO_AREA_NODE_COARSE", path}); continue; }
        const auto m = nodeMembership(n, *node);
        if (!m.issues.empty()) {
            cut.issues.push_back({"MICRO_AREA_NODE_INVALID", path});
            for (const auto& code : m.issues) cut.issues.push_back({code, path});
            continue;
        }
        links.insert(m.links.begin(), m.links.end());
        connectors.insert(m.connectors.begin(), m.connectors.end());
        for (const auto& l : m.inLegs) { links.insert(l.linkId); inLegs.insert(l.linkId); }
        for (const auto& l : m.outLegs) { links.insert(l.linkId); outLegs.insert(l.linkId); }
    }
    const auto ahead = reach(n, outLegs, nodeLinks, true), behind = reach(n, inLegs, nodeLinks, false);
    for (const auto& id : ahead) if (behind.contains(id)) links.insert(id);
    for (const auto& c : n.connectors)
        if (links.contains(c.from.linkId) && links.contains(c.to.linkId)) connectors.insert(c.id);
    for (const auto& l : n.links) {
        if (!links.contains(l.id)) continue;
        cut.links.push_back(l.id);
        bool fedByKept = false, fedByOther = false, feedsKept = false, feedsOther = false;
        for (const auto& c : n.connectors) {
            if (c.to.linkId == l.id) (links.contains(c.from.linkId) ? fedByKept : fedByOther) = true;
            if (c.from.linkId == l.id) (links.contains(c.to.linkId) ? feedsKept : feedsOther) = true;
        }
        if (!fedByKept || fedByOther) cut.entryGates.push_back(l.id);
        if (!feedsKept || feedsOther) cut.exitGates.push_back(l.id);
    }
    for (const auto& c : n.connectors) if (connectors.contains(c.id)) cut.connectors.push_back(c.id);
    if (cut.issues.empty() && cut.links.empty()) cut.issues.push_back({"MICRO_AREA_EMPTY", "studyArea.nodes"});
    return cut;
}
Network cutNetwork(const Network& n, const StudyAreaCut& cut, const std::vector<std::string>& nodes) {
    const Ids links(cut.links.begin(), cut.links.end()), connectors(cut.connectors.begin(), cut.connectors.end()), area(nodes.begin(), nodes.end());
    Network out; out.id = n.id; out.drivingSide = n.drivingSide;
    for (auto l : n.links) if (links.contains(l.id)) { l.streetSegmentId.reset(); out.links.push_back(std::move(l)); }
    Ids paths; // a head names a Connector path, not the Connector
    for (const auto& c : n.connectors)
        if (connectors.contains(c.id)) {
            out.connectors.push_back(c);
            for (int i = 0; i < std::max(c.fromLaneCount, c.toLaneCount); ++i) paths.insert(connectorPathId(c, i));
        }
    for (const auto& h : n.signalHeads)
        if (links.contains(h.lane.linkId) && (h.connectorId.empty() || paths.contains(h.connectorId))) out.signalHeads.push_back(h);
    const auto& row = n.rightOfWay;
    Ids lines, areas;
    for (const auto& w : row.waitingLines)
        if (kept(links, connectors, w.point.path)) { out.rightOfWay.waitingLines.push_back(w); lines.insert(w.id); }
    const auto sideKept = [&](const ConflictSide& s) { return kept(links, connectors, s.path) && (s.waitingLineId.empty() || lines.contains(s.waitingLineId)); };
    for (const auto& a : row.conflictAreas)
        if (sideKept(a.first) && sideKept(a.second)) { out.rightOfWay.conflictAreas.push_back(a); areas.insert(a.id); }
    for (const auto& r : row.priorityRules) if (areas.contains(r.conflictAreaId)) out.rightOfWay.priorityRules.push_back(r);
    for (const auto& s : row.stopControls)
        if (lines.contains(s.waitingLineId) && std::all_of(s.conflictAreaIds.begin(), s.conflictAreaIds.end(), [&](const auto& id) { return areas.contains(id); }))
            out.rightOfWay.stopControls.push_back(s);
    Ids heads;
    for (const auto& h : out.signalHeads) heads.insert(h.id);
    for (const auto& q : n.queueCounters)
        if (std::all_of(q.lines.begin(), q.lines.end(), [&](const MeasurementLine& m) {
                return m.point ? kept(links, connectors, m.point->path) : heads.contains(m.referenceId) || lines.contains(m.referenceId); }))
            out.queueCounters.push_back(q);
    for (const auto& s : n.travelTimeSections)
        if (links.contains(s.start.linkId) && links.contains(s.end.linkId)) out.travelTimeSections.push_back(s);
    for (const auto& node : n.nodes) if (area.contains(node.id)) out.nodes.push_back(node);
    return out;
}
}
