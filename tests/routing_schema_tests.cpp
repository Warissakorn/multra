#include "test.hpp"
#include "routing_schema_cases.hpp"
#include "../src/project/network_schema.hpp"
#include "../src/project/demand_time_types.hpp"

using namespace multra;
namespace {
const Json& resolve(const Json& schema,const Json& ref) {
    return schema.at(Json::json_pointer(ref.at("$ref").get<std::string>().substr(1)));
}
const Json& definitionFor(const Json& schema,int version) {
    for(const auto& branch:schema.at("allOf"))
        for(const auto& v:branch.at("if").at("properties").at("schemaVersion").at("enum"))
            if(v==version)return resolve(schema,branch.at("then").at("properties").at("definition"));
    throw std::runtime_error("Missing definition schema version");
}
void unsupported(const Json& definition,const std::string& path) {
    try {(void)parseRoutingDecisions(definition);}
    catch(const ValidationError& e) {
        CHECK(e.issues[0].code=="EDIT_UNSUPPORTED_FIELD");CHECK(e.issues[0].path==path);return;
    }
    throw std::runtime_error("Expected unsupported routing field");
}
Json typeRule() {
    return {{"vehicleTypeId","car"},{"relativeFlows",Json::array({1})},
        {"intervalFlows",Json::array({Json::array({2,4})})}};
}
}
TEST(project, routing_schema_preserves_closed_rows_open_periods_and_position_floor) {
    const auto schema=projectDocumentSchema();
    for(int v=1;v<=kLatestSchema;++v) {
        const auto& d=definitionFor(schema,v);
        const auto& routing=d.at("properties").at("routingDecisions");
        CHECK(routing.at("type")=="array");
        const auto& decision=resolve(schema,routing.at("items"));
        const auto& share=resolve(schema,decision.at("properties").at("routes").at("items"));
        const auto& period=resolve(schema,decision.at("properties").at("intervals").at("items"));
        const auto& rule=resolve(schema,decision.at("properties").at("typeRules").at("items"));
        CHECK(decision.at("additionalProperties")==false);CHECK(share.at("additionalProperties")==false);
        CHECK(rule.at("additionalProperties")==false);CHECK(period.at("additionalProperties")==true);
        CHECK(decision.at("properties").at("typeRules").at("type")=="array");
        CHECK(decision.at("properties").at("position").is_boolean()==(v<20));
        if(v<20)CHECK(decision.at("properties").at("position")==false);
        CHECK(!share.at("properties").at("relativeFlow").contains("minimum"));
        CHECK(!rule.at("properties").at("relativeFlows").contains("minItems"));
    }
}
TEST(project, routing_nullable_optionals_and_always_strict_keys_keep_existing_reads) {
    auto j=routingCases::minimal();auto& x=j["definition"]["routingDecisions"][0];
    x["name"]=nullptr;x["linkId"]=nullptr;x["position"]=nullptr;x["intervals"]=nullptr;
    x["routes"][0]["destinationLinkId"]=nullptr;x["routes"][0]["intervalFlows"]=nullptr;
    CHECK(x.at("position").is_null());CHECK(x.at("routes")[0].at("intervalFlows").is_null());
    const auto parsed=parseRoutingDecisions(j.at("definition"));
    CHECK(parsed[0].name.empty());CHECK(parsed[0].linkId.empty());CHECK(!parsed[0].position);
    CHECK(parsed[0].intervals.empty());CHECK(parsed[0].routes[0].intervalFlows.empty());
    x["typeRules"]=nullptr;CHECK(x.at("typeRules").is_null());
    test::throws([&]{parseRoutingDecisions(j.at("definition"));},"Field is null: typeRules");
    x.erase("typeRules");x["extra"]=1;CHECK(x.contains("extra"));
    unsupported(j.at("definition"),"routingDecision.extra");
    x.erase("extra");x["routes"][0]["extra"]=1;CHECK(x.at("routes")[0].contains("extra"));
    unsupported(j.at("definition"),"routingDecision.routes.extra");
    x["routes"][0].erase("extra");x["typeRules"]=Json::array({typeRule()});x["typeRules"][0]["extra"]=1;
    CHECK(x.at("typeRules")[0].contains("extra"));
    unsupported(j.at("definition"),"routingDecision.typeRules.extra");
    x.erase("typeRules");x["intervals"]=Json::array({Json{{"startTime",0},{"endTime",10},{"extra",1}}});
    CHECK(x.at("intervals")[0].contains("extra"));CHECK(parseRoutingDecisions(j.at("definition"))[0].intervals.size()==1);
    j["definition"]["routingDecisions"]=nullptr;CHECK(j.at("definition").at("routingDecisions").is_null());
    test::throws([&]{parseAuthoringDefinition(j.at("definition"));},"Field is null: routingDecisions");
}
TEST(project, routing_position_precheck_is_project_only_and_rejects_legacy_null) {
    for(int v:{1,6,19,20,57})for(const auto& position:std::vector<Json>{nullptr,0}) {
        auto j=routingCases::minimal(v);auto& x=j["definition"]["routingDecisions"][0];
        x["position"]=position;x["linkId"]="link";
        CHECK(x.contains("position"));CHECK(x.at("linkId")=="link");
        const auto standalone=parseRoutingDecisions(j.at("definition"));
        CHECK(standalone[0].position.has_value()==!position.is_null());
        if(v<20)test::throws([&]{parseDocument(j);},"UNSUPPORTED_FIELD: routingDecision.position");
        else CHECK(parseDocument(j).definition->routingDecisions[0].position==standalone[0].position);
        j.erase("schemaVersion");CHECK(!j.contains("schemaVersion"));
        CHECK(parseDocument(j).definition->routingDecisions[0].position==standalone[0].position);
    }
}
TEST(project, routing_matrix_diagnostics_and_semantic_dimensions_remain_distinct) {
    auto j=routingCases::minimal();auto& x=j["definition"]["routingDecisions"][0];
    x["typeRules"]=Json::array({typeRule()});auto& rule=x["typeRules"][0];
    rule["relativeFlows"]=Json::array({"bad"});rule["intervalFlows"]=Json::array({7});
    CHECK(rule.at("relativeFlows")[0].is_string());CHECK(rule.at("intervalFlows")[0].is_number());
    test::throws([&]{parseRoutingDecisions(j.at("definition"));},"Expected numbers: relativeFlows");
    rule["relativeFlows"]=Json::array({1});CHECK(rule.at("relativeFlows")[0]==1);
    test::throws([&]{parseRoutingDecisions(j.at("definition"));},"ROUTING_DECISION_INTERVALS");
    rule["intervalFlows"]=Json::array({Json::array({"bad"})});CHECK(rule.at("intervalFlows")[0][0].is_string());
    test::throws([&]{parseRoutingDecisions(j.at("definition"));},"INVALID_SHARE");
    rule["intervalFlows"]=Json::array();CHECK(rule.at("intervalFlows").empty());
    CHECK(parseRoutingDecisions(j.at("definition"))[0].typeRules[0].intervalFlows.empty());
    const auto decoded=parseAuthoringDefinition(j.at("definition"));const auto issues=timeTypeIssues(decoded);
    CHECK(std::any_of(issues.begin(),issues.end(),[](const auto& issue){return issue.code=="ROUTING_DECISION_INTERVALS";}));
    test::throws([&]{parseDocument(j);},"ROUTING_DECISION_INTERVALS");
    x.erase("typeRules");x["routes"][0]["intervalFlows"]=Json::array({"bad"});
    CHECK(x.at("routes")[0].at("intervalFlows")[0].is_string());
    test::throws([&]{parseRoutingDecisions(j.at("definition"));},"INVALID_SHARE");
}
TEST(project, routing_counted_sync_and_structural_refusals_keep_original_stages) {
    auto j=routingCases::minimal(26);auto& input=j["definition"]["inputs"][0];
    input["volumeFromCounts"]=true;input["vehiclesPerHour"]=123;
    CHECK(input.at("volumeFromCounts")==true);CHECK(input.at("vehiclesPerHour")==123);
    CHECK(parseDefinition(j.at("definition")).inputs[0].vehiclesPerHour==123);
    const auto d=parseDocument(j);CHECK(d.definition->inputs[0].intervals.size()==2);
    test::near(d.definition->inputs[0].vehiclesPerHour,540);
    auto changed=j;changed["definition"]["routingDecisions"][0]["routes"][0]["intervalFlows"][1]=8;
    CHECK(changed.at("definition").at("routingDecisions")[0].at("routes")[0].at("intervalFlows")[1]==8);
    test::near(parseDocument(changed).definition->inputs[0].vehiclesPerHour,900);
    CHECK(!documentJson(d).at("definition").at("inputs")[0].contains("intervals"));
    for(const auto& c:routingCases::cases()) {
        if(c.schemaAccepts)continue;
        CHECK(c.document.contains("schemaVersion"));bool accepted=true;
        try {(void)parseDocument(c.document);}catch(const std::exception&) {accepted=false;}
        if(accepted)throw std::runtime_error("Codec accepted routing structural refusal: "+c.name);
    }
}
