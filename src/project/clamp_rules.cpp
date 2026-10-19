#include "clamp_rules.hpp"
#include "../core/validate.hpp"
#include <nlohmann/json.hpp>

namespace trafficsim {
namespace {
// `absent` is the value a missing key means, and the one never written.
struct Key { const char* name; bool ClampRules::*member; bool absent; };
constexpr Key kKeys[] = {{"leaderTravel", &ClampRules::leaderTravel, true}, {"sideEntryAtLine", &ClampRules::sideEntryAtLine, true},
                         {"redLineStop", &ClampRules::redLineStop, true}, {"joinAnticipation", &ClampRules::joinAnticipation, false},
                         {"standingStopFloor", &ClampRules::standingStopFloor, true}};
}
bool ownsClampRules(const ProjectDocument& d) { return d.definition && !(d.definition->clampRules == ClampRules{}); }
bool ownsJoinAnticipation(const ProjectDocument& d) { return d.definition && d.definition->clampRules.joinAnticipation; }
bool ownsStandingStopFloor(const ProjectDocument& d) { return d.definition && !d.definition->clampRules.standingStopFloor; }
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
    return rules;
}
void addClampRulesJson(const AuthoringDefinition& d, Json& definition) {
    Json j = Json::object();
    for (const auto& k : kKeys) if (d.clampRules.*k.member != k.absent) j[k.name] = !k.absent;
    if (!j.empty()) definition["clampRules"] = j;
}
}
