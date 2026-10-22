#include "link_type_codec.hpp"
#include "input_manifest.hpp"
#include "../core/validate.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>
#include <set>
#include <stdexcept>

namespace multra {
namespace {
[[noreturn]] void broken() { throw std::invalid_argument("EDIT_CATALOG_READ"); }
std::string text(const Json& j, const char* key) { if (!j.contains(key) || !j.at(key).is_string()) broken(); return j.at(key).get<std::string>(); }
double number(const Json& j, const char* key) { if (!j.contains(key) || !j.at(key).is_number()) broken(); return j.at(key).get<double>(); }
bool blank(const std::string& s) { return s.find_first_not_of(" \t\r\n") == std::string::npos; }
std::vector<std::filesystem::path> files(const std::filesystem::path& directory) {
    std::vector<std::filesystem::path> result;
    if (!std::filesystem::is_directory(directory)) return result;
    for (const auto& entry : std::filesystem::directory_iterator(directory))
        if (entry.is_regular_file() && entry.path().extension() == ".json") result.push_back(entry.path());
    std::sort(result.begin(), result.end());
    return result;
}
}
bool usesLinkTypes(const ProjectDocument& d) {
    if (d.definition && !d.definition->externalLinkTypes) return true;
    return std::any_of(d.network.links.begin(), d.network.links.end(), [](const auto& l) { return l.linkTypeId.has_value(); });
}
void rejectLinkTypesBefore50(const Json& j) {
    const auto refuse = [](const std::string& path) { throw ValidationError({{"EDIT_UNSUPPORTED_FIELD", path}}); };
    if (j.contains("definition") && j.at("definition").is_object() && j.at("definition").contains("linkTypes"))
        refuse("definition.linkTypes");
    if (!j.contains("network") || !j.at("network").is_object() || !j.at("network").contains("links") ||
        !j.at("network").at("links").is_array()) return;
    const auto& links = j.at("network").at("links");
    for (std::size_t i = 0; i < links.size(); ++i)
        if (links[i].is_object() && links[i].contains("linkType")) refuse("network.links[" + std::to_string(i) + "].linkType");
}
bool usesCapacityBasis(const ProjectDocument& d) {
    return d.definition && !d.definition->externalLinkTypes &&
           std::any_of(d.definition->linkTypes.begin(), d.definition->linkTypes.end(), [](const auto& t) { return !t.capacityBasis.empty(); });
}
void rejectCapacityBasisBefore53(const Json& j) {
    if (!j.contains("definition") || !j.at("definition").is_object() || !j.at("definition").contains("linkTypes") ||
        !j.at("definition").at("linkTypes").is_array()) return;
    const auto& types = j.at("definition").at("linkTypes");
    for (std::size_t i = 0; i < types.size(); ++i)
        if (types[i].is_object() && types[i].contains("capacityBasis"))
            throw ValidationError({{"EDIT_UNSUPPORTED_FIELD", "definition.linkTypes[" + std::to_string(i) + "].capacityBasis"}});
}
std::string capacityBasisOf(const std::string& basis) { return basis.empty() ? "practical" : basis; }
void readLinkTypeIds(const Json& network, Network& n) {
    if (!network.contains("links") || !network.at("links").is_array()) return;
    const auto& links = network.at("links");
    for (std::size_t i = 0; i < links.size() && i < n.links.size(); ++i) {
        if (!links[i].contains("linkType")) continue;
        if (!links[i].at("linkType").is_string()) throw ValidationError({{"INVALID_LINK_TYPE", "network.links[" + std::to_string(i) + "].linkType"}});
        n.links[i].linkTypeId = links[i].at("linkType").get<std::string>();
    }
}
LinkType parseLinkType(const Json& j) {
    if (!j.is_object() || j.size() != 6u + j.contains("modes") + j.contains("jamDensity") + j.contains("capacityBasis")) broken();
    LinkType t{text(j, "id"), text(j, "name"), number(j, "capacityPerLane"), number(j, "freeSpeed"), text(j, "vdfId"), {}, text(j, "source"), {}};
    if (j.contains("jamDensity")) { // M11.2: {value, source}
        const auto& k = j.at("jamDensity");
        if (!k.is_object() || k.size() != 2) broken();
        t.jamDensity = JamDensity{number(k, "value"), text(k, "source")};
    }
    if (j.contains("capacityBasis")) t.capacityBasis = text(j, "capacityBasis");
    if (j.contains("modes")) {
        if (!j.at("modes").is_array()) broken();
        for (const auto& m : j.at("modes")) { if (!m.is_string()) broken(); t.modes.push_back(m.get<std::string>()); }
    }
    return t;
}
Json linkTypeJson(const LinkType& t) {
    Json j{{"id", t.id}, {"name", t.name}, {"capacityPerLane", t.capacityPerLane}, {"freeSpeed", t.freeSpeed}, {"vdfId", t.vdfId}, {"source", t.source}};
    if (!t.modes.empty()) j["modes"] = t.modes;
    if (t.jamDensity) j["jamDensity"] = {{"value", t.jamDensity->value}, {"source", t.jamDensity->source}};
    if (!t.capacityBasis.empty()) j["capacityBasis"] = t.capacityBasis;
    return j;
}
Vdf parseVdf(const Json& j) {
    if (!j.is_object() || j.size() != 5u + j.contains("capacityBasis")) broken();
    return {text(j, "id"), text(j, "form"), number(j, "alpha"), number(j, "beta"), text(j, "source"),
            j.contains("capacityBasis") ? text(j, "capacityBasis") : std::string{}};
}
std::vector<ResolvedLinkType> resolveLinkTypes(const AuthoringDefinition& d, const std::filesystem::path& data, InputManifest* manifest) {
    std::vector<ResolvedLinkType> result;
    if (!d.externalLinkTypes) {
        for (std::size_t i = 0; i < d.linkTypes.size(); ++i) result.push_back({d.linkTypes[i], "definition.linkTypes[" + std::to_string(i) + "]"});
        return result;
    }
    try {
        for (const auto& file : files(data / "link-types")) {
            const auto logical = "catalog/link-types/" + file.filename().generic_string();
            result.push_back({parseLinkType(readInputJson(file, logical, manifest)), logical});
        }
    } catch (const std::exception&) { throw std::runtime_error("EDIT_CATALOG_READ"); }
    return result;
}
std::vector<Vdf> resolveVdfs(const std::filesystem::path& data, InputManifest* manifest) {
    std::vector<Vdf> result;
    try {
        for (const auto& file : files(data / "vdf"))
            result.push_back(parseVdf(readInputJson(file, "catalog/vdf/" + file.filename().generic_string(), manifest)));
    } catch (const std::exception&) { throw std::runtime_error("EDIT_CATALOG_READ"); }
    return result;
}
std::vector<ValidationIssue> linkTypeIssues(const std::vector<ResolvedLinkType>& types, const std::vector<std::string>* modeIds) {
    std::vector<ValidationIssue> issues;
    std::set<std::string> ids;
    const auto positive = [](double v) { return std::isfinite(v) && v > 0; };
    for (const auto& [t, path] : types) {
        const auto bad = [&](const std::string& field) { issues.push_back({"INVALID_LINK_TYPE", path + "." + field}); };
        if (blank(t.id) || !ids.insert(t.id).second) bad("id");
        if (!positive(t.capacityPerLane)) bad("capacityPerLane");
        if (!positive(t.freeSpeed)) bad("freeSpeed");
        if (blank(t.vdfId)) bad("vdfId");
        if (blank(t.source)) bad("source");
        // M11.2: above the critical density capacity / speed (pcu/h over km/h), or the wave speed
        // capacity / (jam - critical) is not a finite positive number.
        if (t.jamDensity && !(std::isfinite(t.jamDensity->value) && positive(t.capacityPerLane) && positive(t.freeSpeed) &&
                              t.jamDensity->value > t.capacityPerLane / (t.freeSpeed * 3.6))) bad("jamDensity.value");
        if (t.jamDensity && blank(t.jamDensity->source)) bad("jamDensity.source");
        if (!t.capacityBasis.empty() && t.capacityBasis != "practical" && t.capacityBasis != "ultimate") bad("capacityBasis");
        if (modeIds)
            for (std::size_t k = 0; k < t.modes.size(); ++k)
                if (std::find(modeIds->begin(), modeIds->end(), t.modes[k]) == modeIds->end()) bad("modes[" + std::to_string(k) + "]");
    }
    return issues;
}
std::vector<ValidationIssue> vdfIssues(const std::vector<Vdf>& vdfs) {
    std::vector<ValidationIssue> issues;
    for (const auto& v : vdfs) {
        const auto path = "catalog/vdf/" + v.id;
        if (v.form != "bpr") issues.push_back({"INVALID_VDF", path + ".form"});
        if (!std::isfinite(v.alpha) || v.alpha < 0) issues.push_back({"INVALID_VDF", path + ".alpha"});
        if (!std::isfinite(v.beta) || v.beta < 0) issues.push_back({"INVALID_VDF", path + ".beta"});
        if (!v.capacityBasis.empty() && v.capacityBasis != "practical" && v.capacityBasis != "ultimate")
            issues.push_back({"INVALID_VDF", path + ".capacityBasis"});
    }
    return issues;
}
std::vector<ValidationIssue> linkTypeDocumentIssues(const ProjectDocument& d) {
    std::vector<ValidationIssue> issues;
    for (std::size_t i = 0; i < d.network.links.size(); ++i)
        if (d.network.links[i].linkTypeId && blank(*d.network.links[i].linkTypeId))
            issues.push_back({"INVALID_LINK_TYPE", "network.links[" + std::to_string(i) + "].linkType"});
    if (d.definition && !d.definition->externalLinkTypes) {
        const auto owned = linkTypeIssues(resolveLinkTypes(*d.definition, {}));
        issues.insert(issues.end(), owned.begin(), owned.end());
    }
    return issues;
}
}
