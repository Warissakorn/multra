#include "test.hpp"
#include "network_schema_cases.hpp"
#include "../src/project/network_schema.hpp"
#include "../src/project/document_envelope.hpp"

using namespace multra;
namespace {
Json minimal(int version=17) {
    auto j=documentJson(ProjectDocument{});j["schemaVersion"]=version;
    j["network"]["links"].push_back({{"id","link"},
        {"geometry",Json::array({Json{{"x",0},{"y",0}},Json{{"x",100},{"y",0}}})},
        {"lanes",Json::array({Json{{"id","lane"},{"width",3.5}}})}});
    return j;
}
const Json& dereference(const Json& schema,const Json& reference) {
    return schema.at(Json::json_pointer(reference.at("$ref").get<std::string>().substr(1)));
}
const Json& networkFor(const Json& schema,int version) {
    for(const auto& branch:schema.at("allOf"))
        for(const auto& allowed:branch.at("if").at("properties").at("schemaVersion").at("enum"))
            if(allowed==version)return dereference(schema,branch.at("then").at("properties").at("network"));
    throw std::runtime_error("Version missing from generated schema");
}
const Json& linkFor(const Json& schema,int version) {
    return dereference(schema,networkFor(schema,version).at("properties").at("links").at("items"));
}
void unsupported(const Json& input,const std::string& path) {
    try {(void)parseDocument(input);}
    catch(const ValidationError& e) {
        CHECK(e.issues.size()==1);CHECK(e.issues.front().code=="EDIT_UNSUPPORTED_FIELD");
        CHECK(e.issues.front().path==path);return;
    }
    throw std::runtime_error("Expected unsupported-field refusal at "+path);
}
}
TEST(project, network_schema_keeps_envelope_api_and_reuses_versioned_definitions) {
    const auto envelope=projectEnvelopeSchema(),schema=projectDocumentSchema();
    CHECK(envelope.at("x-multra-coverage")=="envelope-background");
    CHECK(!envelope.contains("allOf"));
    CHECK(schema.at("x-multra-coverage")=="envelope-background-network-link-lane");
    CHECK(schema.at("x-multra-complete")==false);
    CHECK(schema.at("$defs").size()<kLatestSchema);
    CHECK(schema.at("allOf").size()<kLatestSchema);
    CHECK(projectDocumentSchema()==schema);
    for(int v=1;v<=kLatestSchema;++v) {
        const auto& n=networkFor(schema,v);
        CHECK(n.at("additionalProperties")==Json(v<7));
        CHECK(linkFor(schema,v).at("additionalProperties")==Json(v<7));
        CHECK(n.at("properties").at("connectors").at("items")==Json::object());
        const auto& l=linkFor(schema,v).at("properties");
        CHECK(l.at("linkType")== (v<50?Json(false):Json{{"type","string"}}));
        CHECK(l.at("roadId")== (v<53?Json(false):Json{{"type","string"}}));
        CHECK(l.at("name").at("type")==Json::array({"string","null"}));
    }
}
TEST(project, link_schema_defaults_and_legacy_reads_match_the_model) {
    const auto schema=projectDocumentSchema();const Link defaults;
    for(int version=1;version<=kLatestSchema;++version) {
        const auto d=parseDocument(minimal(version));
        CHECK(d.network.links.size()==1);const auto& link=d.network.links.front();
        CHECK(link.level==defaults.level);CHECK(link.displayType==defaults.displayType);
        CHECK(link.laneOffset==defaults.laneOffset);CHECK(link.name==defaults.name);
        CHECK(link.boundaryMarkings.empty());
        const auto& properties=linkFor(schema,version).at("properties");
        CHECK(properties.at("level").at("default")==link.level);
        CHECK(properties.at("displayType").at("default")==link.displayType);
        CHECK(properties.at("laneOffset").at("default")==link.laneOffset);
        auto nullName=minimal(version);nullName["network"]["links"][0]["name"]=nullptr;
        CHECK(nullName.at("network").at("links")[0].at("name").is_null());
        CHECK(parseDocument(nullName)==d);
    }
}
TEST(project, network_link_lane_point_known_key_paths_keep_the_schema_7_boundary) {
    for(const auto& [pointer,path]:std::vector<std::pair<std::string,std::string>>{
        {"/network/extra","network.extra"},{"/network/links/0/extra","links[0].extra"},
        {"/network/links/0/lanes/0/extra","links[0].lanes[0].extra"},
        {"/network/links/0/geometry/0/extra","geometry[0].extra"}}) {
        auto j=minimal(6);j[Json::json_pointer(pointer)]=true;
        CHECK(j.at(Json::json_pointer(pointer))==true);
        CHECK(parseDocument(j).network.links.size()==1);
        j["schemaVersion"]=7;unsupported(j,path);
    }
}
TEST(project, link_reference_gates_stay_in_the_existing_document_read_stages) {
    auto j=minimal(21);j["network"]["links"][0]["linkType"]=nullptr;
    CHECK(j.at("network").at("links")[0].contains("linkType"));
    // The standalone network reader knows the key from 21 but does not consume it.
    CHECK(parseNetwork(j.at("network"),21).links.front().linkTypeId==std::nullopt);
    unsupported(j,"network.links[0].linkType");
    j["schemaVersion"]=50;
    test::throws([&]{parseDocument(j);},"INVALID_LINK_TYPE");
    j=minimal(52);j["network"]["links"][0]["roadId"]="missing";
    CHECK(j.at("network").at("links")[0].at("roadId")=="missing");
    unsupported(j,"network.links[0].roadId");
    j=minimal(17);j["network"]["links"][0]["level"]=1.0;
    CHECK(j.at("network").at("links")[0].at("level").is_number_float());
    test::throws([&]{parseDocument(j);},"EDIT_DISPLAY_VALUE");
}
TEST(project, link_structural_schema_keeps_semantic_checks_in_the_model) {
    auto j=minimal();j["network"]["links"][0]["lanes"][0]["width"]=0;
    CHECK(j.at("network").at("links")[0].at("lanes")[0].at("width")==0);
    test::throws([&]{parseDocument(j);},"INVALID_WIDTH");
    const auto schema=projectDocumentSchema();
    const auto& lanes=linkFor(schema,17).at("properties").at("lanes");
    const auto& width=dereference(schema,lanes.at("items")).at("properties").at("width");
    CHECK(width==Json{{"type","number"}});
}
TEST(project, network_structural_refusal_cases_are_also_refused_by_the_codec) {
    for(const auto& c:networkCases::cases()) {
        if(c.schemaAccepts)continue;
        CHECK(c.document.contains("schemaVersion"));
        bool accepted=true;
        try {(void)parseDocument(c.document);}catch(const std::exception&) {accepted=false;}
        if(accepted)throw std::runtime_error("Codec accepted structural refusal case: "+c.name);
    }
}
