#pragma once
// S0 (NODES §2, D259-D261): what an authored Node means. Its legs, turns and control are derived
// from the network inside its polygon (NODES §3, slice S0.2); nothing here references other objects.
#include "right_of_way.hpp"

namespace multra {
// Blank or duplicate ids (against every other network id), a polygon that is not simple
// (NODE_POLYGON_INVALID), two Nodes that share any point (NODE_OVERLAP, at the later one).
// Paths are "nodes[i]...".
std::vector<ValidationIssue> nodeIssues(const Network&);
}
