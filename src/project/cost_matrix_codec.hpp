#pragma once
// M10.3 D2 (TRIP_DISTRIBUTION §5-§6, D474): a project cost matrix that overrides the free-flow skim's
// cells (choice 1a) and is the only source of diagonal costs. Root `costMatrices` at schema 67.
#include "json.hpp"
#include "../core/validate.hpp"
#include <optional>
#include <string>
#include <vector>

namespace multra {
struct CostMatrix {
    std::string id, name, citation;            // citation may be blank
    std::string unit{"min"};
    std::vector<std::string> zoneIds;           // the rows' and columns' order
    std::vector<std::vector<std::optional<double>>> rows; // n x n; empty: no override for that pair
    bool operator==(const CostMatrix&) const = default;
};
// Every shape fault at once, as INVALID_COST_MATRIX / INVALID_ID / DUPLICATE_ID at `path` + its JSON
// path: unit "min"; non-blank unique zone ids; n rows of n cells, each a finite number >= 0 or null.
// Zone ids are not checked against the project here: a run refuses one the project lacks.
CostMatrix parseCostMatrix(const Json&, const std::string& path);
Json costMatrixJson(const CostMatrix&);
std::vector<CostMatrix> parseOwnedCostMatrices(const Json& file);
void addOwnedCostMatricesJson(const std::vector<CostMatrix>&, Json& file);
std::vector<ValidationIssue> ownedCostMatrixIssues(const std::vector<CostMatrix>&); // blank and duplicate ids
}
