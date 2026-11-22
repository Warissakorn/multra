#pragma once
// PRIORITY_RULES (D614-D616): the priority-rule lab, data/projects/priority-rule-lab.traffic.json, for
// the owner's Windows look (OWNER_SITTING_WINDOWS item 51). Not a study: the T-junction's roads, routes
// and demand (fixture::tJunction, left-hand, nothing authored) with two Vissim priority rules in
// place of conflict areas, placed through the Stop line tool's own commands. Regenerate with
// multra-priority-rule-fixture, never by hand; `priority_marker` checks the file is this output.
//
// Left-hand traffic: eastbound on the north side, the minor road from the north heading south. Its
// right turn crosses eastbound and joins westbound; its left turn joins eastbound only.
//   - "Right turn gives way to both streams" (Yield): a line 1 m into the right turn; a marker on
//     eastbound at the end of the drawn crossing, and one on westbound where the turn joins.
//   - "Left turn joins eastbound" (Stop): a line 1 m into the left turn; a marker on eastbound where
//     the turn joins; a Stop on the line with no area (D615).
// Every marker has Vissim's defaults, 3.0 s and 5.0 m. The automatic crossing stays passive (grey).
#include "t_junction_network.hpp"
#include "../src/commands/priority_marker_commands.hpp"
#include <stdexcept>

namespace multra::fixture {
struct PriorityRuleLab {
    ProjectDocument document;
    TJunction roads;
    std::string rightLine, leftLine, rightRule, leftRule;
};
inline PriorityRuleLab priorityRuleLab() {
    TJunctionOptions o; o.side = DrivingSide::left; o.control = std::nullopt; o.authoredControls = false;
    PriorityRuleLab lab;
    lab.roads = tJunction(o);
    auto& d = lab.document = lab.roads.document;
    d.network.id = "priority-rule-lab";
    const auto& t = lab.roads;
    const auto connectorPath = [&](const std::string& id) {
        for (const auto& c : d.network.connectors) if (c.id == id) return ControlPathRef{"", "", c.id, c.from.laneId, c.to.laneId};
        throw std::invalid_argument("UNKNOWN_CONNECTOR");
    };
    const auto lanePath = [&](const std::string& link) {
        for (const auto& l : d.network.links) if (l.id == link) return ControlPathRef{l.id, l.lanes.front().id, "", "", ""};
        throw std::invalid_argument("UNKNOWN_LINK");
    };
    const auto joinStation = [&](const std::string& connector) {
        for (const auto& c : d.network.connectors) if (c.id == connector) return attachmentStation(d.network, c.to, false);
        throw std::invalid_argument("UNKNOWN_CONNECTOR");
    };
    // Where the right turn's drawn crossing of eastbound ends, on eastbound (the automatic, passive crossing).
    double crossingEnd = -1;
    for (const auto& a : automaticConflicts(d.network))
        if (a.kind == ConflictKind::crossing)
            for (const auto* side : {&a.first, &a.second}) if (side->path.linkId == t.eastbound) crossingEnd = side->exitStation;
    if (crossingEnd < 0) throw std::logic_error("PRIORITY_LAB_CROSSING");
    lab.rightLine = placeStopLine(d, {connectorPath(t.crossingTurn), 1});
    lab.rightRule = addPriorityMarker(d, lab.rightLine, {lanePath(t.eastbound), crossingEnd});
    addPriorityMarker(d, lab.rightLine, {lanePath(t.westbound), joinStation(t.crossingTurn)});
    lab.leftLine = placeStopLine(d, {connectorPath(t.nearTurn), 1});
    lab.leftRule = addPriorityMarker(d, lab.leftLine, {lanePath(t.eastbound), joinStation(t.nearTurn)});
    putStopControl(d, {"", "", lab.leftLine, StopMode::stop, {}});
    for (auto& r : d.network.rightOfWay.priorityRules)
        r.name = r.id == lab.rightRule ? "Right turn gives way to both streams" : "Left turn joins eastbound";
    for (auto& w : d.network.rightOfWay.waitingLines)
        w.name = w.id == lab.rightLine ? "Right turn line" : "Left turn line";
    validateDocument(d);
    return lab;
}
}
