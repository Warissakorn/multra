#include "leaders.hpp"

namespace trafficsim {
std::optional<Leader> closestVehicle(const Vehicle& vehicle, const std::vector<RoutePart>& parts,
                                     const std::vector<OccupiedSpan>& spans, const SpanBuckets& buckets,
                                     std::uint64_t* id, const LateralFilter* lateral) {
    std::optional<Leader> nearest;
    for (const auto& part : parts) {
        if (part.start + part.length < vehicle.distance) continue;
        // Parts ascend by start and every span's rear is at least 0, so no span on this part or
        // any later one can have a gap strictly below the nearest already found: stop, and the
        // same leader is chosen as by the full scan.
        if (nearest && part.start - vehicle.distance >= nearest->gap) break;
        // Only this segment's spans; the segmentId comparison the full scan did is now implicit.
        for (auto i = buckets.start[part.segmentIndex]; i < buckets.start[part.segmentIndex + 1]; ++i) {
            const auto& span = spans[buckets.items[i]];
            if (span.vehicleId == vehicle.id || part.start + span.front < vehicle.distance - 1e-9 || !lateralCounts(lateral, span)) continue;
            const double gap = part.start + span.rear - vehicle.distance;
            if (!nearest || gap < nearest->gap) {
                nearest = Leader{gap, span.speed, span.acceleration};
                if (id) *id = span.vehicleId;
            }
        }
    }
    // D210: a straddler also follows the lane it covers; a strict < keeps its own lane's leader on a tie.
    forCoveredSpans(vehicle, spans, buckets, lateral, [&](double least) { return nearest && least >= nearest->gap; },
                    [&](double gap, const OccupiedSpan& span) {
                        if (!nearest || gap < nearest->gap) { nearest = Leader{gap, span.speed, span.acceleration}; if (id) *id = span.vehicleId; }
                    });
    return nearest;
}
}
