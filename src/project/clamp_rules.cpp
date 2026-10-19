#include "clamp_rules.hpp"
#include "../core/validate.hpp"
#include <nlohmann/json.hpp>

namespace trafficsim {
namespace {
struct Key { const char* name; bool ClampRules::*member; };
constexpr Key kKeys[] = {{"leaderTravel", &ClampRules::leaderTravel}, {"sideEntryAtLine", &ClampRules::sideEntryAtLine},
                         {"redLineStop", &ClampRules::redLineStop}};
}
bool ownsClampRules(const ProjectDocument& d) { return d.definition && !(d.definition->clampRules == ClampRules{}); }
void rejectClampRulesBefore45(const Json& j) {
    if (j.contains("definition") && j.at("definition").is_object() && j.at("definition").contains("clampRules"))
        throw ValidationError({{"EDIT_UNSUPPORTED_FIELD", "definition.clampRules"}});
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
    for (const auto& k : kKeys) if (!(d.clampRules.*k.member)) j[k.name] = false;
    if (!j.empty()) definition["clampRules"] = j;
}
}
