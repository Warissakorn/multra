#pragma once
// M10.2 G1 (TRIP_GENERATION §3-§4, D468-D470): productions and attractions per zone and purpose from
// resolved zone values, and attractions balanced to productions (Ortuzar and Willumsen 2011,
// Eqs. 4.14 and 4.16). Pure arithmetic: zones and attributes arrive as dense indices.
#include <optional>
#include <string>
#include <vector>

namespace multra {
struct TripCategory { std::size_t attribute{}; double rate{}; };       // T += N * t
struct TripTerm { std::size_t attribute{}; double coefficient{}; };    // T += a * x
struct TripSide {
    enum class Method { crossClassification, regression } method{Method::crossClassification};
    double constant{};                     // regression only
    std::vector<TripCategory> categories;  // crossClassification only
    std::vector<TripTerm> terms;           // regression only
};
struct TripPurpose { std::string id; TripSide productions, attractions; };

struct TripPurposeResult {
    std::string id;
    std::vector<double> productions, attractionsRaw, attractions; // one per zone, in the given order
    double totalProductions{}, totalAttractionsRaw{}, totalAttractions{};
    std::optional<double> factor; // P / A; absent when both totals are 0
};
// A refusal located by indices; the project layer turns it into a path.
struct TripIssue {
    enum class Code { negative, balanceImpossible } code{};
    std::size_t purpose{};
    std::optional<std::size_t> zone; // negative only
    bool attractions{};              // negative only: which side
};
struct TripGeneration {
    std::vector<TripPurposeResult> purposes; // empty when issues is not
    std::vector<TripIssue> issues;
};
// values[z][a] is zone z's value of attribute a. Sums run in file order (categories, then terms),
// zones in the order given. A negative trip end is refused, never clamped (§3).
TripGeneration generateTrips(const std::vector<std::vector<double>>& values, const std::vector<TripPurpose>& purposes);
// One side of one zone (§3), exposed for the run's own checks and tests.
double tripEnd(const TripSide&, const std::vector<double>& zoneValues);
}
