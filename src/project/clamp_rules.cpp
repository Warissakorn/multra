#include "clamp_rules.hpp"
#include "../core/validate.hpp"
#include <nlohmann/json.hpp>

namespace multra {
namespace {
// `absent` is the value a missing key means, and the one never written.
struct Key { const char* name; bool ClampRules::*member; bool absent; };
constexpr Key kKeys[] = {{"leaderTravel", &ClampRules::leaderTravel, true}, {"sideEntryAtLine", &ClampRules::sideEntryAtLine, true},
                         {"redLineStop", &ClampRules::redLineStop, true}, {"joinAnticipation", &ClampRules::joinAnticipation, false},
                         {"standingStopFloor", &ClampRules::standingStopFloor, true}, {"lineRest", &ClampRules::lineRest, true}};
}
bool ownsClampRules(const ProjectDocument& d) {
    if (!d.definition) return false;
    auto rules = d.definition->clampRules; rules.lineRest = true; // D677: never written
    return !(rules == ClampRules{});
}
bool ownsJoinAnticipation(const ProjectDocument& d) { return d.definition && d.definition->clampRules.joinAnticipation; }
bool ownsStandingStopFloor(const ProjectDocument& d) { return d.definition && !d.definition->clampRules.standingStopFloor; }
bool fileTurnsLineRestOff(const Json& j) {
    if (!j.is_object() || !j.contains("definition") || !j.at("definition").is_object()) return false;
    const auto& d = j.at("definition");
    return d.contains("clampRules") && d.at("clampRules").is_object() && d.at("clampRules").contains("lineRest") &&
           d.at("clampRules").at("lineRest").is_boolean() && !d.at("clampRules").at("lineRest").get<bool>();
}
void rejectClampRulesBefore45(const Json& j) {
    if (j.contains("definition") && j.at("definition").is_object() && j.at("definition").contains("clampRules"))
        throw ValidationError({{"EDIT_UNSUPPORTED_FIELD", "definition.clampRules"}});
}
void rejectJoinAnticipationBefore46(const Json& j) {
    if (j.contains("definition") && j.at("definition").is_object() && j.at("definition").contains("clampRules") &&
        j.at("definition").at("clampRules").is_object() && j.at("definition").at("clampRules").contains("joinAnticipation"))
        throw ValidationError({{"EDIT_UNSUPPORTED_FIELD", "definition.clampRules.joinAnticipation"}});
}
void rejectStandingStopFloorBefore47(const Json& j) {
    if (j.contains("definition") && j.at("definition").is_object() && j.at("definition").contains("clampRules") &&
        j.at("definition").at("clampRules").is_object() && j.at("definition").at("clampRules").contains("standingStopFloor"))
        throw ValidationError({{"EDIT_UNSUPPORTED_FIELD", "definition.clampRules.standingStopFloor"}});
}
void rejectLineRestBefore87(const Json& j) {
    if (j.contains("definition") && j.at("definition").is_object() && j.at("definition").contains("clampRules") &&
        j.at("definition").at("clampRules").is_object() && j.at("definition").at("clampRules").contains("lineRest"))
        throw ValidationError({{"EDIT_UNSUPPORTED_FIELD", "definition.clampRules.lineRest"}});
}
ClampRules parseClampRules(const Json& definition) {
    ClampRules rules;
    if (!definition.contains("clampRules")) return rules;
    const auto& j = definition.at("clampRules");
    if (!j.is_object()) throw ValidationError({{"INVALID_CLAMP_RULE", "definition.clampRules"}});
    for (const auto& [name, value] : j.items()) {
        const Key* key = nullptr;
        for (const auto& k : kKeys) if (name == k.name) key = &k;
        if (!key || !value.is_boolean()) throw ValidationError({{"INVALID_CLAMP_RULE", "definition.clampRules." + name}});
        rules.*key->member = value.get<bool>();
    }
    rules.lineRest = true; // D677: read for its type only; a file written with it off opens with it on (fileTurnsLineRestOff)
    return rules;
}
void addClampRulesJson(const AuthoringDefinition& d, Json& definition) {
    Json j = Json::object();
    for (const auto& k : kKeys) if (k.member != &ClampRules::lineRest && d.clampRules.*k.member != k.absent) j[k.name] = !k.absent; // D677
    if (!j.empty()) definition["clampRules"] = j;
}
}
