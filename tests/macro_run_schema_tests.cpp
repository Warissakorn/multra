#include "test.hpp"
#include "macro_run_schema_cases.hpp"
#include "../src/project/network_schema.hpp"
#include <functional>

using namespace multra;
namespace mr=macroRunCases;
namespace {
void refusedAt(const Json& document,const std::string& code,const std::string& path) {
    try {(void)parseDocument(Json::parse(document.dump()));}
    catch(const ValidationError& e) {CHECK(e.issues[0].code==code);CHECK(e.issues[0].path==path);return;}
    throw std::runtime_error("Expected "+code+" at "+path);
}
}
TEST(project, macro_run_reader_keeps_its_floors_paths_and_codes) {
    for(int v=mr::settingsFrom;v<=kLatestSchema;++v) {
        const auto d=parseDocument(mr::minimal(v));CHECK(d.definition->macroSettings->maxIterations==200);
        CHECK(d.macroRuns.size()==(v>=mr::runsFrom?1u:0u));
    }
    auto j=mr::minimal(mr::settingsFrom-1);j["definition"]["macroSettings"]=nullptr;refusedAt(j,"EDIT_UNSUPPORTED_FIELD","definition.macroSettings");
    j=mr::minimal(mr::runsFrom-1);j["macroRuns"]=nullptr;refusedAt(j,"EDIT_UNSUPPORTED_FIELD","macroRuns");
    j=mr::minimal();j["macroRuns"]=nullptr;refusedAt(j,"INVALID_MACRO_RUN","macroRuns");
    j=mr::minimal();j["macroRuns"]=Json::array();refusedAt(j,"INVALID_MACRO_RUN","macroRuns");
    j=mr::minimal();j["macroRuns"][0]=7;refusedAt(j,"INVALID_MACRO_RUN","macroRuns[0]");
    j=mr::minimal();j["macroRuns"][0]["extra"]=1;j["macroRuns"][0].erase("id");refusedAt(j,"INVALID_MACRO_RUN","macroRuns[0].extra"); // before a missing id
    j=mr::minimal();j["macroRuns"][0]["name"]=nullptr;refusedAt(j,"INVALID_MACRO_RUN","macroRuns[0].name");
    j=mr::minimal();j["macroRuns"][0].erase("settings");refusedAt(j,"INVALID_MACRO_SETTING","macroRuns[0].settings");
    j=mr::minimal();j["macroRuns"][0]["settings"]["extra"]=1;refusedAt(j,"INVALID_MACRO_SETTING","macroRuns[0].settings.extra");
    j=mr::minimal();j["macroRuns"][0]["settings"]["targetGap"]=1;refusedAt(j,"INVALID_MACRO_SETTING","macroRuns[0].settings.targetGap");
    j=mr::minimal();j["definition"]["macroSettings"]=nullptr;refusedAt(j,"INVALID_MACRO_SETTING","definition.macroSettings");
    j=mr::minimal();j["definition"]["macroSettings"]["maxIterations"]=2.5;refusedAt(j,"INVALID_MACRO_SETTING","definition.macroSettings.maxIterations");
    j=mr::minimal();j["definition"]["macroSettings"]["maxIterations"]=-4294967295LL;refusedAt(j,"INVALID_MACRO_SETTING","definition.macroSettings.maxIterations");
    j=mr::minimal();j["macroRuns"][0]["period"]=" ";refusedAt(j,"INVALID_MACRO_RUN","macroRuns[0].period");
}
TEST(project, macro_run_structural_refusals_are_also_refused_by_the_codec) {
    for(const auto& c:mr::cases()) {
        if(c.schemaAccepts)continue;
        CHECK(c.document.contains("schemaVersion"));bool accepted=true;
        try {(void)parseDocument(c.document);}catch(const std::exception&) {accepted=false;}
        if(accepted)throw std::runtime_error("Codec accepted macro-run structural refusal: "+c.name);
    }
}
TEST(project, macro_run_schema_floors_and_closed_rows) {
    const auto schema=projectDocumentSchema();
    const auto resolve=[&](const Json& ref)->const Json& {return schema.at(Json::json_pointer(ref.at("$ref").get<std::string>().substr(1)));};
    for(int v=1;v<=kLatestSchema;++v) {
        const Json* root=nullptr;
        for(const auto& branch:schema.at("allOf"))
            for(const auto& x:branch.at("if").at("properties").at("schemaVersion").at("enum"))if(x==v)root=&branch.at("then").at("properties");
        CHECK(root!=nullptr);
        const auto& settings=resolve(root->at("definition")).at("properties").at("macroSettings");
        CHECK((settings==false)==(v<mr::settingsFrom));CHECK((root->at("macroRuns")==false)==(v<mr::runsFrom));
        if(v<mr::settingsFrom)continue;
        const auto& s=resolve(settings);
        CHECK(s.at("additionalProperties")==false);CHECK(s.at("required")==Json::array({"targetGap","maxIterations"}));
        CHECK(s.at("properties").at("targetGap").at("exclusiveMaximum")==1);CHECK(s.at("properties").at("maxIterations").at("maximum")==100000);
        if(v<mr::runsFrom)continue;
        const auto& r=resolve(root->at("macroRuns").at("items"));
        CHECK(r.at("additionalProperties")==false);CHECK(r.at("required")==Json::array({"id","name","period","settings"}));
        CHECK(&resolve(r.at("properties").at("settings"))==&s);
    }
}
