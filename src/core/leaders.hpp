#pragma once
#include "following.hpp"
#include "lateral.hpp"
#include "routes.hpp"
#include <cstdint>
#include <optional>
#include <vector>

namespace trafficsim {
// The physical leader: the nearest rear bumper ahead on the vehicle's own route parts, filtered by
// LATERAL §6, and (D210) on each lane its body covers. `id`, when asked for, receives the leader's
// vehicle id -- only admission needs it (M3.2.3c).
std::optional<Leader> closestVehicle(const Vehicle& vehicle, const std::vector<RoutePart>& parts,
                                     const std::vector<OccupiedSpan>& spans, const SpanBuckets& buckets,
                                     std::uint64_t* id = nullptr, const LateralFilter* lateral = nullptr);
// D210 (LATERAL §6): calls `seen(gap, span)` for every span ahead of the body's front on each lane it
// covers (`lateral->covered`), with the own scan's skips, walking that lane on through its one successor
// (`onlyNext`). It stops a lane once `done(least)` holds for the least gap still possible there.
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
}
