#include "test.hpp"
#include "../src/project/document.hpp"
#include "../src/project/document_schema.hpp"
using namespace multra;
namespace {
Json file() {
    ProjectDocument d; d.definition.emplace();
    d.network.nodes.push_back({"node","Site",{{-20,-20},{20,-20},{20,20},{-20,20}},true});
    d.network.links.push_back({"in",{{-200,0},{-10,0}},{{"lane",3.5}}});
    auto j = documentJson(d); j["schemaVersion"] = 61;
    j["network"]["nodes"][0]["junction"] = {{"templateId","signal-4-major-far-pockets"},
        {"major",{"r2","r3"}}, {"legs",{{"r2",{{"storage",{60,nullptr}},{"taper",{20,25}},
            {"lanes",Json::array({{{"movements","F"},{"pocket",true},{"side","far"}},
                                  {{"movements","T"}},{{"movements","K"},{"pocket",true}}})}}}}},
        {"signal",{{"cycle",100},{"phases",Json::array({{{"number",2},{"green",38},{"yellow",3},{"allRed",2}},
                                                       {{"number",6},{"yellow",0}}})}}}};
    j["network"]["nodes"][0]["approachDirections"] = {{"in","E"}};
    return j;
}
template<class F> void refused(F&& call, const std::string& code, const std::string& path) {
    bool caught = false;
    try { call(); } catch (const ValidationError& e) {
        caught = true;
        CHECK(std::any_of(e.issues.begin(),e.issues.end(),[&](const auto& i) { return i.code == code && i.path == path; }));
    }
    CHECK(caught);
}
}
TEST(junctiontemplates, jt5_recipe_incomplete_values_and_owned_empty_roundtrip_at_61) {
    auto j = file();
    CHECK(j["network"]["nodes"][0]["junction"]["legs"]["r2"]["storage"][1].is_null()); // forcing
    const auto d = parseDocument(j);
    CHECK(documentSchemaVersion(d) == 61 && documentJson(d) == j);
    j["definition"]["junctionTemplates"] = Json::array();
    CHECK(j["definition"]["junctionTemplates"].empty());
    CHECK(documentJson(parseDocument(j)) == j); // owned empty remains owned, no installed fallback
    j["network"]["nodes"][0]["junction"] = {{"templateId","not-yet-installed"}};
    CHECK(documentJson(parseDocument(j)) == j); // a recipe can precede its fill's missing-piece checks
}
TEST(junctiontemplates, jt5_each_new_field_is_refused_before_61_at_its_path_even_null) {
    for (const auto version : {0,17,48,59,60}) for (const auto* key : {"junction","approachDirections","junctionTemplates"}) {
        auto j = file(); auto& node = j["network"]["nodes"][0];
        node.erase("junction"); node.erase("approachDirections");
        const bool catalog = std::string(key) == "junctionTemplates";
        const auto path = catalog ? "definition.junctionTemplates" : "network.nodes[0]."+std::string(key);
        if (catalog) j["definition"][key] = nullptr; else node[key] = nullptr;
        if (version == 0) j.erase("schemaVersion"); else j["schemaVersion"] = version;
        CHECK((catalog ? j["definition"][key] : node[key]).is_null()); // forcing: presence, not non-null
        refused([&] { parseDocument(j); },"EDIT_UNSUPPORTED_FIELD",path);
    }
}
TEST(junctiontemplates, jt5_recipe_shape_sign_paths_and_coarse_coexistence) {
    const std::vector<std::pair<std::string,Json>> cases{
        {"/junction/templateId"," "}, {"/junction/extra",true}, {"/junction/major",{"r2","r2"}},
        {"/junction/legs/r2/storage/0",-1}, {"/junction/legs/r2/taper/0",0},
        {"/junction/signal/cycle",0}, {"/junction/signal/phases/0/green",0},
        {"/junction/signal/phases/0/yellow",-1}, {"/junction/signal/phases/0/allRed",-1},
        {"/junction/signal/phases/1/number",2}, {"/approachDirections/in","NE"}, {"/approachDirections/ghost","N"}};
    for (const auto& [pointer,value] : cases) {
        auto j = file(); auto& node = j["network"]["nodes"][0]; node[Json::json_pointer(pointer)] = value;
        CHECK(node[Json::json_pointer(pointer)] == value); // forcing
        std::string path = "network.nodes[0]";
        // Numeric JSON-pointer components are indices, the others are field/map keys.
        std::size_t pos = 1;
        while (pos < pointer.size()) {
            const auto next = pointer.find('/',pos); const auto part = pointer.substr(pos,next-pos);
            path += part.find_first_not_of("0123456789") == std::string::npos ? "["+part+"]" : "."+part;
            if (next == std::string::npos) break;
            pos = next+1;
        }
        if (pointer == "/junction/major") path += "[1]";
        refused([&] { parseDocument(j); },"EDIT_NODE_INVALID",path);
    }
    auto j = file(); j["network"]["nodes"][0]["coarse"] = Json::object();
    CHECK(j["network"]["nodes"][0].contains("coarse") && j["network"]["nodes"][0].contains("junction"));
    CHECK(documentJson(parseDocument(j)) == j);
}
TEST(junctiontemplates, jt5_no_feature_keeps_older_file_version_and_bytes) {
    ProjectDocument d; d.definition.emplace(); const auto original = documentJson(d);
    CHECK(original.at("schemaVersion") == 17);
    CHECK(documentJson(parseDocument(original)) == original);
    for (const auto version : {48,53,59,60,61}) {
        auto j = original; j["schemaVersion"] = version;
        CHECK(documentJson(parseDocument(j)) == original);
    }
}
