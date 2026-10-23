#pragma once
#include "definition.hpp"
#include "../network/network.hpp"
#include <map>
#include <optional>

namespace multra {
// S3.6a (ROADS §6, §9, D310): what Macro and Meso compile from Roads without generated Links and
// from Nodes without detail, shared by compileMacroNetwork and compileMesoNetwork.
// - Pieces: each Link-less Road gives one directed piece per direction of its template (forward
//   along the centreline, backward against it), trimmed at the legs of the Nodes without detail it
//   ends in (coarse_nodes.hpp). Lanes are the override, else the template's; the type is the
//   template's `linkType`. Stations are on the centreline, so a backward piece's `to` < `from`.
// - Joins: a Node without detail (with or without `coarse`; absent fields are the defaults), and
//   each group of free Road ends within roadEndTolerance of one another, as a Link continuing
//   through a station is in Meso. A free end alone joins nothing. A Node's arms are the pieces
//   ending and starting there and (D311) its Link legs (nodeLinkLegs), whose part inside the Node
//   is in `dropped` and is not compiled.
// - Turns: every (in arm, out arm) pair of different Roads (a Link without `roadId` counts as its
//   own Road); at a Node, minus `bannedTurns`. Turns have no geometry, so no free time (D301). A
//   signalised Node gives each turn `signal` = "node:<id>" and `green` = the summed greens of the
//   stages serving it over the cycle (D299's g/C); a turn no stage serves gets 0.
// Refusals at network.roads[i].crossSectionId: MACRO_ROAD_TEMPLATE_MISSING (no template),
// ROAD_CROSS_SECTION_UNKNOWN, MACRO_LINK_TYPE_MISSING (the template names no type),
// MACRO_LINK_TYPE_UNKNOWN. A Road refused for its template gives no piece.
// Advisory NODE_PRIORITY_UNDETERMINED at network.nodes[i]: a priority Node of three legs or more,
// with no `major`, whose ranks (a Link-less Road's template type, else its first Link's type)
// decide nothing (majorByRank).
constexpr double roadEndTolerance = 2.0; // m: Find crossings' default snap tolerance (ROADS §1)
struct RoadPiece {
    std::string roadId; std::size_t road{}; bool forward{}; double from{}, to{}, length{}, lanes{};
    const LinkType* type{}; std::string linkType;
    std::optional<std::size_t> start, end; // joins
};
// An arm of a join: a piece, or a Link leg at its boundary station. `road` names it for turns.
struct JoinArm { std::optional<std::size_t> piece; std::string linkId; double station{}; std::string road, label; };
struct RoadTurn { std::size_t in{}, out{}; std::string name, signal; double green{}; }; // into ins, outs
struct RoadJoin {
    std::optional<std::size_t> node; // index into network.nodes; absent for free Road ends
    std::string name, control;       // control as MesoNode's: "signalised", "unsignalised" or empty
    std::vector<JoinArm> ins, outs;
    std::vector<RoadTurn> turns;
};
struct RoadCompile {
    std::vector<RoadPiece> pieces; std::vector<RoadJoin> joins;
    std::map<std::string, std::vector<std::pair<double, double>>> dropped; // Link id -> station ranges
    std::vector<ValidationIssue> issues, advisories;
};
RoadCompile compileRoads(const Network&, const std::vector<CrossSection>&, const std::vector<LinkType>&);
// The priority hierarchy (ROADS §6, 4a): the legs with the highest rank are major. Nothing when a
// leg has no rank or every leg shares the top rank (undetermined). Legs are (Road id, rank).
std::optional<std::vector<std::string>> majorByRank(const std::vector<std::pair<std::string, std::optional<int>>>& legs);
}
