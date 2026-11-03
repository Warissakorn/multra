#pragma once
// UTDF U1 (UTDF_EXPORT §7 U1 contract, "Legs"): which Nodes are written, where each leg comes from
// and goes to, and the numbers of the external ends. Internal to utdf_export.cpp.
#include "utdf_export.hpp"
#include "../model/network/approach_layout.hpp"
#include <map>
#include <optional>

namespace multra::utdf {
// A leg walked to its far end: the Links in travel order, and the written Node (index into
// `Plan::nodes`) at the far end, or none for an external end.
struct Walk { std::vector<const Link*> links; std::optional<std::size_t> node; };
struct WrittenNode {
    const Node* node{}; int number{}; Point centroid; ApproachLayout layout;
    std::vector<Walk> in, out;            // per membership in-leg / out-leg
    std::vector<ApproachDirection> outDirection; // travel direction of each out-leg
};
struct ExternalEnd {
    int number{}; Point at; std::size_t node{}; // the written Node whose leg ends here
    std::optional<std::size_t> in, out;         // that Node's in-leg and out-leg indices
};
struct Plan {
    std::vector<WrittenNode> nodes;        // in document order
    std::vector<ExternalEnd> ends;         // by number
    std::map<std::pair<std::size_t,std::size_t>,int> inEnd, outEnd; // (node, leg) -> external number
    std::vector<UtdfNote> refusals;
};
Plan plan(const Network&);
Point centroid(const std::vector<Point>& polygon);
// Along the Links' polylines, with straight joins, from `from` to `to`.
double walkLength(Point from, const Walk&, Point to);
}
