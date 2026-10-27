#include "test.hpp"
#include "signal_control_schema_cases.hpp"
#include "../src/project/network_schema.hpp"
#include "../src/model/demand/signal_control.hpp"

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
}
TEST(project, signal_control_schema_keeps_presence_and_distinct_known_key_policies) {
    const auto schema=projectDocumentSchema();
    for(int v=1;v<=kLatestSchema;++v) {
        const auto& d=definitionFor(schema,v);
        CHECK(d.at("type")==Json::array({"object","null"}));
        CHECK(d.at("required")==Json::array({"signalPrograms","duration","timeStep","routes","inputs"}));
        CHECK(d.at("additionalProperties")==true);
        const auto& c=resolve(schema,d.at("properties").at("signalControllers").at("items"));
        const auto& g=resolve(schema,c.at("properties").at("groups").at("items"));
        const auto& p=resolve(schema,d.at("properties").at("signalPrograms").at("items"));
        const auto& f=resolve(schema,p.at("properties").at("phases").at("items"));
        CHECK(c.at("additionalProperties")==false);CHECK(g.at("additionalProperties")==false);
        CHECK(p.at("additionalProperties")==true);CHECK(f.at("additionalProperties")==true);
        CHECK(c.at("properties").at("generated")== (v<54?Json(false):Json{{"type","boolean"},{"const",true}}));
        CHECK(g.at("properties").at("number").at("minimum")==std::numeric_limits<int>::min());
        CHECK(!g.at("properties").at("number").contains("x-multra-nativeType"));
        CHECK(f.at("properties").at("color").at("enum")==Json::array({"red","amber","green"}));
    }
}
TEST(project, signal_controller_decode_preserves_float_groups_null_names_and_error_order) {
    auto j=signalControlCases::minimal();auto& c=j["definition"]["signalControllers"][0];
    c["name"]=nullptr;c["groups"][0]["name"]=nullptr;c["groups"][0]["number"]=1.0;
    CHECK(c.at("groups")[0].at("number").is_number_float());
    const auto parsed=parseSignalControllers(j.at("definition"));
    CHECK(parsed[0].name.empty());CHECK(parsed[0].groups[0].name.empty());CHECK(parsed[0].groups[0].number==1);
    c["generated"]=false;c["groups"][0]["number"]=1.5;
    CHECK(c.at("generated")==false);CHECK(c.at("groups")[0].at("number")==1.5);
    test::throws([&]{parseSignalControllers(j.at("definition"));},"DUPLICATE_SIGNAL_GROUP");
    c["groups"][0]["number"]=1;
    CHECK(c.at("groups")[0].at("number")==1);
    test::throws([&]{parseSignalControllers(j.at("definition"));},"EDIT_GENERATED_INVALID");
    c["extra"]=1;CHECK(c.contains("extra"));
    try {parseSignalControllers(j.at("definition"));CHECK(false);}
    catch(const ValidationError& e) {CHECK(e.issues[0].path=="signalControllers[0].extra");}
}
TEST(project, signal_control_version_guards_and_legacy_migration_stay_in_document_stage) {
    for(int v:{1,6,12,13,53,54,57}) {
        auto j=signalControlCases::minimal(v);
        const auto d=parseDocument(j);
        CHECK(d.definition->signalControllers.size()==static_cast<std::size_t>(v<13?2:1));
        CHECK(d.definition->signalPrograms.size()==static_cast<std::size_t>(v<13?0:1));
        j["definition"]["signalControllers"][0]["generated"]=true;
        CHECK(j.at("definition").at("signalControllers")[0].at("generated")==true);
        CHECK(parseSignalControllers(j.at("definition"))[0].generated);
        if(v<54)test::throws([&]{parseDocument(j);},"EDIT_UNSUPPORTED_FIELD");
        else CHECK(parseDocument(j).definition->signalControllers[0].generated);
    }
    auto bare=signalControlCases::minimal(12);bare.erase("schemaVersion");CHECK(!bare.contains("schemaVersion"));
    CHECK(parseDocument(bare).definition->signalPrograms.size()==1);
    auto legacy=bare.at("definition");legacy["signalPrograms"][0]["extra"]=1;
    legacy["signalPrograms"][0]["phases"][0]["extra"]=1;
    CHECK(legacy.at("signalPrograms")[0].contains("extra"));
    CHECK(parseDefinition(legacy).signalPrograms.size()==1);
}
TEST(project, signal_control_schema_leaves_timing_and_reference_semantics_to_validation) {
    const auto schema=projectDocumentSchema();const auto& d=definitionFor(schema,57);
    const auto& c=resolve(schema,d.at("properties").at("signalControllers").at("items"));
    CHECK(c.at("properties").at("cycle")==Json{{"type","number"}});
    auto j=signalControlCases::minimal(57);j["definition"]["signalControllers"][0]["cycle"]=-1;
    CHECK(j.at("definition").at("signalControllers")[0].at("cycle")==-1);
    const auto decoded=parseAuthoringDefinition(j.at("definition"));
    const auto issues=signalControlIssues(Network{},decoded);
    CHECK(std::any_of(issues.begin(),issues.end(),[](const auto& issue){return issue.code=="INVALID_SIGNAL_TIMING";}));
    test::throws([&]{parseDocument(j);},"INVALID_SIGNAL_TIMING");
}
TEST(project, signal_control_structural_refusals_are_also_refused_by_native_codec) {
    for(const auto& c:signalControlCases::cases()) {
        if(c.schemaAccepts)continue;
        CHECK(c.document.contains("schemaVersion"));bool accepted=true;
        try {(void)parseDocument(c.document);}catch(const std::exception&) {accepted=false;}
        if(accepted)throw std::runtime_error("Codec accepted signal-control structural refusal: "+c.name);
    }
}
