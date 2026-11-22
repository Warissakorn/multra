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
// D618: the area form's gap reduction against its gapTime (INVALID_PRIORITY_RULE at the key's path).
void areaRuleGapIssues(const AuthoredPriorityRule&, const std::string& path, std::vector<ValidationIssue>&);
void priorityMarkerIssues(const Network&, const AuthoredPriorityRule&, const std::string& path, std::vector<ValidationIssue>&);
// D615: whether a marker-form rule waits at this line, so a Stop control there needs no area.
bool lineServesPriorityMarkers(const RightOfWay&, const std::string& lineId);
// The runtime id of marker `k` of rule `ruleId`, so the project layer can find it again.
std::string priorityMarkerRuleId(const std::string& ruleId, std::size_t k);
// Every marker that locates, as a runtime rule with no type filter (the project layer adds the
// classes); a line or marker that does not locate is a runtime issue, and its rule compiles none.
struct PriorityMarkerResolution {
    std::vector<PriorityRule> rules;
    std::vector<ValidationIssue> issues;
};
PriorityMarkerResolution resolvePriorityMarkers(const Network&, const RuntimeSections&);
// D617 (§3): an advisory for each marker whose Link or Connector no route lists
// (PRIORITY_MARKER_UNREACHED at rightOfWay.priorityRules[i].markers[k]); it never blocks Run.
std::vector<ValidationIssue> unreachedPriorityMarkers(const Network&, const std::vector<Route>& routes);
}
