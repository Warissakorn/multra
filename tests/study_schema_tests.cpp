#include "study_schema_cases.hpp"
#include "../src/project/network_schema.hpp"

namespace sc=studyCases;
namespace {
void refusedAt(const Json& document,const std::string& code,const std::string& path) {
    try {(void)parseDocument(document);}
    catch(const ValidationError& e) {CHECK(e.issues[0].code==code);CHECK(e.issues[0].path==path);return;}
    throw std::runtime_error("Expected "+code+" at "+path);
}
}
TEST(project, study_reader_keeps_its_floors_paths_and_codes) {
    for(int v=sc::periodFrom;v<=kLatestSchema;++v) {
        const auto d=parseDocument(sc::minimal(v));CHECK(d.zones.studyPeriods.size()==1);CHECK(d.definition->clockStart==3600);
    }
    for(int v=sc::areaFrom;v<=kLatestSchema;++v)CHECK(parseDocument(sc::area(v)).zones.studyArea.gateFlows.size()==1);
    auto j=sc::minimal(sc::periodFrom-1);j.erase("studyPeriods");j["definition"]["clockStart"]=nullptr;
    refusedAt(j,"EDIT_UNSUPPORTED_FIELD","definition.clockStart");
    j=sc::minimal(sc::periodFrom-1);j["studyPeriods"]=nullptr;refusedAt(j,"EDIT_UNSUPPORTED_FIELD","studyPeriods");
    j=sc::minimal();j["definition"]["clockStart"]=nullptr;refusedAt(j,"INVALID_NUMBER","definition.clockStart");
    j=sc::minimal();j["definition"]["clockStart"]="bad";refusedAt(j,"INVALID_NUMBER","definition.clockStart");
    j=sc::minimal();j["definition"]["clockStart"]=-1;refusedAt(j,"INVALID_NUMBER","definition.clockStart");
    for(const auto& bad:{Json(nullptr),Json::array(),Json(7)}) {j=sc::minimal();j["studyPeriods"]=bad;test::throws([&]{parseDocument(j);},"EDIT_ZONE_INVALID");}
    j=sc::minimal();j["studyPeriods"][0]["extra"]=1;test::throws([&]{parseDocument(j);},"EDIT_ZONE_INVALID");
    j=sc::area(sc::areaFrom-1);j["studyArea"]=nullptr;refusedAt(j,"EDIT_UNSUPPORTED_FIELD","studyArea");
    j=sc::area(sc::areaFrom);j["studyArea"]=nullptr;test::throws([&]{parseDocument(j);},"EDIT_STUDY_AREA_INVALID");
    j=sc::area(sc::areaFrom);j["studyArea"]["extra"]=1;test::throws([&]{parseDocument(j);},"EDIT_STUDY_AREA_INVALID");
    j=sc::area(sc::areaFrom);j["studyArea"]["gateFlows"][0]["modeId"]="car-mode";test::throws([&]{parseDocument(j);},"EDIT_STUDY_AREA_INVALID");
    j=sc::area(sc::areaFrom);j["studyArea"]["gateFlows"][0].erase("compositionId");j["studyArea"]["gateFlows"][0]["modeId"]="car-mode";
    CHECK(parseDocument(j).zones.studyArea.gateFlows[0].modeId=="car-mode");
}
TEST(project, study_structural_refusals_are_also_refused_by_the_codec) {
    for(const auto& c:sc::cases()) {
        if(c.schemaAccepts)continue;
        CHECK(c.document.contains("schemaVersion"));bool accepted=true;
        try {(void)parseDocument(c.document);}catch(const std::exception&) {accepted=false;}
        if(accepted)throw std::runtime_error("Codec accepted study structural refusal: "+c.name);
    }
}
TEST(project, study_schema_floors_and_closed_rows) {
    const auto schema=projectDocumentSchema();
    const auto resolve=[&](const Json& ref)->const Json& {return schema.at(Json::json_pointer(ref.at("$ref").get<std::string>().substr(1)));};
    for(int v=1;v<=kLatestSchema;++v) {
        const Json* root=nullptr;
        for(const auto& branch:schema.at("allOf"))
            for(const auto& x:branch.at("if").at("properties").at("schemaVersion").at("enum"))if(x==v)root=&branch.at("then").at("properties");
        CHECK(root!=nullptr);const auto& p=*root;
        const auto& clock=resolve(p.at("definition")).at("properties").at("clockStart");
        CHECK((p.at("studyPeriods")==false)==(v<sc::periodFrom));CHECK((clock==false)==(v<sc::periodFrom));
        CHECK((p.at("studyArea")==false)==(v<sc::areaFrom));
        if(v>=sc::periodFrom) {
            CHECK(clock.at("type")=="number");CHECK(p.at("studyPeriods").at("minItems")==1);
            const auto& row=resolve(p.at("studyPeriods").at("items"));
            CHECK(row.at("additionalProperties")==false);CHECK(row.at("required")==Json::array({"id","name","start","end"}));
        }
        if(v<sc::areaFrom)continue;
        const auto& a=resolve(p.at("studyArea"));
        CHECK(a.at("additionalProperties")==false);CHECK(a.at("required")==Json::array({"nodes","flowSource"}));
        CHECK(a.at("properties").at("nodes").at("minItems")==1);CHECK(a.at("properties").at("flowSource").at("enum").size()==3);
        const auto& f=resolve(a.at("properties").at("gateFlows").at("items"));
        CHECK(f.at("additionalProperties")==false);CHECK(f.at("required")==Json::array({"periodId","from","to","vehiclesPerHour"}));
        CHECK(f.at("allOf").at(0).at("if").at("required")==Json::array({"modeId"}));
    }
}
