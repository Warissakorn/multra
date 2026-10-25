// S3.2 rows RD1 (capacity-basis part), RD2 and RD3 (docs/reference/ROADS.md §8, §13, D304): the HCM
// link-type catalog, the capacity basis on types and VDFs, and Macro's basis check. Each row first
// checks its forcing.
#include "test.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/project/link_type_codec.hpp"
#include "../src/project/macro_input.hpp"
#include "../src/project/meso_input.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>
using namespace multra;
namespace {
std::filesystem::path data() { return test::root() / "data"; }
bool issueAt(const std::vector<ValidationIssue>& issues, const std::string& code, const std::string& path) {
    return std::any_of(issues.begin(), issues.end(), [&](const auto& i) { return i.code == code && i.path == path; });
}
bool refusedAt(const Json& j, const std::string& code, const std::string& path) {
    try { parseDocument(j); } catch (const ValidationError& e) {
        return std::any_of(e.issues.begin(), e.issues.end(), [&](const auto& i) { return i.code == code && i.path == path; });
    } catch (...) {}
    return false;
}
// Two Links joined by a Connector, each typed `type`, on the installed or an owned catalog.
ProjectDocument chain(const std::string& type) {
    ProjectDocument d;
    const auto a = addLink(d, {{0, 0}, {500, 0}}, 2, 3.6), b = addLink(d, {{520, 0}, {1000, 0}}, 2, 3.6);
    addConnector(d, {a, d.network.links[0].lanes[0].id, 500}, {b, d.network.links[1].lanes[0].id, 0});
    demand(d);
    for (auto& l : d.network.links) l.linkTypeId = type;
    return d;
}
}
TEST(roadcatalog, rd1_a_capacity_basis_is_schema_53) { // RD1 (capacity basis)
    auto d = chain("local");
    auto& def = demand(d); def.externalLinkTypes = false;
    def.linkTypes = {{"local", "Local", 600, 10, "bpr", {}, "Test values"}};
    CHECK(documentJson(d)["schemaVersion"] == 50); // forcing: an owned type without a basis stays at 50
    def.linkTypes[0].capacityBasis = "ultimate";
    const auto j = documentJson(d);
    CHECK(j["schemaVersion"] == 53 && j["definition"]["linkTypes"][0]["capacityBasis"] == "ultimate");
    CHECK(parseDocument(Json::parse(j.dump())).definition->linkTypes[0].capacityBasis == "ultimate");
    auto old = j; old["schemaVersion"] = 52;
    CHECK(refusedAt(old, "EDIT_UNSUPPORTED_FIELD", "definition.linkTypes[0].capacityBasis"));
    // Every shipped project keeps its schema: none has a basis.
    for (const auto& entry : std::filesystem::directory_iterator(data() / "projects")) {
        if (!entry.path().string().ends_with(".traffic.json")) continue; // the folder's README
        std::ifstream file(entry.path()); Json p; file >> p;
        CHECK(documentJson(parseDocument(p))["schemaVersion"] == p["schemaVersion"]);
    }
}
TEST(roadcatalog, rd2_the_hcm_catalog) { // RD2
    const auto types = resolveLinkTypes(AuthoringDefinition{}, data());
    CHECK(types.size() == 11); // forcing: the installed catalog is read
    CHECK(linkTypeIssues(types).empty() && vdfIssues(resolveVdfs(data())).empty());
    // Exhibit 12-4's table, pc/h/ln by FFS mi/h.
    const std::map<std::string, std::pair<double, double>> table{
        {"freeway-55", {55, 2250}}, {"freeway-60", {60, 2300}}, {"freeway-65", {65, 2350}}, {"freeway-70", {70, 2400}}, {"freeway-75", {75, 2400}},
        {"multilane-45", {45, 1900}}, {"multilane-50", {50, 2000}}, {"multilane-55", {55, 2100}}, {"multilane-60", {60, 2200}},
        {"multilane-65", {65, 2300}}, {"multilane-70", {70, 2300}}};
    for (const auto& [t, path] : types) {
        const auto row = table.find(t.id);
        CHECK(row != table.end());
        if (row == table.end()) continue;
        const auto [ffs, capacity] = row->second;
        CHECK(t.capacityPerLane == capacity);
        // Exhibit 12-6's equations give the same.
        const bool freeway = t.id.starts_with("freeway");
        CHECK(t.capacityPerLane == (freeway ? std::min(2200 + 10 * (ffs - 50), 2400.0) : std::min(1900 + 20 * (ffs - 45), 2300.0)));
        test::near(t.freeSpeed, ffs * 0.44704, 1e-9);
        CHECK(t.jamDensity.has_value());
        if (t.jamDensity) test::near(t.jamDensity->value, 190 / 1.609344, 0.01);
        CHECK(t.capacityBasis == "ultimate" && t.vdfId == "bpr");
        CHECK(t.source.find("Exhibit 12-4") != std::string::npos && t.jamDensity && t.jamDensity->source.find("Exhibit 10-7") != std::string::npos);
    }
}
TEST(roadcatalog, rd3_macro_checks_the_capacity_basis) { // RD3
    auto installed = chain("freeway-70");
    CHECK(demand(installed).externalLinkTypes); // forcing: the installed HCM type
    const auto compiled = macroNetwork(installed, data());
    CHECK(issueAt(compiled.issues, "MACRO_CAPACITY_BASIS_MISMATCH", "network.links[0].linkType"));
    CHECK(issueAt(compiled.issues, "MACRO_CAPACITY_BASIS_MISMATCH", "network.links[1].linkType"));
    CHECK(compiled.issues.size() == 2); // nothing else: the type and its VDF resolve
    CHECK(mesoNetwork(installed, data()).issues.empty()); // Meso reads any basis
    // An owned type without a basis (practical) compiles as before.
    auto owned = chain("local");
    demand(owned).externalLinkTypes = false;
    demand(owned).linkTypes = {{"local", "Local", 600, 10, "bpr", {}, "Test values"}};
    CHECK(macroNetwork(owned, data()).issues.empty());
    // An ultimate type with an ultimate VDF (a temporary data directory) compiles.
    const auto dir = std::filesystem::temp_directory_path() / "multra-rd3";
    std::filesystem::remove_all(dir); std::filesystem::create_directories(dir / "vdf");
    std::ofstream(dir / "vdf/ult.json") << R"({"id":"ult","form":"bpr","alpha":0.15,"beta":4,"source":"Test values","capacityBasis":"ultimate"})";
    auto matched = owned;
    demand(matched).linkTypes = {{"local", "Local", 600, 10, "ult", {}, "Test values", {}, "ultimate"}};
    CHECK(macroNetwork(matched, dir).issues.empty());
    // A bad basis is refused at its field.
    demand(matched).linkTypes[0].capacityBasis = "design";
    CHECK(issueAt(linkTypeIssues(resolveLinkTypes(demand(matched), dir)), "INVALID_LINK_TYPE", "definition.linkTypes[0].capacityBasis"));
    CHECK(issueAt(vdfIssues({{"odd", "bpr", 0.15, 4, "Test values", "design"}}), "INVALID_VDF", "catalog/vdf/odd.capacityBasis"));
    std::filesystem::remove_all(dir);
}
