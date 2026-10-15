#pragma once
#include "types.hpp"
#include <string>
#include <vector>

namespace trafficsim {
// M3.5 perception keys (docs/reference/PERCEPTION.md §6, D179/D181): the one list the codec, the
// validator and the serializer read, so a key name exists once (hard rule 3).
struct PerceptionKey {
    const char* name;
    std::optional<double> PerceptionParameters::* member;
    bool integer; // written and read as a JSON integer
};
const std::vector<PerceptionKey>& perceptionParameterKeys();
// §6 ranges, finiteness and integrality of present keys: INVALID_BEHAVIOUR_PARAMETER at
// `<path>.<key>`. Absent keys are always valid.
std::vector<ValidationIssue> perceptionParameterIssues(const PerceptionParameters& parameters, const std::string& path);
bool hasPerception(const PerceptionParameters& parameters);
}
