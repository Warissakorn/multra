#pragma once
#include "node_membership.hpp"
#include <map>

namespace multra {
enum class JunctionMovement { left, through, right };
struct LayoutMovement {
    std::size_t turn{}; // index into membership.turns
    std::string name;   // travel direction + L/T/R: NBL, EBT, ...
    JunctionMovement movement{};
    bool operator==(const LayoutMovement&) const = default;
};
struct LayoutPocket {
    PocketSide side{};
    double storage{}, taper{}; // metres on the Link/reference spine, not the sloping lane path
    std::string connector;
    bool operator==(const LayoutPocket&) const = default;
};
struct LayoutLane {
    std::string id; double width{}, stopLine{};
    std::vector<std::size_t> turns; // membership.turns, in membership order
    std::optional<LayoutPocket> pocket;
    bool operator==(const LayoutLane&) const = default;
};
struct LayoutApproach {
    std::string inLink;
    double bearing{}; // travel bearing, degrees clockwise from north [0, 360)
    ApproachDirection direction{};
    std::vector<LayoutLane> lanes; // Link order: kerb to far on both driving sides
    bool operator==(const LayoutApproach&) const = default;
};
struct ApproachLayout {
    NodeMembership membership;
    std::vector<LayoutApproach> approaches;
    std::vector<LayoutMovement> movements;
    std::vector<ValidationIssue> issues;
};
// J2 (JUNCTION_TEMPLATES §6): derived only, on a structurally valid network; never writes it. Refuses duplicate compass
// directions, compass ties, U-turns (>= 135 degrees) and membership issues, leaving no layout.
// Through is strictly within 45 degrees. Overrides apply to this Node's in-leg Link ids; their
// Node.approachDirections supplies stored choices; per-call keys take precedence. Neither changes the turn angle.
// A pocket's predecessor walk stops at a Connector with a run inside any Node, including through movements.
ApproachLayout approachLayout(const Network&, const Node&, const ApproachDirections& = {});
}
