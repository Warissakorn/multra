#pragma once
#include "network.hpp"
#include <array>
#include <map>

namespace multra {
// P1/P4 intersect the local Connector and Link edges; P2 is the centre
// of the attached lane range; P3 is the nearest point to P2 on the far LINK edge.
// Near/far are measured from the Connector body, not fixed left/right labels.
struct ConnectorMouth {
    std::array<Point,4> points;
    bool firstBoundaryNear{};
    // M3.2.9b: one point per Link lane boundary of the attached range, in the
    // order of the Connector's own boundaries: [0] is where the first rail ends, back() the last.
    // The two range edges are P1 and P4; interior points are built as they are (D76).
    std::vector<Point> boundaries;
};
struct ConnectorSurface {
    std::vector<Point> outline; // Closed implicitly, in perimeter order.
    std::vector<ConnectorMarking> markings;
    std::optional<ConnectorMouth> source, target;
    std::vector<std::vector<Point>> boundaries; // Same rails used by paint and conflict coverage.
    bool selfIntersecting{}; // Report the authored fold; never delete a valid mouth to hide it.
};
// One P1-P4 construction for paint, picking and conflict strips. Non-coincident parallel
// lines have no mouth: outline is empty, open rails remain editable, diagnostics report it.
// Coincident parallel edges use their attachment points (the continuous straight join).
ConnectorSurface connectorSurface(const Network&, const Connector&);
// Call-local reuse only. The bound Network must remain unchanged for this object's lifetime.
// No cross-edit invalidation policy: automaticConflicts owns and destroys it in one call.
class ConnectorSurfaceCache {
public:
    explicit ConnectorSurfaceCache(const Network& network) : network_(network) {}
    const ConnectorSurface& get(const Connector& connector) {
        const auto found = surfaces_.find(connector.id);
        if (found != surfaces_.end()) return found->second;
        return surfaces_.emplace(connector.id, connectorSurface(network_, connector)).first->second;
    }
private:
    const Network& network_;
    std::map<std::string, ConnectorSurface> surfaces_;
};
// P2 on its own (M3.2.9f, D77): the middle of the attached Link lane range at the attachment
// station -- where an end grip is drawn and where a dropped end is measured from.
std::optional<Point> connectorRangeCentre(const Network&, const Connector&, bool start);
// The Connector's grips: connectorCentreline, with its two ends on connectorRangeCentre, so an
// end grip sits on the Link at every arrival angle, not on the body's square-end fallback.
std::vector<Point> connectorGrips(const Network&, const Connector&);
// Convert an interior centre-axis grip to stored geometry. The returned edit clears laneBlend.
std::vector<Point> connectorGeometryWithGrip(const Network&, const Connector&, std::size_t, Point);
}
