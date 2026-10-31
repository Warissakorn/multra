#include "link_speed_codec.hpp"
#include "network_fields.hpp"
#include "../core/validate.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>

namespace multra {
namespace {
using networkCodec::linkSpeedVersion;
const Json* array(const Json& object, const char* key) {
    return object.is_object() && object.contains(key) && object.at(key).is_array() ? &object.at(key) : nullptr;
}
bool speed(const Json& v) { return v.is_number() && std::isfinite(v.get<double>()) && v.get<double>() > 0; }
}
bool usesLinkSpeedLimits(const ProjectDocument& d) {
    const auto& links = d.network.links;
    if (std::any_of(links.begin(), links.end(), [](const auto& l) { return l.speedLimit.has_value(); })) return true;
    return d.definition && !d.definition->externalCrossSections &&
           std::any_of(d.definition->crossSections.begin(), d.definition->crossSections.end(), [](const auto& t) { return t.speedLimit.has_value(); });
}
void rejectLinkSpeedFields(const Json& file, int version) {
    const auto refuse = [](const std::string& path) { throw ValidationError({{"EDIT_UNSUPPORTED_FIELD", path}}); };
    if (!file.is_object()) return;
    if (version < linkSpeedVersion) {
        if (file.contains("network"))
            if (const auto* links = array(file.at("network"), "links"))
                for (std::size_t i = 0; i < links->size(); ++i)
                    if ((*links)[i].is_object() && (*links)[i].contains("speedLimit")) refuse("network.links[" + std::to_string(i) + "].speedLimit");
        if (file.contains("definition"))
            if (const auto* sections = array(file.at("definition"), "crossSections"))
                for (std::size_t i = 0; i < sections->size(); ++i)
                    if ((*sections)[i].is_object() && (*sections)[i].contains("speedLimit"))
                        refuse("definition.crossSections[" + std::to_string(i) + "].speedLimit");
        return;
    }
    if (!file.contains("network")) return;
    const auto* nodes = array(file.at("network"), "nodes");
    for (std::size_t i = 0; nodes && i < nodes->size(); ++i) {
        const auto& node = (*nodes)[i];
        if (!node.is_object() || !node.contains("analysis")) continue;
        const auto* rows = array(node.at("analysis"), "approaches");
        for (std::size_t k = 0; rows && k < rows->size(); ++k)
            if ((*rows)[k].is_object() && (*rows)[k].contains("speedLimit"))
                refuse("network.nodes[" + std::to_string(i) + "].analysis.approaches[" + std::to_string(k) + "].speedLimit");
    }
}
void readLinkSpeedLimits(const Json& network, Network& n) {
    const auto* links = array(network, "links");
    for (std::size_t i = 0; links && i < links->size() && i < n.links.size(); ++i) {
        const auto& item = (*links)[i];
        if (!item.is_object() || !item.contains("speedLimit")) continue;
        if (!speed(item.at("speedLimit"))) throw ValidationError({{"INVALID_LINK_SPEED", "network.links[" + std::to_string(i) + "].speedLimit"}});
        n.links[i].speedLimit = item.at("speedLimit").get<double>();
    }
}
void carryApproachSpeeds(const Json& network, Network& n, int version) {
    if (version >= linkSpeedVersion) return;
    const auto* nodes = array(network, "nodes");
    for (std::size_t i = 0; nodes && i < nodes->size(); ++i) {
        const auto& node = (*nodes)[i];
        if (!node.is_object() || !node.contains("analysis")) continue;
        const auto* rows = array(node.at("analysis"), "approaches");
        for (std::size_t k = 0; rows && k < rows->size(); ++k) {
            const auto& row = (*rows)[k];
            if (!row.is_object() || !row.contains("speedLimit")) continue;
            const auto at = "network.nodes[" + std::to_string(i) + "].analysis.approaches[" + std::to_string(k) + "].speedLimit";
            const auto value = row.at("speedLimit").get<double>(); // checked by parseNodes
            const auto& id = row.at("link").get_ref<const std::string&>();
            const auto link = std::find_if(n.links.begin(), n.links.end(), [&](const auto& l) { return l.id == id; });
            if (link == n.links.end() || (link->speedLimit && *link->speedLimit != value)) throw ValidationError({{"EDIT_NODE_INVALID", at}});
            link->speedLimit = value;
        }
    }
}
}
