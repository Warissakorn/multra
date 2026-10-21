#pragma once
#include "following.hpp"
#include "lateral.hpp"
#include "routes.hpp"
#include <cstdint>
#include <optional>
#include <vector>

namespace multra {
// D210 (LATERAL §6): calls `seen(gap, span)` for every span ahead of the body's front on each lane it
// covers (`lateral->covered`), with the own scan's skips, walking that lane on through its one successor
// (`onlyNext`). It stops a lane once `done(least)` holds for the least gap still possible there.
// The covered-lane part of closestVehicle (D210): lowers `nearest` (and sets `id`) for a nearer
// span on a covered lane; a strict < keeps the own lane's leader on a tie.
void coveredLeader(const Vehicle& vehicle, const std::vector<OccupiedSpan>& spans, const SpanBuckets& buckets,
                   const LateralFilter& lateral, std::optional<Leader>& nearest, std::uint64_t* id);
template <class Done, class Seen>
void forCoveredSpans(const Vehicle& vehicle, const std::vector<OccupiedSpan>& spans, const SpanBuckets& buckets,
                     const LateralFilter* lateral, Done&& done, Seen&& seen) {
    if (!lateral) return;
    for (const auto& lane : lateral->covered) {
        double offset = 0; // where `segment` starts, in metres along the covered lane from lane.segment
        auto segment = lane.segment;
        // A circuit's chain closes on itself: never walk more segments than exist.
        for (std::size_t walked = 0; walked < lateral->onlyNext->size(); ++walked) {
            if (done(offset - lane.front)) break;
            for (auto i = buckets.start[segment]; i < buckets.start[segment + 1]; ++i) {
                const auto& span = spans[buckets.items[i]];
                if (span.vehicleId == vehicle.id || offset + span.front < lane.front - 1e-9 || !lateralCounts(lateral, span)) continue;
                seen(offset + span.rear - lane.front, span);
            }
            offset += lateral->scenario->segments[segment].length;
            segment = (*lateral->onlyNext)[segment];
            if (segment == SIZE_MAX) break;
        }
    }
}
// The physical leader: the nearest rear bumper ahead on the vehicle's own route parts, filtered by
// LATERAL §6, and (D210) on each lane its body covers. `id`, when asked for, receives the leader's
// vehicle id -- only admission needs it (M3.2.3c).
// Inline, as it was in simulation.cpp before D210 (D212): out of line, with the covered scan inside,
// it cost the switched-off four-leg run 1.7 % Ir.
inline std::optional<Leader> closestVehicle(const Vehicle& vehicle, const std::vector<RoutePart>& parts,
                                            const std::vector<OccupiedSpan>& spans, const SpanBuckets& buckets,
                                            std::uint64_t* id = nullptr, const LateralFilter* lateral = nullptr) {
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
    // D210: a straddler also follows the lane it covers (out of line: rare, and kept off the hot path).
    if (lateral && !lateral->covered.empty()) coveredLeader(vehicle, spans, buckets, *lateral, nearest, id);
    return nearest;
}
}
