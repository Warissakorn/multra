#include "test.hpp"
#include "right_of_way_schema_cases.hpp"
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
    throw std::runtime_error("Missing network schema version");
}
void unsupported(const Json& j,const std::string& path) {
    try {(void)parseDocument(j);}
    catch(const ValidationError& e) {
        CHECK(e.issues[0].code=="EDIT_UNSUPPORTED_FIELD");CHECK(e.issues[0].path==path);return;
    }
    throw std::runtime_error("Expected unknown-key failure");
}
}
TEST(project, right_of_way_schema_tracks_gates_nullable_arrays_and_supported_enums) {
    const auto schema=projectDocumentSchema();
    for(int v=1;v<=kLatestSchema;++v) {
        const auto& n=networkFor(schema,v);
        CHECK(n.at("properties").contains("rightOfWay")== (v>=14));
        if(v<14)continue;
        const auto& row=resolve(schema,n.at("properties").at("rightOfWay"));
        CHECK(!row.contains("type"));CHECK(row.at("required").empty());
        CHECK(row.at("properties").contains("stopControls")== (v>=15));
        const auto& lines=row.at("properties").at("waitingLines");
        CHECK(lines.at("type")==Json::array({"array","null"}));
        const auto& w=resolve(schema,lines.at("items"));
        CHECK(w.at("additionalProperties")==false);CHECK(w.at("properties").contains("generated")== (v>=54));
        const auto& area=resolve(schema,row.at("properties").at("conflictAreas").at("items"));
        CHECK(area.at("properties").at("kind").at("enum")==Json::array({"crossing","merge"}));
        CHECK(area.at("properties").at("priority").at("enum")==Json::array({"firstYields","secondYields","undetermined"}));
    }
}
TEST(project, right_of_way_legacy_presence_and_path_decode_preserve_semantic_boundary) {
    for(const auto& container:std::vector<Json>{nullptr,7,true,"ignored",Json::array(),Json::object()}) {
        auto j=rightOfWayCases::minimal();j["network"]["rightOfWay"]=container;
        CHECK(j.at("network").at("rightOfWay")==container);
        CHECK(parseDocument(j).network.rightOfWay.empty());
    }
    auto j=rightOfWayCases::minimal();auto& row=j["network"]["rightOfWay"];
    for(const auto* key:{"waitingLines","conflictAreas","priorityRules","stopControls"})row[key]=nullptr;
    CHECK(row.at("waitingLines").is_null());CHECK(parseDocument(j).network.rightOfWay.empty());
    for(const auto& path:std::vector<Json>{nullptr,7,true,"ignored",Json::array(),Json::object()})
        CHECK(networkCodec::readControlPath(path,"point.path",17)==ControlPathRef{});
    const Json path={{"linkId","link"},{"laneId","lane"},{"connectorId",nullptr}};
    CHECK(networkCodec::readControlPath(path,"point.path",17).connectorId.empty());
}
TEST(project, right_of_way_diagnostics_and_generated_checks_keep_original_order) {
    auto j=rightOfWayCases::minimal();auto& w=j["network"]["rightOfWay"]["waitingLines"][0];
    w["point"]["extra"]=1;w.erase("id");CHECK(w.at("point").contains("extra"));CHECK(!w.contains("id"));
    unsupported(j,"rightOfWay.waitingLines[0].point.extra");
    j=rightOfWayCases::minimal(54);auto& line=j["network"]["rightOfWay"]["waitingLines"][0];
    line["generated"]=false;line["point"]["station"]="bad";
    CHECK(line.at("generated")==false);CHECK(line.at("point").at("station").is_string());
    test::throws([&]{parseDocument(j);},"Expected number: station");
    line["point"]["station"]=10;CHECK(line.at("point").at("station")==10);
    test::throws([&]{parseDocument(j);},"EDIT_GENERATED_INVALID");
    line["generated"]=true;CHECK(line.at("generated")==true);
    CHECK(parseDocument(j).network.rightOfWay.waitingLines[0].generated);
    j["schemaVersion"]=53;CHECK(j.at("schemaVersion")==53);
    unsupported(j,"rightOfWay.waitingLines[0].generated");
    j=rightOfWayCases::minimal(14);j["network"]["rightOfWay"]["stopControls"]=nullptr;
    CHECK(j.at("network").at("rightOfWay").contains("stopControls"));
    unsupported(j,"network.rightOfWay.stopControls");
}
TEST(project, right_of_way_structural_schema_does_not_replace_model_validation) {
    auto j=rightOfWayCases::minimal();CHECK(parseDocument(j).network.rightOfWay.conflictAreas.size()==1);
    j["network"]["rightOfWay"]["priorityRules"][0]["gapTime"]=-1;
    CHECK(j.at("network").at("rightOfWay").at("priorityRules")[0].at("gapTime")==-1);
    const auto n=parseNetwork(j.at("network"),17);
    const auto issues=rightOfWayStructuralIssues(n);
    CHECK(std::any_of(issues.begin(),issues.end(),[](const auto& i){return i.code=="INVALID_PRIORITY_RULE";}));
    test::throws([&]{parseDocument(j);},"INVALID_PRIORITY_RULE");
    j=rightOfWayCases::minimal();j["network"]["rightOfWay"]["waitingLines"][0]["point"]["path"]=true;
    CHECK(j.at("network").at("rightOfWay").at("waitingLines")[0].at("point").at("path")==true);
    CHECK(parseNetwork(j.at("network"),17).rightOfWay.waitingLines[0].point.path==ControlPathRef{});
    test::throws([&]{parseDocument(j);},"INVALID_CONTROL_PATH");
}
TEST(project, right_of_way_structural_refusals_are_also_refused_by_codec) {
    for(const auto& c:rightOfWayCases::cases()) {
        if(c.schemaAccepts)continue;
        CHECK(c.document.contains("schemaVersion"));bool accepted=true;
        try {(void)parseDocument(c.document);}catch(const std::exception&) {accepted=false;}
        if(accepted)throw std::runtime_error("Codec accepted right-of-way structural refusal: "+c.name);
    }
}
