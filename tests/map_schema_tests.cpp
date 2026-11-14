#include "test.hpp"
#include "map_schema_cases.hpp"
#include "../src/project/network_schema.hpp"
#include "../src/project/map_settings.hpp"

using namespace multra;
namespace mc=mapCases;
TEST(project, map_reader_keeps_its_floor_and_code) {
    for(int v=mc::from;v<=kLatestSchema;++v)CHECK(parseDocument(mc::minimal(v)).map->reference.projectedCrs=="EPSG:32647");
    auto j=mc::minimal(mc::from-1);j["map"]=nullptr;test::throws([&]{parseDocument(j);},"UNSUPPORTED_FIELD: map");
    for(const auto& bad:{Json(nullptr),Json(7),Json::object()}) {j=mc::minimal();j["map"]=bad;test::throws([&]{parseDocument(j);},"EDIT_MAP_INVALID");}
    j=mc::minimal();j["map"]["extra"]=1;test::throws([&]{parseDocument(j);},"EDIT_MAP_INVALID");
    j=mc::minimal();j["map"]["reference"]["extra"]=1;test::throws([&]{parseDocument(j);},"EDIT_MAP_INVALID");
    j=mc::minimal();j["map"]["reference"]["projectedCrs"]="EPSG:32747";test::throws([&]{parseDocument(j);},"EDIT_MAP_INVALID");
    j=mc::minimal();j["map"]["providerId"]=std::string(80,'a');CHECK(parseDocument(j).map->providerId.size()==80);
    j=mc::minimal();j["map"]["opacity"]=1;j["map"]["reference"]["rotationDegrees"]=-180;j["map"]["reference"]["anchorWorldX"]=1e7;
    CHECK(parseDocument(j).map->opacity==1);
}
TEST(project, map_structural_refusals_are_also_refused_by_the_codec) {
    for(const auto& c:mc::cases()) {
        if(c.schemaAccepts)continue;
        CHECK(c.document.contains("schemaVersion"));bool accepted=true;
        try {(void)parseDocument(c.document);}catch(const std::exception&) {accepted=false;}
        if(accepted)throw std::runtime_error("Codec accepted map structural refusal: "+c.name);
    }
}
TEST(project, map_schema_is_refused_below_29_with_the_codec_ranges) {
    const auto schema=projectDocumentSchema();
    const auto resolve=[&](const Json& ref)->const Json& {return schema.at(Json::json_pointer(ref.at("$ref").get<std::string>().substr(1)));};
    for(int v=1;v<=kLatestSchema;++v) {
        const Json* root=nullptr;
        for(const auto& branch:schema.at("allOf"))
            for(const auto& x:branch.at("if").at("properties").at("schemaVersion").at("enum"))if(x==v)root=&branch.at("then").at("properties");
        CHECK(root!=nullptr);
        CHECK((root->at("map")==false)==(v<mc::from));
        if(v<mc::from)continue;
        const auto& m=resolve(root->at("map"));
        CHECK(m.at("additionalProperties")==false);CHECK(m.at("required")==Json::array({"providerId","opacity","reference"}));
        CHECK(m.at("properties").at("opacity").at("maximum")==1);
        const auto& r=resolve(m.at("properties").at("reference"));
        CHECK(r.at("additionalProperties")==false);CHECK(r.at("required").size()==6);
        CHECK(r.at("properties").at("anchorWorldX").at("maximum")==mapWorldAnchorLimit);
        CHECK(r.at("properties").at("anchorLatitude").at("minimum")==-80);
    }
}
