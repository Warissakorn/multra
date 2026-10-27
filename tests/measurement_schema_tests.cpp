#include "test.hpp"
#include "measurement_schema_cases.hpp"
#include "../src/project/network_schema.hpp"
#include "../src/project/network_fields.hpp"
#include "../src/model/network/right_of_way.hpp"

using namespace multra;
namespace {
const Json& resolve(const Json& schema,const Json& ref) {
    return schema.at(Json::json_pointer(ref.at("$ref").get<std::string>().substr(1)));
}
const Json& networkFor(const Json& schema,int version) {
    for(const auto& branch:schema.at("allOf"))
        for(const auto& v:branch.at("if").at("properties").at("schemaVersion").at("enum"))
            if(v==version)return resolve(schema,branch.at("then").at("properties").at("network"));
    throw std::runtime_error("Missing network version");
}
void unsupported(const Json& j,const std::string& path) {
    try {(void)parseDocument(j);}
    catch(const ValidationError& e) {CHECK(e.issues[0].code=="EDIT_UNSUPPORTED_FIELD");CHECK(e.issues[0].path==path);return;}
    throw std::runtime_error("Expected unknown field");
}
}
TEST(project, measurement_schema_preserves_version_gates_and_nullable_containers) {
    const auto schema=projectDocumentSchema();
    for(int v=1;v<=kLatestSchema;++v) {
        const auto& n=networkFor(schema,v);
        for(const auto& [name,gate]:{std::pair{"queueCounters",16},std::pair{"travelTimeSections",23}}) {
            CHECK(n.at("properties").contains(name)==(v>=gate));if(v<gate)continue;
            CHECK(n.at("properties").at(name).at("type")==Json::array({"array","null"}));
            const auto& row=resolve(schema,n.at("properties").at(name).at("items"));
            CHECK(row.at("additionalProperties")==false);CHECK(!row.at("properties").contains("generated"));
        }
        if(v<23)continue;
        const auto& s=resolve(schema,n.at("properties").at("travelTimeSections").at("items"));
        CHECK(s.at("properties").contains("controlType")== (v>=24));
        if(v>=24)CHECK(s.at("properties").at("controlType").at("enum")==Json::array({"signalised","unsignalised",nullptr}));
    }
}
TEST(project, measurement_schema_reuses_control_points_and_keeps_empty_sources_structural) {
    const auto schema=projectDocumentSchema();const auto& n=networkFor(schema,24);
    const auto& q=resolve(schema,n.at("properties").at("queueCounters").at("items"));
    const auto& l=resolve(schema,q.at("properties").at("lines").at("items"));
    CHECK(!l.contains("type"));CHECK(l.at("required").empty());CHECK(!l.contains("oneOf"));
    const auto& row=resolve(schema,n.at("properties").at("rightOfWay"));
    const auto& w=resolve(schema,row.at("properties").at("waitingLines").at("items"));
    CHECK(l.at("properties").at("point").at("anyOf")[0]==w.at("properties").at("point"));
    auto j=measurementCases::minimal(24);auto& line=j["network"]["queueCounters"][0]["lines"][0];
    line["referenceId"]=nullptr;CHECK(line.at("referenceId").is_null());
    CHECK(parseDocument(j).network.queueCounters[0].lines[0].referenceId.empty());
    line["point"]=nullptr;CHECK(line.at("point").is_null());
    CHECK(!parseNetwork(j.at("network"),24).queueCounters[0].lines[0].point);
    test::throws([&]{parseDocument(j);},"INVALID_MEASUREMENT_LINE");
    line=7;CHECK(line.is_number());
    CHECK(parseNetwork(j.at("network"),24).queueCounters[0].lines[0]==MeasurementLine{});
}
TEST(project, measurement_codec_keeps_diagnostic_paths_and_read_order) {
    auto j=measurementCases::minimal(24);auto& l=j["network"]["queueCounters"][0]["lines"][0];
    l["referenceId"]=7;l["point"]["station"]="bad";
    CHECK(l.at("referenceId").is_number());CHECK(l.at("point").at("station").is_string());
    test::throws([&]{parseDocument(j);},"Expected text: referenceId");
    l.erase("referenceId");l["point"]["path"]["extra"]=1;CHECK(l.at("point").at("path").contains("extra"));
    unsupported(j,"queueCounters[0].lines[0].point.path.extra");
    j=measurementCases::minimal(24,false);auto& s=j["network"]["travelTimeSections"][0];
    s["start"]["station"]="bad";s["controlType"]="invalid";
    CHECK(s.at("start").at("station").is_string());CHECK(s.at("controlType")=="invalid");
    test::throws([&]{parseDocument(j);},"Expected number: station");
    s["start"]["station"]=10;CHECK(s.at("start").at("station")==10);
    test::throws([&]{parseDocument(j);},"INVALID_ENUM");
    s["controlType"]=nullptr;j["schemaVersion"]=23;CHECK(j.at("schemaVersion")==23);
    unsupported(j,"travelTimeSections[0].controlType");
}
TEST(project, measurement_codec_preserves_defaults_save_versions_and_semantic_checks) {
    auto queue=measurementCases::minimal(16);auto d=parseDocument(queue);
    CHECK(d.network.queueCounters[0].name.empty());CHECK(documentJson(d).at("schemaVersion")==17);
    auto section=measurementCases::minimal(23,false);d=parseDocument(section);
    CHECK(d.network.travelTimeSections[0].name.empty());CHECK(!d.network.travelTimeSections[0].controlType);
    CHECK(documentJson(d).at("schemaVersion")==23);
    section["schemaVersion"]=24;section["network"]["travelTimeSections"][0]["controlType"]="unsignalised";
    CHECK(section.at("network").at("travelTimeSections")[0].at("controlType")=="unsignalised");
    d=parseDocument(section);CHECK(documentJson(d).at("schemaVersion")==24);
    section["network"]["travelTimeSections"][0]["start"]["station"]=90;
    CHECK(section.at("network").at("travelTimeSections")[0].at("start").at("station")==90);
    CHECK(parseNetwork(section.at("network"),24).travelTimeSections[0].start.station==90);
    test::throws([&]{parseDocument(section);},"INVALID_SECTION_ORDER");
    queue["network"]["queueCounters"][0]["lines"][0]["referenceId"]="missing";
    CHECK(queue.at("network").at("queueCounters")[0].at("lines")[0].at("referenceId")=="missing");
    CHECK(parseNetwork(queue.at("network"),16).queueCounters[0].lines[0].point.has_value());
    test::throws([&]{parseDocument(queue);},"INVALID_MEASUREMENT_LINE");
}
TEST(project, measurement_structural_refusals_are_also_refused_by_codec) {
    for(const auto& c:measurementCases::cases()) {
        if(c.schemaAccepts)continue;
        CHECK(c.document.contains("schemaVersion"));bool accepted=true;
        try {(void)parseDocument(c.document);}catch(const std::exception&) {accepted=false;}
        if(accepted)throw std::runtime_error("Codec accepted measurement structural refusal: "+c.name);
    }
}
