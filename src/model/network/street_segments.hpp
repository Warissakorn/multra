#pragma once
// S3.3 (STREET_SEGMENTS §3, D305): what an authored StreetSegment means on its own. Its template is a definition
// catalog entry, checked with the document (src/project/street_segment_codec.hpp).
#include "network.hpp"

namespace multra {
// Paths are "roads[i]..." and "links[i].streetSegmentId":
// - a blank or duplicate id (INVALID_ID, DUPLICATE_ID, against every other network id);
// - a centreline with a non-finite point, fewer than two points, or two equal consecutive points
//   (STREET_SEGMENT_GEOMETRY_INVALID);
// - STREET_SEGMENT_LINK_MISMATCH: a `links` entry naming no Link, a Link listed twice or whose streetSegmentId is not
//   this StreetSegment, or a Link whose streetSegmentId names no StreetSegment or a StreetSegment that does not list it.
// S3.5: STREET_SEGMENT_OVERPASS_UNKNOWN at overpasses[i] for a pair naming no StreetSegment or one StreetSegment twice.
std::vector<ValidationIssue> streetSegmentIssues(const Network&);
// D382: whether a StreetSegment's generated Link runs forward, with the centreline: its end projects farther
// along the centreline than its start. Read from geometry, not from the Link's place in `links`, which
// a detach shifts; only a Link whose ends project to one station falls back to being listed first.
bool forwardOnStreetSegment(const StreetSegment&, const Link&);
// S3.4 (STREET_SEGMENTS §3, §17, D308): the mean of two opposite carriageways' centrelines, digitised along
// `forward`; `backward` is given in its own direction of travel. Equal vertex counts pair vertex by
// vertex, which is exact for two offsets of one reference (oppositeLink); otherwise each vertex is
// paired with its closest point on the other line and the midpoints are ordered along `forward`.
std::vector<Point> meanCentreline(const std::vector<Point>& forward, const std::vector<Point>& backward);
}
