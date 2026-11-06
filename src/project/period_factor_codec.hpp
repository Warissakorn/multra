#pragma once
// M10.4 E1 (MODE_CHOICE §5-§6, D478): the project's period factors, root `periodFactors` at schema 69:
// per purpose and study period, the shares of the P-A total travelling production to attraction and back
// in that period (Ortuzar and Willumsen 2011, §5.4.4). Owner data; nothing ships (rule 4).
#include "json.hpp"
#include "../core/validate.hpp"
#include <string>
#include <vector>

namespace multra {
struct ProjectDocument;
struct PeriodFactor {
    std::string purposeId, periodId;
    double productionToAttraction{}, attractionToProduction{};
    bool operator==(const PeriodFactor&) const = default;
};
// Absent: none. A present list is a non-empty array of exact {purposeId, periodId, productionToAttraction,
// attractionToProduction}; ids non-blank, shares finite in [0, 1] with a sum <= 1. Faults are
// PERIOD_FACTOR_INVALID at `periodFactors` or `periodFactors[i].<field>`.
std::vector<PeriodFactor> parsePeriodFactors(const Json& file);
void addPeriodFactorsJson(const std::vector<PeriodFactor>&, Json& file); // written only when non-empty
// UNKNOWN_PERIOD at `periodFactors[i].periodId`; DUPLICATE_ID at `periodFactors[i]` for a repeated pair.
// A purpose id is checked by a run, not here, so a rate set stays editable.
std::vector<ValidationIssue> periodFactorIssues(const ProjectDocument&);
}
