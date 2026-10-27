#include "test.hpp"
#include "connector_schema_cases.hpp"
#include "../src/project/network_schema.hpp"

using namespace multra;
namespace {
const Json& resolve(const Json& schema,const Json& ref) {
    return schema.at(Json::json_pointer(ref.at("$ref").get<std::string>().substr(1)));
}
const Json& connectorFor(const Json& schema,int version) {
    for(const auto& branch:schema.at("allOf"))
        for(const auto& v:branch.at("if").at("properties").at("schemaVersion").at("enum"))
            if(v==version) {
                const auto& n=resolve(schema,branch.at("then").at("properties").at("network"));
                return resolve(schema,n.at("properties").at("connectors").at("items"));
            }
    throw std::runtime_error("Missing Connector schema version");
}
void issue(const Json& j,const std::string& code,const std::string& path) {
    try {(void)parseDocument(j);}
    catch(const ValidationError& e) {
        CHECK(e.issues.size()==1);CHECK(e.issues.front().code==code);CHECK(e.issues.front().path==path);return;
    }
    throw std::runtime_error("Expected Connector issue: "+code);
}
}
TEST(project, connector_schema_preserves_defaults_units_and_version_profiles) {
    const auto schema=projectDocumentSchema();const Connector defaults;
    for(int v=1;v<=kLatestSchema;++v) {
        const auto d=parseDocument(connectorCases::minimal(v));
        CHECK(d.network.connectors.size()==1);const auto& c=d.network.connectors.front();
        CHECK(c.fromLaneCount==defaults.fromLaneCount);CHECK(c.toLaneCount==defaults.toLaneCount);
        CHECK(c.level==defaults.level);CHECK(c.displayType==defaults.displayType);
        CHECK(c.name.empty());CHECK(c.laneBlend.empty());CHECK(c.laneWidths.empty());CHECK(c.laneMarkings.empty());
        CHECK(!c.from.station);CHECK(!c.to.station);CHECK(!c.laneChangeSide);CHECK(!c.generated);
        const auto& shape=connectorFor(schema,v);const auto& properties=shape.at("properties");
        CHECK(shape.at("additionalProperties")==Json(v<7));
        CHECK(properties.at("fromLaneCount").at("default")==c.fromLaneCount);
        CHECK(properties.at("displayType").at("default")==c.displayType);
        CHECK(properties.contains("laneChangeSide")== (v>=17));
        CHECK(properties.contains("generated")== (v>=54));
        const auto& ref=resolve(schema,properties.at("from"));
        CHECK(ref.at("properties").at(v<5?"station":"fraction")==false);
        CHECK(ref.at("properties").at(v<5?"fraction":"station")==Json{{"type","number"}});
    }
}
TEST(project, connector_fraction_migration_and_shared_signal_reference_reads_are_unchanged) {
    for(int v=1;v<=kLatestSchema;++v) {
        auto j=connectorCases::minimal(v);auto& c=j["network"]["connectors"][0];
        const char* key=v<5?"fraction":"station";
        c["from"][key]=v<5?0.5:50.;c["to"][key]=v<5?0.25:25.;
        c["geometry"]=Json::array({Json{{"x",50},{"y",0}},Json{{"x",145},{"y",0}}});
        CHECK(c.at("from").contains(key));
        const auto d=parseDocument(j);
        CHECK(d.network.connectors[0].from.station.has_value());
        CHECK(d.network.connectors[0].to.station.has_value());
        test::near(*d.network.connectors[0].from.station,50,1e-9);
        test::near(*d.network.connectors[0].to.station,25,1e-9);
        j["network"]["signalHeads"].push_back({{"id","head"},
            {"lane",{{"linkId","from"},{"laneId","from-lane"},{key,v<5?0.5:50.}}},
            {"position",0},{"programId","legacy"}});
        const auto network=parseNetwork(j.at("network"),v);
        CHECK(network.signalHeads.size()==1);
        CHECK(network.signalHeads[0].lane.station.has_value());
        test::near(*network.signalHeads[0].lane.station,50,1e-9);
        c["from"][v<5?"station":"fraction"]=nullptr;
        CHECK(c.at("from").contains(v<5?"station":"fraction"));
        test::throws([&]{parseDocument(j);},"EDIT_VERSION");
    }
}
TEST(project, connector_optional_side_generated_and_diagnostic_paths_keep_their_gates) {
    auto j=connectorCases::minimal(17);j["network"]["connectors"][0]["laneChangeSide"]=nullptr;
    CHECK(j.at("network").at("connectors")[0].at("laneChangeSide").is_null());
    CHECK(!parseDocument(j).network.connectors.front().laneChangeSide);
    j["schemaVersion"]=16;issue(j,"EDIT_UNSUPPORTED_FIELD","connectors[0].laneChangeSide");
    j["schemaVersion"]=17;j["network"]["connectors"][0]["laneChangeSide"]="bad";
    CHECK(j.at("network").at("connectors")[0].at("laneChangeSide")=="bad");
    issue(j,"EDIT_LANE_RANGE","connectors[0].laneChangeSide");
    j=connectorCases::minimal(54);j["network"]["connectors"][0]["generated"]=false;
    CHECK(j.at("network").at("connectors")[0].at("generated")==false);
    issue(j,"EDIT_GENERATED_INVALID","connectors[0].generated");
    j["schemaVersion"]=53;issue(j,"EDIT_UNSUPPORTED_FIELD","connectors[0].generated");
    j["schemaVersion"]=6;CHECK(!parseDocument(j).network.connectors[0].generated);
    j=connectorCases::minimal(7);j["network"]["connectors"][0]["from"]["extra"]=true;
    CHECK(j.at("network").at("connectors")[0].at("from").at("extra")==true);
    issue(j,"EDIT_UNSUPPORTED_FIELD","reference.extra");
}
TEST(project, connector_array_and_native_integer_errors_are_preserved) {
    auto j=connectorCases::minimal();j["network"]["connectors"][0]["laneBlend"]=Json::array({"bad"});
    CHECK(j.at("network").at("connectors")[0].at("laneBlend")[0].is_string());
    test::throws([&]{parseDocument(j);},"INVALID_GEOMETRY");
    j=connectorCases::minimal();j["network"]["connectors"][0]["laneWidths"]=Json::array({"bad"});
    CHECK(j.at("network").at("connectors")[0].at("laneWidths")[0].is_string());
    test::throws([&]{parseDocument(j);},"INVALID_WIDTH");
    j=connectorCases::minimal();j["network"]["connectors"][0]["fromLaneCount"]=1.0;
    CHECK(j.at("network").at("connectors")[0].at("fromLaneCount").is_number_float());
    test::throws([&]{parseDocument(j);},"EDIT_DISPLAY_VALUE");
}
TEST(project, connector_structural_refusal_corpus_is_also_refused_by_the_codec) {
    for(const auto& c:connectorCases::cases()) {
        if(c.schemaAccepts)continue;
        CHECK(c.document.contains("schemaVersion"));bool accepted=true;
        try {(void)parseDocument(c.document);}catch(const std::exception&) {accepted=false;}
        if(accepted)throw std::runtime_error("Codec accepted Connector structural refusal: "+c.name);
    }
}
