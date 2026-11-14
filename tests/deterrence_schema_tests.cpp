#include "test.hpp"
#include "deterrence_schema_cases.hpp"
#include "../src/project/network_schema.hpp"

using namespace multra;
namespace ds=deterrenceCases;
namespace {
void refusedAt(const Json& document,const std::string& code,const std::string& path) {
    try {(void)parseDocument(Json::parse(document.dump()));}
    catch(const ValidationError& e) {CHECK(e.issues[0].code==code);CHECK(e.issues[0].path==path);return;}
    throw std::runtime_error("Expected "+code+" at "+path);
}
}
TEST(project, deterrence_reader_keeps_its_floors_paths_codes_and_order) { // DS1
    for(int v=ds::from;v<=kLatestSchema;++v) {
        auto j=ds::minimal(v);const auto d=parseDocument(j);
        CHECK(d.deterrenceSets.size()==1u);CHECK(d.deterrenceSets[0].purposes[0].beta==0.1);
        for(const auto& value:std::vector<Json>{nullptr,7,"double","origin","destination","unknown"}) {
            j=ds::minimal(v);j["deterrenceSets"][0]["purposes"][0]["constraint"]=value;
            const std::string path="deterrenceSets[0].purposes[0].constraint";
            if(v<68 || (v<72 && value=="destination"))refusedAt(j,"EDIT_UNSUPPORTED_FIELD",path);
            else if(value=="double" || value=="origin" || value=="destination")
                CHECK(parseDocument(j).deterrenceSets[0].purposes[0].constraint==value.get<std::string>());
            else refusedAt(j,"INVALID_DETERRENCE_SET",path);
        }
    }
    auto j=ds::minimal(ds::from-1);j["deterrenceSets"]=nullptr;refusedAt(j,"EDIT_UNSUPPORTED_FIELD","deterrenceSets");
    j["costMatrices"]=nullptr;refusedAt(j,"EDIT_UNSUPPORTED_FIELD","deterrenceSets"); // root gate order
    for(const auto& value:std::vector<Json>{nullptr,7,Json::array()}) {
        j=ds::minimal();j["deterrenceSets"]=value;refusedAt(j,"INVALID_DETERRENCE_SET","deterrenceSets");
    }
    j=ds::minimal();j["deterrenceSets"][0]=7;refusedAt(j,"INVALID_DETERRENCE_SET","deterrenceSets[0]");
    j=ds::minimal();j["deterrenceSets"][0].erase("id");j["deterrenceSets"][0]["extra"]=1;
    refusedAt(j,"INVALID_DETERRENCE_SET","deterrenceSets[0]"); // unknown keys before fields
    for(const auto* key:{"source","purposes"}) {
        j=ds::minimal();const std::string path=std::string("deterrenceSets[0].")+key;
        if(std::string(key)=="source")j["deterrenceSets"][0][key]["extra"]=1;
        else j["deterrenceSets"][0][key][0]["extra"]=1;
        refusedAt(j,"INVALID_DETERRENCE_SET",path+(std::string(key)=="purposes"?"[0]":""));
    }
    j=ds::minimal();j["deterrenceSets"][0]["purposes"][0]["beta"]=0;
    refusedAt(j,"INVALID_DETERRENCE_SET","deterrenceSets[0].purposes[0].beta");
    j=ds::minimal();j["deterrenceSets"][0]["purposes"][0]["intrazonalShare"]=1;
    refusedAt(j,"INVALID_DETERRENCE_SET","deterrenceSets[0].purposes[0].intrazonalShare");
    j=ds::minimal();j["deterrenceSets"][0]["purposes"].push_back(j["deterrenceSets"][0]["purposes"][0]);
    refusedAt(j,"DUPLICATE_ID","deterrenceSets[0].purposes[1].id");
    j=ds::minimal();j["deterrenceSets"].push_back(ds::set());refusedAt(j,"DUPLICATE_ID","deterrenceSets[1].id");
    // The version guard precedes row checks, even with unknown keys and a missing id.
    j=ds::minimal();j["deterrenceSets"][0].erase("id");j["deterrenceSets"][0]["extra"]=1;
    j["deterrenceSets"][0]["purposes"][0]["constraint"]=nullptr;
    refusedAt(j,"EDIT_UNSUPPORTED_FIELD","deterrenceSets[0].purposes[0].constraint");
    j=ds::minimal(72);auto& s=j["deterrenceSets"][0];
    s["id"]=" ";s["name"]=nullptr;s["function"]="power";s["costUnit"]="s";
    s["source"]={{"citation",7},{"table",nullptr}};
    auto& p=s["purposes"][0];p["id"]="";p["beta"]=0;p["intrazonalShare"]=1;p["constraint"]="bad";
    const std::vector<std::pair<std::string,std::string>> expected={
        {"INVALID_ID",".id"},{"INVALID_DETERRENCE_SET",".name"},{"INVALID_DETERRENCE_SET",".function"},
        {"INVALID_DETERRENCE_SET",".costUnit"},{"INVALID_DETERRENCE_SET",".source.citation"},
        {"INVALID_DETERRENCE_SET",".source.table"},{"INVALID_ID",".purposes[0].id"},
        {"INVALID_DETERRENCE_SET",".purposes[0].beta"},{"INVALID_DETERRENCE_SET",".purposes[0].intrazonalShare"},
        {"INVALID_DETERRENCE_SET",".purposes[0].constraint"}};
    bool refused=false;
    try {(void)parseDocument(j);}catch(const ValidationError& e) {
        refused=true;CHECK(e.issues.size()==expected.size());
        for(std::size_t i=0;i<expected.size();++i) {
            CHECK(e.issues[i].code==expected[i].first);CHECK(e.issues[i].path=="deterrenceSets[0]"+expected[i].second);
        }
    }
    CHECK(refused);
}
TEST(project, deterrence_structural_refusals_are_also_refused_by_the_codec) { // DS3 (codec half)
    for(const auto& c:ds::cases()) {
        if(c.schemaAccepts)continue;
        CHECK(c.document.contains("schemaVersion"));bool accepted=true;
        try {(void)parseDocument(c.document);}catch(const std::exception&) {accepted=false;}
        if(accepted)throw std::runtime_error("Codec accepted deterrence structural refusal: "+c.name);
    }
}
TEST(project, deterrence_schema_floors_closed_rows_and_constraint_values) { // DS2
    const auto schema=projectDocumentSchema();
    const auto resolve=[&](const Json& ref)->const Json& {return schema.at(Json::json_pointer(ref.at("$ref").get<std::string>().substr(1)));};
    for(int v=1;v<=kLatestSchema;++v) {
        const Json* root=nullptr;
        for(const auto& branch:schema.at("allOf"))
            for(const auto& x:branch.at("if").at("properties").at("schemaVersion").at("enum"))if(x==v)root=&branch.at("then").at("properties");
        CHECK(root!=nullptr);CHECK((root->at("deterrenceSets")==false)==(v<ds::from));
        if(v<ds::from)continue;
        const auto& list=root->at("deterrenceSets");CHECK(list.at("minItems")==1);
        const auto& s=resolve(list.at("items"));
        CHECK(s.at("additionalProperties")==false);
        CHECK(s.at("required")==Json::array({"id","name","source","function","costUnit","purposes"}));
        const auto& fields=s.at("properties");
        CHECK(fields.at("function").at("const")=="exponential");CHECK(fields.at("costUnit").at("const")=="min");
        const auto& source=resolve(fields.at("source"));
        CHECK(source.at("additionalProperties")==false);CHECK(source.at("required").empty());
        CHECK(source.at("properties").size()==2u);
        CHECK(source.at("properties").at("citation").at("type")=="string");
        CHECK(source.at("properties").at("table").at("type")=="string");
        const auto& rates=resolve(root->at("tripRateSets").at("items"));
        CHECK(fields.at("source")==rates.at("properties").at("source")); // one source row
        CHECK(fields.at("purposes").at("minItems")==1);
        const auto& p=resolve(fields.at("purposes").at("items"));
        CHECK(p.at("additionalProperties")==false);CHECK(p.at("required")==Json::array({"id","beta"}));
        const auto& f=p.at("properties");
        CHECK(f.at("beta").at("exclusiveMinimum")==0);
        CHECK(f.at("intrazonalShare").at("minimum")==0);CHECK(f.at("intrazonalShare").at("exclusiveMaximum")==1);
        if(v<68)CHECK(f.at("constraint")==false);
        else CHECK(f.at("constraint").at("enum")==
            (v<72?Json::array({"double","origin"}):Json::array({"double","origin","destination"})));
    }
}
