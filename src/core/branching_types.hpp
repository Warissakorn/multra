#pragma once
#include <string>

namespace multra {
// D623 (M3_CONTRACT, branching): two Connectors leaving the same lane at the same station, and how far
// past the split -- metres along each one's runtime segment -- the editor's branching area reaches.
// COMPILED from that area, never authored; empty without a diverge, which runs the old path exactly.
struct BranchingSpan {
    std::string firstSegmentId; double firstLength{};
    std::string secondSegmentId; double secondLength{};
    bool operator==(const BranchingSpan&) const = default;
};
// ScenarioIndex::siblingsOfSegment: a segment's sibling, and the area's reach on this segment: a body
// whose rear is short of it also occupies the sibling.
struct BranchSibling { std::size_t segment{}; double ownTouch{}; };
}
