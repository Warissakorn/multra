#include "test.hpp"
#include "period_factor_schema_cases.hpp"
#include "../src/project/network_schema.hpp"

using namespace multra;
namespace pf=periodFactorCases;
namespace {
using Fault=std::pair<std::string,std::string>;
void refusedWith(const Json& document,const std::vector<Fault>& expected) {
    CHECK(!expected.empty());
    try {(void)parseDocument(Json::parse(document.dump()));}
    catch(const ValidationError& e) {
        CHECK(e.issues.size()==expected.size());
        for(std::size_t i=0;i<expected.size();++i) {
            CHECK(e.issues[i].code==expected[i].first);CHECK(e.issues[i].path==expected[i].second);
        }
        return;
    }
    throw std::runtime_error("Expected "+expected[0].first+" at "+expected[0].second);
}
}
TEST(project, period_factor_reader_keeps_its_floor_paths_codes_and_order) { // PF1
    for(int v=pf::from;v<=kLatestSchema;++v) {
        const auto d=parseDocument(pf::minimal(v));validateDocument(d);
        CHECK(d.periodFactors.size()==1u);CHECK(d.periodFactors[0].productionToAttraction==0.4);
    }
    auto j=pf::minimal(pf::from-1);j["periodFactors"]=nullptr;CHECK(j.contains("periodFactors"));
    refusedWith(j,{{"EDIT_UNSUPPORTED_FIELD","periodFactors"}});
    for(const auto& value:std::vector<Json>{nullptr,7,true,Json::object(),Json::array()}) {
        j=pf::minimal();j["periodFactors"]=value;CHECK(j["periodFactors"]==value);
        refusedWith(j,{{"PERIOD_FACTOR_INVALID","periodFactors"}});
    }
    for(const auto& value:std::vector<Json>{nullptr,7,Json::array()}) {
        j=pf::minimal();j["periodFactors"][0]=value;CHECK(j["periodFactors"][0]==value);
        refusedWith(j,{{"PERIOD_FACTOR_INVALID","periodFactors[0]"}});
    }
    for(const auto* key:{"purposeId","periodId","productionToAttraction","attractionToProduction"}) {
        j=pf::minimal();j["periodFactors"][0].erase(key);CHECK(!j["periodFactors"][0].contains(key));
        refusedWith(j,{{"PERIOD_FACTOR_INVALID","periodFactors[0]"}});
    }
    j=pf::minimal();auto& extra=j["periodFactors"][0];extra["purposeId"]=nullptr;extra["extra"]=1;
    CHECK(extra.size()==5);refusedWith(j,{{"PERIOD_FACTOR_INVALID","periodFactors[0]"}}); // row before text
    j=pf::minimal();j["periodFactors"][0].erase("purposeId");j["periodFactors"][0]["extra"]=1;
    CHECK(j["periodFactors"][0].size()==4);refusedWith(j,{{"PERIOD_FACTOR_INVALID","periodFactors[0]"}});
    for(const auto* key:{"purposeId","periodId"}) {
        j=pf::minimal();j["periodFactors"][0][key]=" \t\r\n";CHECK(j["periodFactors"][0][key]==" \t\r\n");
        refusedWith(j,{{"PERIOD_FACTOR_INVALID",std::string("periodFactors[0].")+key}});
    }
    for(const auto* key:{"productionToAttraction","attractionToProduction"})
        for(const auto& value:std::vector<Json>{nullptr,"0.5",true,-0.000001,1.000001}) {
            j=pf::minimal();j["periodFactors"][0][key]=value;CHECK(j["periodFactors"][0][key]==value);
            refusedWith(j,{{"PERIOD_FACTOR_INVALID",std::string("periodFactors[0].")+key}});
        }
    for(const auto& shares:std::vector<std::pair<double,double>>{{0,0},{0,1},{1,0},{0.25,0.75}}) {
        j=pf::minimal();j["periodFactors"][0]["productionToAttraction"]=shares.first;
        j["periodFactors"][0]["attractionToProduction"]=shares.second;validateDocument(parseDocument(j));
    }
    j=pf::minimal();j["periodFactors"][0]["productionToAttraction"]=0.75;j["periodFactors"][0]["attractionToProduction"]=0.5;
    CHECK(j["periodFactors"][0]["productionToAttraction"].get<double>()+j["periodFactors"][0]["attractionToProduction"].get<double>()>1);
    refusedWith(j,{{"PERIOD_FACTOR_INVALID","periodFactors[0]"}});
    // Text errors precede share errors; an invalid share suppresses the sum error.
    j=pf::minimal();auto& f=j["periodFactors"][0];f["purposeId"]=7;f["periodId"]="";
    f["productionToAttraction"]=2;f["attractionToProduction"]=nullptr;CHECK(f.size()==4);
    refusedWith(j,{{"PERIOD_FACTOR_INVALID","periodFactors[0].purposeId"},{"PERIOD_FACTOR_INVALID","periodFactors[0].periodId"},
        {"PERIOD_FACTOR_INVALID","periodFactors[0].productionToAttraction"},{"PERIOD_FACTOR_INVALID","periodFactors[0].attractionToProduction"}});
    j=pf::minimal();j["periodFactors"][0]["purposeId"]="";j["periodFactors"][0]["productionToAttraction"]=0.9;
    j["periodFactors"][0]["attractionToProduction"]=0.9;CHECK(j["periodFactors"][0]["purposeId"]=="");
    refusedWith(j,{{"PERIOD_FACTOR_INVALID","periodFactors[0].purposeId"},{"PERIOD_FACTOR_INVALID","periodFactors[0]"}});
    // parseDocument finishes with validation: UNKNOWN_PERIOD before DUPLICATE_ID, in list order.
    j=pf::minimal();j["periodFactors"][0]["periodId"]="gone";j["periodFactors"].push_back(j["periodFactors"][0]);
    CHECK(j["periodFactors"].size()==2u && j["periodFactors"][0]["periodId"]=="gone");
    refusedWith(j,{{"UNKNOWN_PERIOD","periodFactors[0].periodId"},{"UNKNOWN_PERIOD","periodFactors[1].periodId"},
        {"DUPLICATE_ID","periodFactors[1]"}});
    j=pf::minimal();j["periodFactors"].push_back(pf::factor());CHECK(j["periodFactors"].size()==2);
    refusedWith(j,{{"DUPLICATE_ID","periodFactors[1]"}});
}
TEST(project, period_factor_structural_refusals_are_also_refused_by_the_reader) { // PF3 (codec half)
    for(const auto& c:pf::cases()) {
        if(c.schemaAccepts)continue;
        CHECK(c.document.contains("schemaVersion"));bool accepted=true;
        try {(void)parseDocument(c.document);}catch(const std::exception&) {accepted=false;}
        if(accepted)throw std::runtime_error("Reader accepted period-factor structural refusal: "+c.name);
    }
}
TEST(project, period_factor_schema_floor_closed_row_and_share_bounds) { // PF2
    const auto schema=projectDocumentSchema();
    const auto resolve=[&](const Json& ref)->const Json& {return schema.at(Json::json_pointer(ref.at("$ref").get<std::string>().substr(1)));};
    for(int v=1;v<=kLatestSchema;++v) {
        const Json* root=nullptr;
        for(const auto& branch:schema.at("allOf"))
            for(const auto& x:branch.at("if").at("properties").at("schemaVersion").at("enum"))if(x==v)root=&branch.at("then").at("properties");
        CHECK(root!=nullptr);CHECK((root->at("periodFactors")==false)==(v<pf::from));
        if(v<pf::from)continue;
        const auto& list=root->at("periodFactors");CHECK(list.at("minItems")==1);CHECK(!list.contains("uniqueItems"));
        const auto& row=resolve(list.at("items"));CHECK(row.at("additionalProperties")==false);
        CHECK(row.at("required")==Json::array({"purposeId","periodId","productionToAttraction","attractionToProduction"}));
        const auto& fields=row.at("properties");CHECK(fields.size()==4u);
        for(const auto* key:{"purposeId","periodId"})CHECK(fields.at(key)==Json{{"type","string"}});
        for(const auto* key:{"productionToAttraction","attractionToProduction"})
            CHECK(fields.at(key)==Json({{"type","number"},{"minimum",0},{"maximum",1}}));
        CHECK(!row.contains("allOf")); // No sum-of-shares or reference-resolution schema extension.
    }
}
