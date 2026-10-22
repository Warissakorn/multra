#pragma once
// S3.3 (ROADS §3, D305): what an authored Road means on its own. Its template is a definition
// catalog entry, checked with the document (src/project/road_codec.hpp).
#include "network.hpp"

namespace multra {
// Paths are "roads[i]..." and "links[i].roadId":
// - a blank or duplicate id (INVALID_ID, DUPLICATE_ID, against every other network id);
// - a centreline with fewer than two distinct finite points (ROAD_GEOMETRY_INVALID);
// - ROAD_LINK_MISMATCH: a `links` entry naming no Link, a Link listed twice or whose roadId is not
//   this Road, or a Link whose roadId names no Road or a Road that does not list it.
std::vector<ValidationIssue> roadIssues(const Network&);
}
