#include "test.hpp"
#include "../src/project/latest_schema.hpp"
#include "behaviour_class_schema_cases.hpp"
#include "../src/project/network_schema.hpp"

using namespace multra;
namespace cc=behaviourClassCases;
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
void refusedAt(const Json& document,const std::string& code,const std::string& path) {
    try {(void)parseDocument(document);}
    catch(const ValidationError& e) {CHECK(e.issues[0].code==code);CHECK(e.issues[0].path==path);return;}
    throw std::runtime_error("Expected "+code+" at "+path);
}
}
TEST(project, behaviour_class_reader_keeps_its_gate_paths_and_sentences) {
    for(const auto* key:{"vehicleClasses","linkBehaviourTypes"}) {
        auto j=cc::minimal(cc::from-1);j["definition"].erase(key==std::string("vehicleClasses")?"linkBehaviourTypes":"vehicleClasses");
        j["definition"][key]=nullptr;CHECK(j.at("definition").at(key).is_null());
        refusedAt(j,"EDIT_UNSUPPORTED_FIELD",std::string("definition.")+key);
    }
    auto j=cc::minimal();j["definition"]["vehicleClasses"][0]["extra"]=1;refusedAt(j,"EDIT_UNSUPPORTED_FIELD","vehicleClasses[0].extra");
    j=cc::minimal();j["definition"]["linkBehaviourTypes"][0]["extra"]=1;refusedAt(j,"EDIT_UNSUPPORTED_FIELD","linkBehaviourTypes[0].extra");
    j=cc::minimal();j["definition"]["linkBehaviourTypes"][0]["overrides"][0]["extra"]=1;
    refusedAt(j,"EDIT_UNSUPPORTED_FIELD","linkBehaviourTypes[0].overrides[0].extra");
    j=cc::minimal();j["definition"]["vehicleClasses"][0]["name"]=nullptr;test::throws([&]{parseDocument(j);},"Expected text: name");
    j=cc::minimal();j["definition"]["vehicleClasses"][0]["vehicleTypeIds"]=Json::array({7});test::throws([&]{parseDocument(j);},"Expected text: vehicleTypeIds");
    j=cc::minimal();j["definition"]["linkBehaviourTypes"]=nullptr;test::throws([&]{parseDocument(j);},"EDIT_CATALOG_READ");
    j=cc::minimal();j["definition"]["linkBehaviourTypes"][0].erase("id");test::throws([&]{parseDocument(j);},"Missing field: id");
    const auto d=parseDocument(cc::minimal());
    CHECK(d.definition->vehicleClasses.size()==1);CHECK(d.definition->linkBehaviourTypes[0].overrides.size()==1);
}
TEST(project, behaviour_class_schema_is_refused_below_21_and_closed_from_it) {
    const auto schema=projectDocumentSchema();
    for(int v=1;v<=kLatestSchema;++v) {
        const auto& p=definitionFor(schema,v).at("properties");
        for(const auto* key:{"vehicleClasses","linkBehaviourTypes"}) {
            CHECK((p.at(key)==false)==(v<cc::from));
            if(v<cc::from)continue;
            const auto& row=resolve(schema,p.at(key).at("items"));CHECK(row.at("additionalProperties")==false);
            CHECK(row.at("required").at(0)=="id");CHECK(row.at("properties").at("name").at("type")=="string");
        }
        if(v<cc::from)continue;
        CHECK(resolve(schema,p.at("vehicleClasses").at("items")).at("required")==Json::array({"id","vehicleTypeIds"}));
        const auto& t=resolve(schema,p.at("linkBehaviourTypes").at("items"));CHECK(t.at("required")==Json::array({"id"}));
        const auto& o=resolve(schema,t.at("properties").at("overrides").at("items"));
        CHECK(o.at("additionalProperties")==false);CHECK(o.at("required")==Json::array({"classId","behaviourId"}));
    }
}
TEST(project, behaviour_class_structural_refusals_are_also_refused_by_the_codec) {
    for(const auto& c:cc::cases()) {
        if(c.schemaAccepts)continue;
        CHECK(c.document.contains("schemaVersion"));bool accepted=true;
        try {(void)parseDocument(c.document);}catch(const std::exception&) {accepted=false;}
        if(accepted)throw std::runtime_error("Codec accepted behaviour-class structural refusal: "+c.name);
    }
}
