#include "leaders.hpp"

namespace multra {
void coveredLeader(const Vehicle& vehicle, const std::vector<OccupiedSpan>& spans, const SpanBuckets& buckets,
                   const LateralFilter& lateral, std::optional<Leader>& nearest, std::uint64_t* id) {
    forCoveredSpans(vehicle, spans, buckets, &lateral, [&](double least) { return nearest && least >= nearest->gap; },
                    [&](double gap, const OccupiedSpan& span) {
                        if (!nearest || gap < nearest->gap) { nearest = Leader{gap, span.speed, span.acceleration}; if (id) *id = span.vehicleId; }
                    });
}
}
