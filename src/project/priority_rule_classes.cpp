#include "priority_rule_classes.hpp"
#include "../model/network/priority_markers.hpp"
#include <algorithm>

namespace multra {
std::vector<ValidationIssue> priorityRuleClassIssues(const ProjectDocument& d) {
    std::vector<ValidationIssue> issues;
    const auto& rules = d.network.rightOfWay.priorityRules;
    const auto known = [&](const std::string& id) {
        return d.definition && std::any_of(d.definition->vehicleClasses.begin(), d.definition->vehicleClasses.end(),
                                           [&](const auto& c) { return c.id == id; });
    };
    const auto check = [&](const std::vector<std::string>& ids, const std::string& path) {
        for (std::size_t k = 0; k < ids.size(); ++k)
            if (!known(ids[k])) issues.push_back({"UNKNOWN_VEHICLE_CLASS", path + ".vehicleClassIds[" + std::to_string(k) + "]"});
    };
    for (std::size_t i = 0; i < rules.size(); ++i) {
        const auto path = "rightOfWay.priorityRules[" + std::to_string(i) + "]";
        check(rules[i].vehicleClassIds, path);
        for (std::size_t k = 0; k < rules[i].markers.size(); ++k)
            check(rules[i].markers[k].vehicleClassIds, path + ".markers[" + std::to_string(k) + "]");
    }
    return issues;
}
void applyPriorityRuleClasses(Scenario& scenario, const Network& network, const AuthoringDefinition& definition) {
    const auto types = [&](const std::vector<std::string>& classIds) -> std::optional<std::vector<std::string>> {
        if (classIds.empty()) return std::nullopt;
        std::vector<std::string> result;
        for (const auto& c : definition.vehicleClasses)
            if (std::find(classIds.begin(), classIds.end(), c.id) != classIds.end())
                result.insert(result.end(), c.vehicleTypeIds.begin(), c.vehicleTypeIds.end());
        return result;
    };
    for (const auto& r : network.rightOfWay.priorityRules)
        for (std::size_t k = 0; k < r.markers.size(); ++k) {
            const auto id = priorityMarkerRuleId(r.id, k);
            const auto rule = std::find_if(scenario.priorityRules.begin(), scenario.priorityRules.end(), [&](const auto& x) { return x.id == id; });
            if (rule == scenario.priorityRules.end()) continue; // did not locate: a runtime issue already
            rule->yieldTypeIds = types(r.vehicleClassIds); rule->blockTypeIds = types(r.markers[k].vehicleClassIds);
        }
}
}
