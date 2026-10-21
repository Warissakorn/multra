#pragma once
// S0.2 (NODES §3, D262, D263): what a Node holds, derived from the Micro detail inside its polygon
// on every read and never stored. Pure, and never throws: a drawing the author is still working on
// gives refusals, not exceptions.
#include "network.hpp"
#include <optional>

namespace multra {
// A boundary crossing of a Link, at metres along its reference polyline (`link.geometry`).
struct NodeLeg { std::string linkId, name; double station{}; bool operator==(const NodeLeg&) const = default; };
// One object of a turn's path: a Link from one station to another, or a whole Connector (0, 0).
struct TurnSpan { std::string objectId; double from{}, to{}; bool operator==(const TurnSpan&) const = default; };
// An (in-leg, out-leg) pair, indices into NodeMembership's legs, with every object chain joining
// them; several lanes or Connectors of one pair make one turn.
struct NodeTurn {
    std::size_t in{}, out{}; std::string name; std::vector<std::vector<TurnSpan>> paths;
    bool operator==(const NodeTurn&) const = default;
};
// A conflict area inside whose two sides lie on two different turns; indices into `turns`.
struct NodeConflict {
    std::string conflictAreaId; std::size_t first{}, second{};
    ConflictKind kind{}; ConflictPriority priority{};
    bool operator==(const NodeConflict&) const = default;
};
struct NodeMembership {
    // Inside the polygon: Links and Connectors wholly inside, and the control objects whose points are.
    std::vector<std::string> links, connectors, conflictAreas, waitingLines, signalHeads;
    std::vector<NodeLeg> inLegs, outLegs;
    std::vector<NodeTurn> turns;
    std::vector<NodeConflict> conflicts;
    std::optional<SectionControl> control;
    // NODE_LINK_REENTERS, NODE_CONNECTOR_CROSSES, NODE_NO_TURN. When any is present the Node gives
    // no legs, turns, conflicts or control.
    std::vector<std::string> issues;
    bool operator==(const NodeMembership&) const = default;
};
NodeMembership nodeMembership(const Network&, const Node&);
// The refusals of every Node with a valid polygon, at "nodes[i]"; reported as advisories.
std::vector<ValidationIssue> nodeMembershipIssues(const Network&);
}
