#pragma once
// PRIORITY_RULES (D604, D606): a Vissim priority rule, a waiting line with one or more conflict
// markers and no conflict area. Its structural issues, and its compile to the core's existing
// PriorityRule: one per marker, all at the line's yield point, so the line holds while any blocks.
#include "right_of_way.hpp"

namespace multra {
// Structural issues of one marker-form rule at `path` ("rightOfWay.priorityRules[i]"): the line
// it names, the area form's keys left unset, each marker's path, station and numbers, and no
// marker on the line's own path (it would hold a vehicle for itself). Class ids are checked by
// the project layer, which owns the classes.
void priorityMarkerIssues(const Network&, const AuthoredPriorityRule&, const std::string& path, std::vector<ValidationIssue>&);
// The runtime id of marker `k` of rule `ruleId`, so the project layer can find it again.
std::string priorityMarkerRuleId(const std::string& ruleId, std::size_t k);
// Every marker that locates, as a runtime rule with no type filter (the project layer adds the
// classes); a line or marker that does not locate is a runtime issue, and its rule compiles none.
struct PriorityMarkerResolution {
    std::vector<PriorityRule> rules;
    std::vector<ValidationIssue> issues;
};
PriorityMarkerResolution resolvePriorityMarkers(const Network&, const RuntimeSections&);
}
