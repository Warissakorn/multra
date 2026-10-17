#pragma once
// B-6 §7 (docs/reference/SPEED_DECISIONS.md, D225): reduced-speed areas in the project file and
// their compile into the engine's per-lane-section form.
#include "document.hpp"
namespace trafficsim {
void parseSpeedAreas(const Json& definition, int schema, AuthoringDefinition&);
void addSpeedAreasJson(const AuthoringDefinition&, Json& definition);
// §7.1's faults at their paths; vehicle types only when the document owns them (else on Run).
std::vector<ValidationIssue> speedAreaSpecIssues(const Network&, const AuthoringDefinition&);
std::vector<ReducedSpeedArea> compileSpeedAreas(const Network&, const AuthoringDefinition&);
// The lane ids an area covers on `link`: its own list, or every lane.
std::vector<std::string> coveredLanes(const ReducedSpeedAreaSpec&, const Link& link);
}
