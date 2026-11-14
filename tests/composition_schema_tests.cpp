#include "test.hpp"
#include "../src/project/latest_schema.hpp"
#include "composition_schema_cases.hpp"
#include "../src/project/network_schema.hpp"
#include "../src/project/json.hpp"

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
void unsupported(const Json& composition,const std::string& path) {
    try {(void)parseComposition(composition);}
    catch(const ValidationError& e) {
        CHECK(e.issues[0].code=="EDIT_UNSUPPORTED_FIELD");CHECK(e.issues[0].path==path);return;
    }
    throw std::runtime_error("Expected unsupported composition field");
}
Json composition() {return compositionCases::minimal().at("definition").at("compositions")[0];}
}
TEST(project, composition_reader_keeps_closed_rows_and_contains_based_optionals) {
    auto x=composition();x["extra"]=1;CHECK(x.contains("extra"));unsupported(x,"composition.extra");
    x=composition();x["types"][0]["extra"]=1;CHECK(x.at("types")[0].contains("extra"));
    unsupported(x,"composition.types.extra");
    x=composition();x["intervals"][0]["extra"]=1;CHECK(x.at("intervals")[0].contains("extra"));
    unsupported(x,"composition.intervals.extra");
    x=composition();x["intervals"][0]["types"][0]["extra"]=1;CHECK(x.at("intervals")[0].at("types")[0].contains("extra"));
    unsupported(x,"composition.intervals.types.extra");
    // The unknown key is reported before a missing id.
    unsupported(Json{{"extra",1}},"composition.extra");
    x=composition();x["name"]=nullptr;CHECK(x.at("name").is_null());
    test::throws([&]{parseComposition(x);},"Field is null: name");
    x=composition();x["intervals"]=nullptr;CHECK(x.at("intervals").is_null());
    test::throws([&]{parseComposition(x);},"Field is null: intervals");
    x["intervals"]=7;test::throws([&]{parseComposition(x);},"Expected array: intervals");
    x=composition();x["types"][0]["share"]="bad";test::throws([&]{parseComposition(x);},"Expected number: share");
    x=composition();x.erase("id");test::throws([&]{parseComposition(x);},"Missing field: id");
    test::throws([&]{parseComposition(Json(7));},"Expected an object containing: id");
    const auto parsed=parseComposition(composition());
    CHECK(parsed.id=="mix");CHECK(parsed.name=="Mix");CHECK(parsed.types.size()==1);CHECK(parsed.intervals.size()==1);
    auto j=compositionCases::minimal();j["definition"]["compositions"]=nullptr;
    CHECK(j.at("definition").at("compositions").is_null());
    test::throws([&]{parseAuthoringDefinition(j.at("definition"));},"EDIT_CATALOG_READ");
}
TEST(project, composition_schema_rows_are_closed_and_compositions_stay_optional) {
    const auto schema=projectDocumentSchema();
    for(int v=1;v<=kLatestSchema;++v) {
        const auto& d=definitionFor(schema,v);
        const auto& list=d.at("properties").at("compositions");
        CHECK(list.at("type")=="array");
        for(const auto& r:d.at("required"))CHECK(r!="compositions");
        const auto& c=resolve(schema,list.at("items"));
        const auto& s=resolve(schema,c.at("properties").at("types").at("items"));
        const auto& p=resolve(schema,c.at("properties").at("intervals").at("items"));
        const auto& ps=resolve(schema,p.at("properties").at("types").at("items"));
        for(const auto* row:{&c,&s,&p,&ps})CHECK(row->at("additionalProperties")==false);
        CHECK(c.at("required")==Json::array({"id","types"}));
        CHECK(s.at("required")==Json::array({"vehicleTypeId","share"}));
        CHECK(p.at("required")==Json::array({"startTime","endTime","types"}));
        CHECK(&s==&ps);
        CHECK(c.at("properties").at("name").at("type")=="string");
        CHECK(c.at("properties").at("intervals").at("type")=="array");
        CHECK(!s.at("properties").at("share").contains("minimum"));
    }
}
TEST(project, composition_structural_refusals_are_also_refused_by_the_codec) {
    for(int v=1;v<=kLatestSchema;++v) {
        const auto base=compositionCases::minimal(v);CHECK(base.at("schemaVersion")==v);
        const auto d=parseDocument(base);
        CHECK(!d.definition->externalCompositions);CHECK(d.definition->compositions.size()==1);
        CHECK(d.definition->compositions[0].intervals.size()==1);
    }
    for(const auto& c:compositionCases::cases()) {
        if(c.schemaAccepts)continue;
        CHECK(c.document.contains("schemaVersion"));bool accepted=true;
        try {(void)parseDocument(c.document);}catch(const std::exception&) {accepted=false;}
        if(accepted)throw std::runtime_error("Codec accepted composition structural refusal: "+c.name);
    }
}
