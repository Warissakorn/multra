#include "test.hpp"
#include "../src/project/document.hpp"
#include "../src/project/document_schema.hpp"
#include <fstream>

using namespace multra;
TEST(project, schema_inspection_selects_features_without_mutating_the_document) {
    ProjectDocument d;
    const auto original=d;
    CHECK(documentSchemaVersion(d)==17);
    CHECK(d==original);
    d.map=MapSettings{};
    CHECK(documentSchemaVersion(d)==29);
    d.zones.zones.push_back({"zone","",{{0,0},{10,0},{0,10}}, {}});
    CHECK(documentSchemaVersion(d)==31);
    d.zones.studyPeriods.push_back({"am","Morning",0,3600});
    CHECK(documentSchemaVersion(d)==49);
    d.definition.emplace();d.definition->macroSettings=kMacroDefaults;
    CHECK(documentSchemaVersion(d)==51);
    d.definition->mesoSettings=kMesoDefaults;
    CHECK(documentSchemaVersion(d)==52);
    d.macroRuns.push_back({"named-run","Morning","am",kMacroDefaults});
    CHECK(documentSchemaVersion(d)==56);
    const auto saved=documentJson(d);
    CHECK(saved.at("schemaVersion")==56);
    CHECK(parseDocument(saved)==d);
    // Removing higher-version features returns to the remaining lower feature floor.
    d.macroRuns.clear();d.definition->mesoSettings.reset();
    CHECK(documentSchemaVersion(d)==51);
    d.definition.reset();d.zones={};d.map.reset();
    CHECK(d==original);CHECK(documentSchemaVersion(d)==17);
    CHECK(documentJson(d)==documentJson(original));
}
TEST(project, schema_inspection_does_not_promise_draft_validity) {
    ProjectDocument d;
    ZoneConnector c;c.id="missing-zone-target";c.streetSegmentId="missing-road";
    d.zones.zoneConnectors.push_back(c);
    CHECK(!d.zones.zoneConnectors.front().streetSegmentId.empty());
    CHECK(documentSchemaVersion(d)==59); // D389: street-segment keys
    test::throws([&]{validateDocument(d);});
}
TEST(project, schema_input_version_is_not_retained_as_the_output_version) {
    const auto canonical=documentJson(ProjectDocument{});
    for(int version=1;version<=kLatestSchema;++version) {
        auto input=canonical;input["schemaVersion"]=version;
        CHECK(input.at("schemaVersion")==version);
        const auto decoded=parseDocument(input);
        CHECK(documentSchemaVersion(decoded)==17);
        CHECK(documentJson(decoded)==canonical);
    }
}
TEST(project, schema_inspection_preserves_shipped_project_versions_and_roundtrips) {
    for(const auto* name:{"lane-change-lab","t-junction-priority","m2.6-study-template","four-leg-signalised"}) {
        std::ifstream file(test::root()/"data/projects"/(std::string(name)+".traffic.json"));
        CHECK(file.good());const auto input=Json::parse(file);
        const auto decoded=parseDocument(input);
        CHECK(documentSchemaVersion(decoded)==input.at("schemaVersion").get<int>());
        const auto encoded=documentJson(decoded);
        CHECK(parseDocument(encoded)==decoded);
        CHECK(documentJson(parseDocument(encoded))==encoded);
    }
}
