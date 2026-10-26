#include "test.hpp"
#include "demand_schema_cases.hpp"
#include "../src/project/network_schema.hpp"

using namespace multra;
namespace {
const Json& resolve(const Json& schema,const Json& ref) {
    return schema.at(Json::json_pointer(ref.at("$ref").get<std::string>().substr(1)));
}
const Json& definitionFor(const Json& schema,int version) {
    for(const auto& branch:schema.at("allOf"))
        for(const auto& v:branch.at("if").at("properties").at("schemaVersion").at("enum"))
            if(v==version)return resolve(schema,branch.at("then").at("properties").at("definition"));
    throw std::runtime_error("Missing definition version");
}
}
TEST(project, demand_schema_matches_required_arrays_open_rows_and_counted_version_gate) {
    const auto schema=projectDocumentSchema();
    for(int v=1;v<=kLatestSchema;++v) {
        const auto& d=definitionFor(schema,v);
        CHECK(d.at("properties").at("duration")==Json{{"type","number"}});
        CHECK(d.at("required")==Json::array({"signalPrograms","duration","timeStep","routes","inputs"}));
        const auto& route=resolve(schema,d.at("properties").at("routes").at("items"));
        const auto& input=resolve(schema,d.at("properties").at("inputs").at("items"));
        const auto& interval=resolve(schema,input.at("properties").at("intervals").at("items"));
        CHECK(route.at("additionalProperties")==true);CHECK(input.at("additionalProperties")==true);
        CHECK(interval.at("additionalProperties")==true);
        CHECK(route.at("properties").at("segmentIds").at("items")==Json{{"type","string"}});
        CHECK(input.at("properties").at("volumeFromCounts")== (v<26?Json(false):Json{{"type","boolean"}}));
        CHECK(input.at("properties").at("compositionId").at("type")=="string");
    }
}
TEST(project, demand_interval_totals_and_scalar_read_order_are_preserved) {
    auto j=demandCases::minimal();auto& i=j["definition"]["inputs"][0];
    i["intervals"]=Json::array({Json{{"startTime",10},{"endTime",20},{"vehiclesPerHour",360}},
        Json{{"startTime",30},{"endTime",50},{"vehiclesPerHour",720}}});
    CHECK(i.at("vehiclesPerHour")==100);CHECK(i.at("startTime")==0);
    const auto d=parseDocument(j);const auto& input=d.definition->inputs[0];
    CHECK(input.startTime==10);CHECK(input.endTime==50);test::near(input.vehiclesPerHour,450);
    CHECK(documentJson(d).at("definition").at("inputs")[0].at("vehiclesPerHour")==450);
    i["vehiclesPerHour"]=nullptr;CHECK(i.at("vehiclesPerHour").is_null());
    test::throws([&]{parseDocument(j);},"Field is null: vehiclesPerHour");
    i["vehiclesPerHour"]=100;i["intervals"]=Json::array();CHECK(i.at("intervals").empty());
    CHECK(parseDocument(j).definition->inputs[0].vehiclesPerHour==100);
}
TEST(project, demand_counted_guard_and_sync_stay_outside_standalone_definition_reads) {
    auto j=demandCases::minimal(25);auto& i=j["definition"]["inputs"][0];i["volumeFromCounts"]=false;
    CHECK(i.at("volumeFromCounts")==false);CHECK(!parseDefinition(j.at("definition")).inputs[0].volumeFromCounts);
    try {parseDocument(j);CHECK(false);}
    catch(const ValidationError& e) {CHECK(e.issues[0].path=="definition.inputs.volumeFromCounts");}
    j["schemaVersion"]=26;i["volumeFromCounts"]=true;i["routingDecisionId"]="decision";i["routeId"]="";
    j["definition"]["routingDecisions"]=Json::array({Json{{"id","decision"},
        {"routes",Json::array({Json{{"routeId","route"},{"relativeFlow",1},{"intervalFlows",Json::array({2,4})}}})},
        {"intervals",Json::array({Json{{"startTime",0},{"endTime",10}},Json{{"startTime",20},{"endTime",40}}})}}});
    CHECK(i.at("vehiclesPerHour")==100);CHECK(i.at("routeId")=="");
    CHECK(parseDefinition(j.at("definition")).inputs[0].vehiclesPerHour==100);
    const auto d=parseDocument(j);CHECK(d.definition->inputs[0].intervals.size()==2);
    test::near(d.definition->inputs[0].vehiclesPerHour,540);
    const auto saved=documentJson(d);CHECK(saved.at("schemaVersion")==26);
    CHECK(!saved.at("definition").at("inputs")[0].contains("intervals"));
}
TEST(project, demand_optional_nulls_open_keys_and_route_migration_keep_existing_behavior) {
    auto j=demandCases::minimal();auto& i=j["definition"]["inputs"][0];
    i["compositionId"]=nullptr;CHECK(i.at("compositionId").is_null());
    test::throws([&]{parseDocument(j);},"Field is null: compositionId");
    i.erase("compositionId");i["laneShares"]=Json::array({"bad"});CHECK(i.at("laneShares")[0].is_string());
    test::throws([&]{parseDocument(j);},"Expected numbers: laneShares");
    i.erase("laneShares");i["extra"]=1;j["definition"]["routes"][0]["extra"]=1;
    j["definition"]["routes"][0]["segmentIds"]=Json::array({"lane"});
    CHECK(j.at("definition").at("routes")[0].at("segmentIds")[0]=="lane");
    CHECK(parseDefinition(j.at("definition")).routes[0].segmentIds==std::vector<std::string>{"lane"});
    const auto d=parseDocument(j);CHECK(d.definition->routes[0].segmentIds==std::vector<std::string>{"link"});
    CHECK(d.definition->inputs[0].laneShares.empty());
}
TEST(project, demand_schema_refusals_and_semantic_exclusions_match_codec_stages) {
    for(const auto& c:demandCases::cases()) {
        if(c.schemaAccepts)continue;
        CHECK(c.document.contains("schemaVersion"));bool accepted=true;
        try {(void)parseDocument(c.document);}catch(const std::exception&) {accepted=false;}
        if(accepted)throw std::runtime_error("Codec accepted demand structural refusal: "+c.name);
    }
    auto j=demandCases::minimal();j["definition"]["inputs"][0]["vehiclesPerHour"]=-1;
    CHECK(j.at("definition").at("inputs")[0].at("vehiclesPerHour")==-1);
    CHECK(parseDefinition(j.at("definition")).inputs[0].vehiclesPerHour==-1);
    test::throws([&]{parseDocument(j);},"INVALID_NUMBER");
}
