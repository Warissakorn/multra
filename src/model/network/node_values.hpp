#pragma once
#include "node_analysis.hpp"
#include "junction_templates.hpp"

namespace multra {
struct Point { double x{}, y{}; bool operator==(const Point&) const = default; };
// S3.6a (STREET_SEGMENTS §6, D310): coarse values, stored only on a Node without detail. Turns are (in StreetSegment,
// out StreetSegment) pairs naming the Node's legs (coarse_nodes.hpp); absent fields are the defaults.
enum class CoarseControl { priority, signalised, allWayStop };
struct CoarseStage { double green{}; std::vector<std::pair<std::string, std::string>> turns; bool operator==(const CoarseStage&) const = default; };
struct CoarseSignal { double cycle{}; std::vector<CoarseStage> stages; bool operator==(const CoarseSignal&) const = default; };
struct NodeCoarse {
    std::optional<CoarseControl> control{};  // absent: priority
    std::vector<std::string> major{};        // empty: by the link-type rank
    std::vector<std::pair<std::string, std::string>> bannedTurns{};
    std::optional<CoarseSignal> signal{};    // required when signalised
    bool operator==(const NodeCoarse&) const = default;
};
// S0 (NODES §2): a polygon over a junction. Membership stays derived; optional inputs carry their own keys.
// `generated` marks one the program drew, until a person edits it.
struct Node {
    std::string id, name; std::vector<Point> polygon; bool generated{};
    std::optional<NodeCoarse> coarse{}; std::optional<NodeAnalysis> analysis{}; // schema 53 (S3.6a), 58 (M13 A1)
    std::optional<NodeJunction> junction{}; // M14 J3a, schema 61: a recipe, never the layout
    ApproachDirections approachDirections{}; // schema 61, independent of the recipe
    bool operator==(const Node&) const = default;
};
}
