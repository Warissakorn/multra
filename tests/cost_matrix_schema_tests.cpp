#include "test.hpp"
#include "cost_matrix_schema_cases.hpp"
#include "../src/project/network_schema.hpp"

using namespace multra;
namespace cm=costMatrixCases;
namespace {
void refusedAt(const Json& document,const std::string& code,const std::string& path) {
    try {(void)parseDocument(Json::parse(document.dump()));}
    catch(const ValidationError& e) {CHECK(e.issues[0].code==code);CHECK(e.issues[0].path==path);return;}
    throw std::runtime_error("Expected "+code+" at "+path);
}
}
TEST(project, cost_matrix_reader_keeps_its_floor_paths_codes_and_order) { // CM1
    for(int v=cm::from;v<=kLatestSchema;++v) {
        const auto d=parseDocument(cm::minimal(v));CHECK(d.costMatrices.size()==1u);
        CHECK(!d.costMatrices[0].rows[0][1]);CHECK(d.costMatrices[0].rows[1][0]==5.5);
    }
    auto j=cm::minimal(cm::from-1);j["costMatrices"]=nullptr;refusedAt(j,"EDIT_UNSUPPORTED_FIELD","costMatrices");
    for(const auto& value:std::vector<Json>{nullptr,7,Json::array()}) {
        j=cm::minimal();j["costMatrices"]=value;refusedAt(j,"INVALID_COST_MATRIX","costMatrices");
    }
    j=cm::minimal();j["costMatrices"][0]=7;refusedAt(j,"INVALID_COST_MATRIX","costMatrices[0]");
    j=cm::minimal();j["costMatrices"][0].erase("id");j["costMatrices"][0]["extra"]=1;
    refusedAt(j,"INVALID_COST_MATRIX","costMatrices[0]");
    j=cm::minimal();j["costMatrices"][0]["source"]["table"]="t";
    refusedAt(j,"INVALID_COST_MATRIX","costMatrices[0].source"); // unlike a deterrence source
    j=cm::minimal();j["costMatrices"][0]["source"]["citation"]=nullptr;
    refusedAt(j,"INVALID_COST_MATRIX","costMatrices[0].source");
    j=cm::minimal();j["costMatrices"][0]["zoneIds"][1]="a";
    refusedAt(j,"INVALID_COST_MATRIX","costMatrices[0].zoneIds[1]");
    j=cm::minimal();j["costMatrices"][0]["zoneIds"][0]=" ";
    refusedAt(j,"INVALID_COST_MATRIX","costMatrices[0].zoneIds[0]");
    j=cm::minimal();j["costMatrices"][0]["rows"].erase(1);
    refusedAt(j,"INVALID_COST_MATRIX","costMatrices[0].rows");
    j=cm::minimal();j["costMatrices"][0]["rows"][0]=Json::array({1});
    refusedAt(j,"INVALID_COST_MATRIX","costMatrices[0].rows[0]");
    j=cm::minimal();j["costMatrices"][0]["rows"][0][0]=-1;
    refusedAt(j,"INVALID_COST_MATRIX","costMatrices[0].rows[0][0]");
    j=cm::minimal();j["costMatrices"].push_back(cm::matrix());refusedAt(j,"DUPLICATE_ID","costMatrices[1].id");
    // Field order, then zone order, then row/cell order; bad cells in a missized row are skipped.
    j=cm::minimal();auto& m=j["costMatrices"][0];m["id"]=" ";m["name"]=nullptr;m["unit"]="s";
    m["source"]={{"table","t"}};m["zoneIds"][1]="a";m["rows"][0]=Json::array({-1});m["rows"][1][0]=-1;
    const std::vector<std::pair<std::string,std::string>> expected={
        {"INVALID_ID",".id"},{"INVALID_COST_MATRIX",".name"},{"INVALID_COST_MATRIX",".unit"},
        {"INVALID_COST_MATRIX",".source"},{"INVALID_COST_MATRIX",".zoneIds[1]"},
        {"INVALID_COST_MATRIX",".rows[0]"},{"INVALID_COST_MATRIX",".rows[1][0]"}};
    bool refused=false;
    try {(void)parseDocument(j);}catch(const ValidationError& e) {
        refused=true;CHECK(e.issues.size()==expected.size());
        for(std::size_t i=0;i<expected.size();++i) {
            CHECK(e.issues[i].code==expected[i].first);CHECK(e.issues[i].path=="costMatrices[0]"+expected[i].second);
        }
    }
    CHECK(refused);
}
TEST(project, cost_matrix_structural_refusals_are_also_refused_by_the_codec) { // CM3 (codec half)
    for(const auto& c:cm::cases()) {
        if(c.schemaAccepts)continue;
        CHECK(c.document.contains("schemaVersion"));bool accepted=true;
        try {(void)parseDocument(c.document);}catch(const std::exception&) {accepted=false;}
        if(accepted)throw std::runtime_error("Codec accepted cost-matrix structural refusal: "+c.name);
    }
}
TEST(project, cost_matrix_schema_floor_closed_rows_and_nullable_cells) { // CM2
    const auto schema=projectDocumentSchema();
    const auto resolve=[&](const Json& ref)->const Json& {return schema.at(Json::json_pointer(ref.at("$ref").get<std::string>().substr(1)));};
    for(int v=1;v<=kLatestSchema;++v) {
        const Json* root=nullptr;
        for(const auto& branch:schema.at("allOf"))
            for(const auto& x:branch.at("if").at("properties").at("schemaVersion").at("enum"))if(x==v)root=&branch.at("then").at("properties");
        CHECK(root!=nullptr);CHECK((root->at("costMatrices")==false)==(v<cm::from));
        if(v<cm::from)continue;
        const auto& list=root->at("costMatrices");CHECK(list.at("minItems")==1);
        const auto& m=resolve(list.at("items"));
        CHECK(m.at("additionalProperties")==false);
        CHECK(m.at("required")==Json::array({"id","name","unit","zoneIds","rows"}));
        const auto& fields=m.at("properties");CHECK(fields.at("unit").at("const")=="min");
        const auto& source=resolve(fields.at("source"));
        CHECK(source.at("additionalProperties")==false);CHECK(source.at("required").empty());
        CHECK(source.at("properties").size()==1u);CHECK(source.at("properties").at("citation").at("type")=="string");
        const auto& zones=fields.at("zoneIds");CHECK(zones.at("minItems")==1);
        CHECK(zones.at("items")==Json{{"type","string"}});CHECK(!zones.contains("uniqueItems"));
        const auto& rows=fields.at("rows");CHECK(rows.at("minItems")==1);CHECK(!rows.contains("maxItems"));
        const auto& row=rows.at("items");CHECK(row.at("type")=="array");
        CHECK(!row.contains("minItems") && !row.contains("maxItems")); // n-by-n stays with the codec
        CHECK(row.at("items").at("type")==Json::array({"number","null"}));CHECK(row.at("items").at("minimum")==0);
    }
}
