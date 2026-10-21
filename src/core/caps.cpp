#include "conflicts.hpp"
#include "leaders.hpp"
#include <algorithm>
#include <cmath>

namespace multra {
namespace {
// The part of the leader's body that is not on this route's segments: a vehicle entering at a join
// has its rear on its own connector, so its occupancy here starts at the join (MERGE_CLAMPS §1).
double hiddenLength(const std::vector<OccupiedSpan>& spans, const std::vector<RoutePart>& parts, std::uint64_t id, double length) {
    double visible = 0;
    for (const auto& span : spans)
        if (span.vehicleId == id && std::any_of(parts.begin(), parts.end(), [&](const RoutePart& p) { return p.segmentIndex == span.segmentIndex; }))
            visible += span.front - span.rear;
    return std::max(0.0, length - visible);
}
}
double relievedCap(const Scenario& scenario, const std::vector<Vehicle>& vehicles, const std::vector<VehicleRefs>& refs, const Vehicle& vehicle,
                   const std::vector<RoutePart>& parts, const std::vector<OccupiedSpan>& spans, const SpanBuckets& buckets,
                   const LateralFilter* lateral, const Leader& leader, double leaderCap, double holdCap, double giveWayLine, double proposed, double dt) {
    const double cap = leaderCap; // the leader binds: tickCap called this only below the holds
    const auto& rules = scenario.clampRules;
    if (!rules.leaderTravel && !rules.sideEntryAtLine) return cap;
    // The same search as the step's, now with the id (kept off the hot path); vehicles are in id order.
    std::uint64_t leaderId = 0;
    closestVehicle(vehicle, parts, spans, buckets, &leaderId, lateral);
    const auto it = std::lower_bound(vehicles.begin(), vehicles.end(), leaderId, [](const Vehicle& v, std::uint64_t id) { return v.id < id; });
    if (it == vehicles.end() || it->id != leaderId) return cap;
    const auto& type = scenario.vehicleTypes[refs[static_cast<std::size_t>(it - vehicles.begin())].type];
    const double hidden = hiddenLength(spans, parts, leaderId, type.length);
    double relieved = cap;
    if (rules.leaderTravel) { // §2 B: the leader cannot stop sooner than its type's maximum braking allows
        const double v = leader.speed, b = type.maxDeceleration;
        const double travel = v >= b * dt ? v * dt - 0.5 * b * dt * dt : v * v / (2 * b);
        relieved = std::min(holdCap, leaderCap + std::max(0.0, travel - hidden));
    }
    // §2 M: the rule, which holds the vehicle at a line nearer than the leader, keeps the two apart.
    if (rules.sideEntryAtLine && proposed > relieved && relieved < holdCap && std::isfinite(giveWayLine) && giveWayLine <= leader.gap && hidden > 1e-9)
        relieved = holdCap;
    return relieved;
}
}
