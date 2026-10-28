#pragma once
// S3.5 (STREET_SEGMENTS §5, D309): Find crossings. Pure: where StreetSegments on one level cross or touch, read from
// their centrelines alone. StreetSegments on different levels never meet, and an overpass pair is not asked again.
#include "network.hpp"

namespace multra {
struct StreetSegmentAtCrossing {
    std::string streetSegmentId; double station{}; bool end{}; // end: within the tolerance of either end
    bool operator==(const StreetSegmentAtCrossing&) const = default;
};
struct StreetSegmentCrossing {
    Point at; int level{};
    std::vector<StreetSegmentAtCrossing> streetSegments; // by StreetSegment order in the network
    int legs{};          // an interior station gives two legs, an end one
    bool endpoint{};     // a StreetSegment ends here, so it cannot be an overpass
    bool operator==(const StreetSegmentCrossing&) const = default;
};
// Events for every pair of StreetSegments on one level that is not an overpass: each proper crossing of their
// centrelines, and each endpoint of one within `tolerance` of the other. Events on one level within `tolerance` of
// one another are one place, at their mean. A place is listed with two StreetSegments or more and three legs
// or more, so two StreetSegments meeting end to end are not, and only where no Node already covers it (a
// confirmed junction is not asked again). Sorted by level, then x, then y.
std::vector<StreetSegmentCrossing> findCrossings(const Network&, double tolerance);
}
