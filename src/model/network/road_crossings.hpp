#pragma once
// S3.5 (ROADS §5, D309): Find crossings. Pure: where Roads on one level cross or touch, read from
// their centrelines alone. Roads on different levels never meet, and an overpass pair is not asked again.
#include "network.hpp"

namespace multra {
struct RoadAtCrossing {
    std::string roadId; double station{}; bool end{}; // end: within the tolerance of either end
    bool operator==(const RoadAtCrossing&) const = default;
};
struct RoadCrossing {
    Point at; int level{};
    std::vector<RoadAtCrossing> roads; // by Road order in the network
    int legs{};          // an interior station gives two legs, an end one
    bool endpoint{};     // a Road ends here, so it cannot be an overpass
    bool operator==(const RoadCrossing&) const = default;
};
// Events for every pair of Roads on one level that is not an overpass: each proper crossing of their
// centrelines, and each endpoint of one within `tolerance` of the other. Events within `tolerance` of
// one another are one place, at their mean. A place is listed with two Roads or more and three legs
// or more, so two Roads meeting end to end are not, and only where no Node already covers it (a
// confirmed junction is not asked again). Sorted by level, then x, then y.
std::vector<RoadCrossing> findCrossings(const Network&, double tolerance);
}
