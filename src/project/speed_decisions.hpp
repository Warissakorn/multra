#pragma once
// B-6 (docs/reference/SPEED_DECISIONS.md, D223): desired-speed decisions in the project file and
// their compile into the engine's per-lane form.
#include "document.hpp"
namespace trafficsim {
void parseSpeedDecisions(const Json& definition, int schema, AuthoringDefinition&);
void addSpeedDecisionsJson(const AuthoringDefinition&, Json& definition);
// §2's faults at their paths; vehicle types only when the document owns them (else on Run).
std::vector<ValidationIssue> speedDecisionSpecIssues(const Network&, const AuthoringDefinition&);
std::vector<SpeedDecision> compileSpeedDecisions(const Network&, const AuthoringDefinition&);
}
