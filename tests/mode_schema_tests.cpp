#include "test.hpp"
#include "../src/project/latest_schema.hpp"
#include "mode_schema_cases.hpp"
#include "../src/project/network_schema.hpp"
#include "../src/project/mode_codec.hpp"

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
// The reader's only diagnostic is the bare code, never a field sentence.
void catalogRead(const Json& mode) {
    try {(void)parseMode(mode);}
    catch(const std::invalid_argument& e) {CHECK(std::string(e.what())=="EDIT_CATALOG_READ");return;}
    throw std::runtime_error("Expected EDIT_CATALOG_READ");
}
void unsupportedModes(const Json& document) {
    try {(void)parseDocument(document);}
    catch(const ValidationError& e) {
        CHECK(e.issues[0].code=="EDIT_UNSUPPORTED_FIELD");CHECK(e.issues[0].path=="definition.modes");return;
    }
    throw std::runtime_error("Expected definition.modes to be refused");
}
}
TEST(project, mode_reader_keeps_its_exact_keys_and_bare_catalog_code) {
    const auto good=modeCases::mode();CHECK(parseMode(good).compositionId=="mix");
    auto two=good;two.erase("name");CHECK(two.size()==2);catalogRead(two);
    auto four=good;four["extra"]=1;CHECK(four.size()==4);catalogRead(four);
    auto swapped=two;swapped["other"]="x";CHECK(swapped.size()==3);catalogRead(swapped);
    auto number=good;number["name"]=7;CHECK(number.at("name").is_number());catalogRead(number);
    auto null=good;null["compositionId"]=nullptr;CHECK(null.at("compositionId").is_null());catalogRead(null);
    catalogRead(Json(7));catalogRead(Json::array({good}));
    auto j=modeCases::minimal(modeCases::modeFloor);j["definition"]["modes"]=nullptr;
    CHECK(j.at("definition").at("modes").is_null());
    test::throws([&]{parseAuthoringDefinition(j.at("definition"));},"EDIT_CATALOG_READ");
    for(const auto& modes:std::vector<Json>{Json::array({good}),nullptr,Json::array()}) {
        auto old=modeCases::minimal(modeCases::modeFloor-1);old["definition"]["modes"]=modes;
        CHECK(old.at("schemaVersion")==48);unsupportedModes(old);
    }
}
TEST(project, mode_schema_is_refused_below_49_and_a_closed_row_from_it) {
    const auto schema=projectDocumentSchema();
    for(int v=1;v<=kLatestSchema;++v) {
        const auto& d=definitionFor(schema,v);const auto& modes=d.at("properties").at("modes");
        for(const auto& r:d.at("required"))CHECK(r!="modes");
        if(v<modeCases::modeFloor) {CHECK(modes==false);continue;}
        CHECK(modes.at("type")=="array");
        const auto& row=resolve(schema,modes.at("items"));
        CHECK(row.at("additionalProperties")==false);
        CHECK(row.at("required")==Json::array({"id","name","compositionId"}));
        for(const auto* key:{"id","name","compositionId"})CHECK(row.at("properties").at(key).at("type")=="string");
    }
}
TEST(project, mode_structural_refusals_are_also_refused_by_the_codec) {
    for(int v=1;v<=kLatestSchema;++v) {
        const auto base=modeCases::minimal(v);CHECK(base.at("definition").contains("modes"));
        if(v<modeCases::modeFloor) {unsupportedModes(base);continue;}
        const auto d=parseDocument(base);
        CHECK(!d.definition->externalModes);CHECK(d.definition->modes.size()==1);
    }
    for(const auto& c:modeCases::cases()) {
        if(c.schemaAccepts)continue;
        CHECK(c.document.contains("schemaVersion"));bool accepted=true;
        try {(void)parseDocument(c.document);}catch(const std::exception&) {accepted=false;}
        if(accepted)throw std::runtime_error("Codec accepted mode structural refusal: "+c.name);
    }
}
