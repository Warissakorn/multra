#pragma once
// PRIORITY_RULES (D606): a priority rule's vehicle classes. The classes are authoring data the
// definition owns, so the network layer compiles a rule's markers without them and these add them.
#include "document.hpp"

namespace multra {
// Each class id a marker-form rule or marker names must be one of definition.vehicleClasses
// (UNKNOWN_VEHICLE_CLASS at its path); a file without classes can name none.
std::vector<ValidationIssue> priorityRuleClassIssues(const ProjectDocument&);
// The compiled marker rules' type filters: the line's classes as the types that wait, each
// marker's as the types that block, both left absent (every type) when the list is empty.
void applyPriorityRuleClasses(Scenario&, const Network&, const AuthoringDefinition&);
}
