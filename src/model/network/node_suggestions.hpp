#pragma once
// S0.5 (NODES §6, D265): Suggest nodes. One `generated` polygon per junction whose conflicts no
// Node covers yet. Pure; derived from the drawing alone, so it needs no number of its own.
#include "network.hpp"

namespace multra {
// A junction is a cluster of conflicts (authored and automatic) joined through their Connectors,
// and Connectors that leave or arrive at the same place on a Link. Its polygon is the convex hull
// of the cluster's Connectors and conflict points, padded by the widest lane at those Connectors'
// ends. Overlapping suggestions merge; one overlapping an existing Node is dropped.
std::vector<Node> nodeSuggestions(const Network&);
// The convex hull, counter-clockwise from the lowest-left point; fewer than three distinct points come back as they are.
std::vector<Point> convexHull(std::vector<Point>);
}
