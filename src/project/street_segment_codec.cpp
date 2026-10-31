#include "street_segment_codec.hpp"
#include "input_manifest.hpp"
#include "network_fields.hpp"
#include "zone_codec.hpp"
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
int whole(const Json& j, const char* key) { if (!j.contains(key) || !j.at(key).is_number_integer()) broken(); return j.at(key).get<int>(); }
bool blank(const std::string& s) { return s.find_first_not_of(" \t\r\n") == std::string::npos; }
std::vector<std::filesystem::path> files(const std::filesystem::path& directory) {
    std::vector<std::filesystem::path> result;
    if (!std::filesystem::is_directory(directory)) return result;
    for (const auto& entry : std::filesystem::directory_iterator(directory))
        if (entry.is_regular_file() && entry.path().extension() == ".json") result.push_back(entry.path());
    std::sort(result.begin(), result.end());
    return result;
}
[[noreturn]] void invalid(const std::string& path) { throw ValidationError({{"EDIT_STREET_SEGMENT_INVALID", path}}); }
}
bool usesStreetSegmentKeys(const ProjectDocument& d) {
    return !d.network.streetSegments.empty() || usesStreetSegmentZoneConnectors(d.zones) ||
           std::any_of(d.network.links.begin(), d.network.links.end(), [](const auto& l) { return l.streetSegmentId.has_value(); });
}
bool usesStreetSegmentFields(const ProjectDocument& d) {
    if (!d.network.streetSegments.empty() || !d.network.overpasses.empty()) return true;
    if (std::any_of(d.network.nodes.begin(), d.network.nodes.end(), [](const auto& n) { return n.coarse.has_value(); })) return true; // S3.6a
    if (d.definition && !d.definition->externalCrossSections) return true;
    return std::any_of(d.network.links.begin(), d.network.links.end(), [](const auto& l) { return l.streetSegmentId.has_value(); });
}
void rejectStreetSegmentFieldsBefore53(const Json& j) {
    const auto refuse = [](const std::string& path) { throw ValidationError({{"EDIT_UNSUPPORTED_FIELD", path}}); };
    if (j.contains("definition") && j.at("definition").is_object() && j.at("definition").contains("crossSections"))
        refuse("definition.crossSections");
    if (!j.contains("network") || !j.at("network").is_object()) return;
    const auto& n = j.at("network");
    for (const auto* key : {"roads", "streetSegments"}) if (n.contains(key)) refuse(std::string("network.") + key); // either name (D389)
    if (n.contains("overpasses")) refuse("network.overpasses");
    if (n.contains("nodes") && n.at("nodes").is_array())
        for (std::size_t i = 0; i < n.at("nodes").size(); ++i)
            if (n.at("nodes")[i].is_object() && n.at("nodes")[i].contains("coarse")) refuse("network.nodes[" + std::to_string(i) + "].coarse");
    if (!n.contains("links") || !n.at("links").is_array()) return;
    for (std::size_t i = 0; i < n.at("links").size(); ++i)
        for (const auto* key : {"roadId", "streetSegmentId"})
            if (n.at("links")[i].is_object() && n.at("links")[i].contains(key)) refuse("network.links[" + std::to_string(i) + "]." + key);
}
void rejectOtherStreetSegmentKeys(const Json& j, int version) {
    if (version < 53 || !j.contains("network") || !j.at("network").is_object()) return;
    const auto refuse = [](const std::string& path) { throw ValidationError({{"EDIT_UNSUPPORTED_FIELD", path}}); };
    const bool renamed = version >= networkCodec::streetSegmentKeyVersion;
    const auto& n = j.at("network");
    if (n.contains(renamed ? "roads" : "streetSegments")) refuse(std::string("network.") + (renamed ? "roads" : "streetSegments"));
    if (!n.contains("links") || !n.at("links").is_array()) return;
    const char* other = renamed ? "roadId" : "streetSegmentId";
    for (std::size_t i = 0; i < n.at("links").size(); ++i)
        if (n.at("links")[i].is_object() && n.at("links")[i].contains(other)) refuse("network.links[" + std::to_string(i) + "]." + other);
}
void addStreetSegmentsJson(const Network& n, Json& network) {
    if (!n.overpasses.empty()) {
        Json pairs = Json::array();
        for (const auto& [a, b] : n.overpasses) pairs.push_back(Json::array({a, b}));
        network["overpasses"] = pairs;
    }
    if (n.streetSegments.empty()) return;
    Json roads = Json::array();
    for (const auto& r : n.streetSegments) {
        Json centreline = Json::array();
        for (const auto& p : r.centreline) centreline.push_back({{"x", p.x}, {"y", p.y}});
        Json road{{"id", r.id}, {"name", r.name}, {"centreline", centreline}, {"level", r.level}, {"links", r.links}};
        if (r.crossSectionId) road["crossSectionId"] = *r.crossSectionId;
        Json overrides = Json::object();
        if (r.overrides.lanesForward) overrides["lanesForward"] = *r.overrides.lanesForward;
        if (r.overrides.lanesBackward) overrides["lanesBackward"] = *r.overrides.lanesBackward;
        if (r.overrides.laneWidth) overrides["laneWidth"] = *r.overrides.laneWidth;
        if (r.overrides.median) overrides["median"] = *r.overrides.median;
        if (!overrides.empty()) road["overrides"] = overrides;
        roads.push_back(std::move(road));
    }
    network["streetSegments"] = roads;
}
std::vector<std::pair<std::string, std::string>> parseOverpasses(const Json& network, int schema) {
    std::vector<std::pair<std::string, std::string>> pairs;
    if (schema < 53 || !network.is_object() || !network.contains("overpasses")) return pairs;
    const auto& values = network.at("overpasses");
    if (!values.is_array() || values.empty()) invalid("network.overpasses"); // never written empty
    for (std::size_t i = 0; i < values.size(); ++i) {
        const auto& v = values[i];
        if (!v.is_array() || v.size() != 2 || !v[0].is_string() || !v[1].is_string()) invalid("network.overpasses[" + std::to_string(i) + "]");
        pairs.emplace_back(v[0].get<std::string>(), v[1].get<std::string>());
    }
    return pairs;
}
std::vector<StreetSegment> parseStreetSegments(const Json& network, int schema) {
    std::vector<StreetSegment> roads;
    const std::string key = networkCodec::streetSegmentsKey(schema);
    if (schema < 53 || !network.is_object() || !network.contains(key)) return roads;
    const auto& values = network.at(key);
    if (!values.is_array() || values.empty()) invalid("network." + key); // never written empty
    for (std::size_t i = 0; i < values.size(); ++i) {
        const auto& j = values[i];
        const auto path = "network." + key + "[" + std::to_string(i) + "]";
        if (!j.is_object()) invalid(path);
        for (const auto& [key, value] : j.items()) {
            (void)value;
            if (key != "id" && key != "name" && key != "centreline" && key != "level" && key != "links" && key != "crossSectionId" && key != "overrides")
                invalid(path + "." + key);
        }
        StreetSegment road;
        if (!j.contains("id") || !j.at("id").is_string()) invalid(path + ".id");
        if (!j.contains("name") || !j.at("name").is_string()) invalid(path + ".name");
        if (!j.contains("centreline") || !j.at("centreline").is_array()) invalid(path + ".centreline");
        if (!j.contains("level") || !j.at("level").is_number_integer()) invalid(path + ".level");
        if (!j.contains("links") || !j.at("links").is_array()) invalid(path + ".links");
        road.id = j.at("id").get<std::string>(); road.name = j.at("name").get<std::string>(); road.level = j.at("level").get<int>();
        for (std::size_t k = 0; k < j.at("centreline").size(); ++k) {
            const auto& p = j.at("centreline")[k];
            if (!p.is_object() || p.size() != 2 || !p.contains("x") || !p.contains("y") || !p.at("x").is_number() || !p.at("y").is_number())
                invalid(path + ".centreline[" + std::to_string(k) + "]");
            road.centreline.push_back({p.at("x").get<double>(), p.at("y").get<double>()});
        }
        for (std::size_t k = 0; k < j.at("links").size(); ++k) {
            if (!j.at("links")[k].is_string()) invalid(path + ".links[" + std::to_string(k) + "]");
            road.links.push_back(j.at("links")[k].get<std::string>());
        }
        if (j.contains("crossSectionId")) {
            if (!j.at("crossSectionId").is_string()) invalid(path + ".crossSectionId");
            road.crossSectionId = j.at("crossSectionId").get<std::string>();
        }
        if (j.contains("overrides")) {
            const auto& o = j.at("overrides");
            if (!o.is_object() || o.empty()) invalid(path + ".overrides"); // never written empty
            for (const auto& [key, value] : o.items()) {
                const auto at = path + ".overrides." + key;
                if (key == "lanesForward" || key == "lanesBackward") {
                    if (!value.is_number_integer()) invalid(at);
                    (key == "lanesForward" ? road.overrides.lanesForward : road.overrides.lanesBackward) = value.get<int>();
                } else if (key == "laneWidth" || key == "median") {
                    if (!value.is_number()) invalid(at);
                    (key == "laneWidth" ? road.overrides.laneWidth : road.overrides.median) = value.get<double>();
                } else invalid(at);
            }
        }
        roads.push_back(std::move(road));
    }
    return roads;
}
void readStreetSegmentIds(const Json& network, Network& n, int schema) {
    const std::string key = networkCodec::streetSegmentIdKey(schema);
    if (!network.contains("links") || !network.at("links").is_array()) return;
    const auto& links = network.at("links");
    for (std::size_t i = 0; i < links.size() && i < n.links.size(); ++i) {
        if (!links[i].contains(key)) continue;
        if (!links[i].at(key).is_string()) invalid("network.links[" + std::to_string(i) + "]." + key);
        n.links[i].streetSegmentId = links[i].at(key).get<std::string>();
    }
}
CrossSection parseCrossSection(const Json& j) {
    if (!j.is_object() || j.size() != 8u + j.contains("laneWidths") + j.contains("linkType") + j.contains("speedLimit")) broken();
    CrossSection t{text(j, "id"), text(j, "name"), whole(j, "directions"), whole(j, "lanesForward"), whole(j, "lanesBackward"),
                   number(j, "laneWidth"), {}, number(j, "median"), {}, text(j, "source")};
    if (j.contains("laneWidths")) {
        if (!j.at("laneWidths").is_array()) broken();
        for (const auto& w : j.at("laneWidths")) { if (!w.is_number()) broken(); t.laneWidths.push_back(w.get<double>()); }
    }
    if (j.contains("linkType")) t.linkType = text(j, "linkType");
    if (j.contains("speedLimit")) t.speedLimit = number(j, "speedLimit"); // J1 (D390), schema 60 when owned
    return t;
}
Json crossSectionJson(const CrossSection& t) {
    Json j{{"id", t.id}, {"name", t.name}, {"directions", t.directions}, {"lanesForward", t.lanesForward}, {"lanesBackward", t.lanesBackward},
           {"laneWidth", t.laneWidth}, {"median", t.median}, {"source", t.source}};
    if (!t.laneWidths.empty()) j["laneWidths"] = t.laneWidths;
    if (t.linkType) j["linkType"] = *t.linkType;
    if (t.speedLimit) j["speedLimit"] = *t.speedLimit;
    return j;
}
std::vector<CrossSection> linkLessStreetSegmentTemplates(const ProjectDocument& d, const std::filesystem::path& data, InputManifest* manifest) {
    std::vector<CrossSection> result;
    if (std::none_of(d.network.streetSegments.begin(), d.network.streetSegments.end(), [](const auto& r) { return r.links.empty(); })) return result;
    for (auto& r : resolveCrossSections(d.definition.value_or(AuthoringDefinition{}), data, manifest)) result.push_back(std::move(r.section));
    return result;
}
std::vector<ResolvedCrossSection> resolveCrossSections(const AuthoringDefinition& d, const std::filesystem::path& data, InputManifest* manifest) {
    std::vector<ResolvedCrossSection> result;
    if (!d.externalCrossSections) {
        for (std::size_t i = 0; i < d.crossSections.size(); ++i) result.push_back({d.crossSections[i], "definition.crossSections[" + std::to_string(i) + "]"});
        return result;
    }
    try {
        for (const auto& file : files(data / "cross-sections")) {
            const auto logical = "catalog/cross-sections/" + file.filename().generic_string();
            result.push_back({parseCrossSection(readInputJson(file, logical, manifest)), logical});
        }
    } catch (const std::exception&) { throw std::runtime_error("EDIT_CATALOG_READ"); }
    return result;
}
std::vector<ValidationIssue> crossSectionIssues(const std::vector<ResolvedCrossSection>& sections, const std::vector<std::string>* linkTypeIds) {
    std::vector<ValidationIssue> issues;
    std::set<std::string> ids;
    const auto positive = [](double v) { return std::isfinite(v) && v > 0; };
    for (const auto& [t, path] : sections) {
        const auto bad = [&](const std::string& field) { issues.push_back({"INVALID_CROSS_SECTION", path + "." + field}); };
        if (blank(t.id) || !ids.insert(t.id).second) bad("id");
        if (t.directions != 1 && t.directions != 2) bad("directions");
        if (t.lanesForward < 1 || t.lanesForward > 12) bad("lanesForward");
        if (t.directions == 2 ? (t.lanesBackward < 1 || t.lanesBackward > 12) : t.lanesBackward != 0) bad("lanesBackward");
        if (!positive(t.laneWidth)) bad("laneWidth");
        const auto lanes = static_cast<std::size_t>(std::max(0, t.lanesForward) + (t.directions == 2 ? std::max(0, t.lanesBackward) : 0));
        if (!t.laneWidths.empty() && (t.laneWidths.size() != lanes || !std::all_of(t.laneWidths.begin(), t.laneWidths.end(), positive))) bad("laneWidths");
        if (!std::isfinite(t.median) || t.median < 0) bad("median");
        if (blank(t.source)) bad("source");
        if (t.speedLimit && !positive(*t.speedLimit)) bad("speedLimit");
        if (t.linkType && (blank(*t.linkType) || (linkTypeIds && std::find(linkTypeIds->begin(), linkTypeIds->end(), *t.linkType) == linkTypeIds->end())))
            bad("linkType");
    }
    return issues;
}
std::vector<ValidationIssue> streetSegmentDocumentIssues(const ProjectDocument& d) {
    std::vector<ValidationIssue> issues;
    if (!d.definition || d.definition->externalCrossSections) return issues;
    const auto owned = resolveCrossSections(*d.definition, {});
    issues = crossSectionIssues(owned);
    for (std::size_t i = 0; i < d.network.streetSegments.size(); ++i) {
        const auto& r = d.network.streetSegments[i];
        if (r.crossSectionId && std::none_of(owned.begin(), owned.end(), [&](const auto& s) { return s.section.id == *r.crossSectionId; }))
            issues.push_back({"STREET_SEGMENT_CROSS_SECTION_UNKNOWN", "network.streetSegments[" + std::to_string(i) + "].crossSectionId"});
    }
    return issues;
}
}
