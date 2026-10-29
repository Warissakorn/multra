#include "test.hpp"
#include "../src/project/junction_template_codec.hpp"
#include "../src/project/input_manifest.hpp"
#include <fstream>
#include <set>
using namespace multra;
namespace {
Json recipe() {
    return {{"id","signal-4-major-far-pockets"},{"name","Four legs, major far pockets"},
        {"kind","junction"},{"legs",4},{"control","signalised"},{"phasing","P2"},
        {"roles",{{"major",Json::array({{{"movements","F"},{"pocket",true}},{{"movements","T"}},{{"movements","TK"}}})},
                  {"minor",Json::array({{{"movements","FT"}},{{"movements","TK"}}})}}}};
}
template<class F> void refused(F&& call, const std::string& code, const std::string& path) {
    bool caught = false;
    try { call(); } catch (const ValidationError& e) {
        caught = true;
        CHECK(std::any_of(e.issues.begin(),e.issues.end(),[&](const auto& i) { return i.code == code && i.path == path; }));
    }
    CHECK(caught);
}
void invalidAt(const std::string& pointer, const Json& value, const std::string& suffix) {
    auto j = recipe(); const auto p = Json::json_pointer(pointer); j[p] = value;
    CHECK(j[p] == value && j != recipe()); // forcing: mutation must actually change the fixture
    refused([&] { parseJunctionTemplate(j,"templates[2]"); },"JUNCTION_TEMPLATE_INVALID","templates[2]"+suffix);
}
struct CatalogDirectory {
    std::filesystem::path root;
    CatalogDirectory() {
        for (unsigned i = 0;; ++i) {
            root = std::filesystem::temp_directory_path()/("multra-junction-catalog-"+std::to_string(i));
            if (std::filesystem::create_directory(root)) break;
        }
        std::filesystem::create_directory(root/"junction-templates");
    }
    ~CatalogDirectory() { std::error_code ignored; std::filesystem::remove_all(root,ignored); }
    void write(const std::string& name, const Json& j) {
        const auto path = root/"junction-templates"/name;
        { std::ofstream out(path); out << j.dump(); CHECK(out.good()); }
        CHECK(readInputJson(path,name) == j); // forcing: file bytes parse as the intended fixture
    }
};
}
TEST(junctiontemplates, jt5_empty_noncontiguous_and_crossed_movements) {
    for (const auto* moves : {"","FK","KF","TT","L","TF"})
        invalidAt("/roles/major/0/movements",moves,".roles.major[0].movements");
    invalidAt("/roles/major/1/movements","K",".roles.major[2].movements"); // K before TK crosses
    invalidAt("/roles/minor",Json::array(),".roles.minor");
}
TEST(junctiontemplates, jt5_pockets_and_unknown_row_keys) {
    invalidAt("/roles/major/0/movements","FT",".roles.major[0].pocket");
    invalidAt("/roles/major/0/pocket","true",".roles.major[0].pocket");
    invalidAt("/roles/major/0/side","left",".roles.major[0].side");
    invalidAt("/roles/major/0/width",3.5,".roles.major[0].width");
}
TEST(junctiontemplates, jt5_roles_leg_count_and_catalog_shape) {
    auto j = recipe(); j["roles"].erase("minor");
    CHECK(!j["roles"].contains("minor")); // forcing
    refused([&] { parseJunctionTemplate(j,"t"); },"JUNCTION_TEMPLATE_INVALID","t.roles.minor");
    invalidAt("/legs",3,".roles.major-far");
    invalidAt("/legs",5,".legs"); invalidAt("/legs",4.5,".legs");
    invalidAt("/roles/west",Json::array({{{"movements","T"}}}),".roles.west");
    invalidAt("/control","yield",".control"); invalidAt("/phasing","P5",".phasing");
    invalidAt("/id"," ",".id"); invalidAt("/siteSpeed",50,".siteSpeed");
}
TEST(junctiontemplates, jt5_valid_templates_keep_structure_and_optional_side) {
    auto j = recipe(); const auto t = parseJunctionTemplate(j,"t");
    CHECK(t.id == j.at("id").get<std::string>() && t.roles.at("major").size() == 3);
    CHECK(junctionTemplateJson(t) == j);
    j["roles"]["major"][0]["side"] = "far";
    CHECK(junctionTemplateJson(parseJunctionTemplate(j,"t")) == j);
    j = {{"id","t-stop"},{"name","Three legs"},{"kind","junction"},{"legs",3},{"control","twoWayStop"},
         {"roles",{{"major-far",Json::array({{{"movements","FT"}}})},
                   {"major-kerb",Json::array({{{"movements","TK"}}})},
                   {"minor",Json::array({{{"movements","F"}},{{"movements","K"}}})}}}};
    CHECK(junctionTemplateJson(parseJunctionTemplate(j,"t")) == j);
    j["control"] = "allWayStop";
    CHECK(parseJunctionTemplate(j,"t").control == "allWayStop");
}
TEST(junctiontemplates, jt5_installed_structural_set_is_deterministic) {
    AuthoringDefinition d; InputManifest manifest;
    const auto a = resolveJunctionTemplates(d,test::root()/"data",&manifest);
    CHECK(a.size() == 12);
    std::set<std::string> ids, presets;
    std::string previous;
    for (const auto& [t,path] : a) {
        CHECK(path > previous); previous = path;
        CHECK(ids.insert(t.id).second && junctionTemplateIssues(t,path).empty());
        const auto j = junctionTemplateJson(t);
        CHECK(!j.contains("signal") && !j.contains("storage") && !j.contains("taper"));
        if (t.control == "signalised") presets.insert(t.phasing);
    }
    CHECK(presets == std::set<std::string>{"P1","P2","P3","P4"});
    CHECK(resolveJunctionTemplates(d,test::root()/"data").size() == a.size());
    CHECK(manifest.json().at("files").size() == 12);
}
TEST(junctiontemplates, jt5_installed_and_owned_invalid_entries_keep_their_paths) {
    CatalogDirectory catalog;
    auto bad = recipe(); bad["roles"]["major"][0]["movements"] = "FK";
    catalog.write("bad.json",bad);
    refused([&] { resolveJunctionTemplates(AuthoringDefinition{},catalog.root); },
            "JUNCTION_TEMPLATE_INVALID","catalog/junction-templates/bad.json.roles.major[0].movements");
    ProjectDocument d; d.definition.emplace(); auto j = documentJson(d); j["schemaVersion"] = 61;
    j["definition"]["junctionTemplates"] = Json::array({bad});
    CHECK(j["definition"]["junctionTemplates"][0]["roles"]["major"][0]["movements"] == "FK");
    refused([&] { parseDocument(j); },"JUNCTION_TEMPLATE_INVALID","definition.junctionTemplates[0].roles.major[0].movements");
    j["definition"]["junctionTemplates"] = nullptr;
    CHECK(j["definition"]["junctionTemplates"].is_null());
    refused([&] { parseDocument(j); },"JUNCTION_TEMPLATE_INVALID","definition.junctionTemplates");
}
TEST(junctiontemplates, jt5_duplicate_ids_and_broken_json_are_refused_by_filename) {
    CatalogDirectory catalog; catalog.write("a.json",recipe()); catalog.write("b.json",recipe());
    CHECK(readInputJson(catalog.root/"junction-templates/a.json","a").at("id") ==
          readInputJson(catalog.root/"junction-templates/b.json","b").at("id")); // forcing
    refused([&] { resolveJunctionTemplates(AuthoringDefinition{},catalog.root); },
            "JUNCTION_TEMPLATE_INVALID","catalog/junction-templates/b.json.id");
    std::filesystem::remove(catalog.root/"junction-templates/b.json");
    { std::ofstream out(catalog.root/"junction-templates/a.json"); out << "{"; CHECK(out.good()); }
    std::ifstream in(catalog.root/"junction-templates/a.json");
    CHECK(std::string(std::istreambuf_iterator<char>(in),{}) == "{"); // forcing: syntactically invalid
    refused([&] { resolveJunctionTemplates(AuthoringDefinition{},catalog.root); },
            "JUNCTION_TEMPLATE_INVALID","catalog/junction-templates/a.json");
}
TEST(junctiontemplates, jt5_shipped_protected_far_turns_have_separate_rows) {
    const auto templates = resolveJunctionTemplates(AuthoringDefinition{},test::root()/"data");
    CHECK(templates.size() == 12); // forcing: the installed set, not an absent catalog
    std::size_t checked = 0;
    for (const auto& [t,path] : templates) {
        (void)path;
        if (t.phasing != "P2" && t.phasing != "P3") continue;
        for (const auto& [role,lanes] : t.roles) {
            if (t.phasing == "P2" && role != "major") continue;
            ++checked;
            for (const auto& lane : lanes)
                if (lane.movements.find('F') != std::string::npos) CHECK(lane.movements == "F");
        }
    }
    CHECK(checked == 6); // P2 major x 2 variants; P3 major+minor x 2 variants
}
