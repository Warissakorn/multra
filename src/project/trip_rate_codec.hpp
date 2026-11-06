#pragma once
// M10.2 G1 (TRIP_GENERATION §6.1, D470): a trip-rate set, its JSON form and the catalog in
// `data/trip-rates/`. No set ships (choice 8a); project-owned sets are G2.
#include "json.hpp"
#include "../macro/trip_generation.hpp"
#include "../core/validate.hpp"
#include <filesystem>
#include <string>
#include <vector>

namespace multra {
class InputManifest;
struct TripRateCategory { std::string attribute; double rate{}; bool operator==(const TripRateCategory&) const = default; };
struct TripRateTerm { std::string attribute; double coefficient{}; bool operator==(const TripRateTerm&) const = default; };
struct TripRateSide {
    TripSide::Method method{TripSide::Method::crossClassification};
    double constant{};
    std::vector<TripRateCategory> categories;
    std::vector<TripRateTerm> terms;
    bool operator==(const TripRateSide&) const = default;
};
struct TripRatePurpose { std::string id; TripRateSide productions, attractions; bool operator==(const TripRatePurpose&) const = default; };
struct TripRateSet {
    std::string id, name, citation, table, unit; // `table` may be blank
    std::vector<TripRatePurpose> purposes;
    bool operator==(const TripRateSet&) const = default;
};
struct ResolvedTripRateSet { TripRateSet set; std::string path; }; // path: the issue prefix, e.g. catalog/trip-rates/x.json

// Every shape fault of §6.1 at once, as INVALID_TRIP_RATE_SET / INVALID_ID at `path` + its JSON path.
// `requireCitation` is true for a catalog set.
TripRateSet parseTripRateSet(const Json&, const std::string& path, bool requireCitation);
Json tripRateSetJson(const TripRateSet&);
// M10.2 G2 (§6.3, D471): the project's own sets, root `tripRateSets` at schema 66. A present list must
// be a non-empty array (INVALID_TRIP_RATE_SET at `tripRateSets`); rows are parsed at `tripRateSets[i]`
// with the citation optional; ids must be unique (DUPLICATE_ID). Absent or empty lists are not written.
std::vector<TripRateSet> parseOwnedTripRateSets(const Json& file);
void addOwnedTripRateSetsJson(const std::vector<TripRateSet>&, Json& file);
std::vector<ValidationIssue> ownedTripRateSetIssues(const std::vector<TripRateSet>&); // blank and duplicate ids
// `data/trip-rates/*.json` in file-name order; a missing folder is an empty catalog. Duplicate ids are
// DUPLICATE_ID at the later file's `.id`.
std::vector<ResolvedTripRateSet> resolveTripRateSets(const std::filesystem::path& dataDirectory, InputManifest* = nullptr);
}
