#include "test.hpp"
#include "mode_choice_schema_cases.hpp"
#include "../src/project/network_schema.hpp"

using namespace multra;
namespace mc=modeChoiceCases;
namespace {
using Fault=std::pair<std::string,std::string>;
void refusedWith(const Json& j,const std::vector<Fault>& expected) {
    CHECK(!expected.empty());
    try {(void)parseDocument(Json::parse(j.dump()));}
    catch(const ValidationError& e) {
        CHECK(e.issues.size()==expected.size());
        for(std::size_t i=0;i<expected.size();++i) {
            CHECK(e.issues[i].code==expected[i].first);CHECK(e.issues[i].path==expected[i].second);
        }
        return;
    }
    throw std::runtime_error("Expected "+expected[0].first+" at "+expected[0].second);
}
constexpr auto bad="MODE_CHOICE_INVALID_SET";
const std::string sp="modeChoiceSets[0]",pp=sp+".purposes[0]",ap=pp+".alternatives[1]",np=pp+".nests[0]";
}
TEST(project, mode_choice_reader_keeps_floors_paths_and_inclusive_cost_sources) { // MCS1
    for(int v=mc::from;v<=kLatestSchema;++v) {
        auto j=mc::minimal(v);CHECK(parseDocument(j).modeChoiceSets.size()==1u);
        auto& a=j["modeChoiceSets"][0]["purposes"][0]["alternatives"][1];
        a["modeId"]="car";CHECK(a.contains("costMatrix") && a.contains("modeId"));
        CHECK(parseDocument(j).modeChoiceSets[0].purposes[0].alternatives[1].modeId=="car");
        a.erase("costMatrix");CHECK(parseDocument(j).modeChoiceSets[0].purposes[0].alternatives[1].modeId=="car");
        for(const auto* key:{"nests","distributionCost"}) {
            j=mc::minimal(v);j["modeChoiceSets"][0]["purposes"][0][key]=nullptr;
            CHECK(j["modeChoiceSets"][0]["purposes"][0].contains(key));
            const int floor=std::string(key)=="nests"?71:76;
            refusedWith(j,{{v<floor?"EDIT_UNSUPPORTED_FIELD":bad,pp+"."+key}});
        }
        if(v>=71) {
            j=mc::minimal(v);j["modeChoiceSets"][0]["purposes"][0]["nests"]=Json::array({mc::nest()});
            CHECK(parseDocument(j).modeChoiceSets[0].purposes[0].nests.size()==1u);
        }
        if(v>=76) {
            j=mc::minimal(v);j["modeChoiceSets"][0]["purposes"][0]["distributionCost"]="logsum";
            CHECK(parseDocument(j).modeChoiceSets[0].purposes[0].distributionCost=="logsum");
        }
    }
    auto j=mc::minimal(69);j["modeChoiceSets"]=nullptr;CHECK(j.contains("modeChoiceSets"));
    refusedWith(j,{{"EDIT_UNSUPPORTED_FIELD","modeChoiceSets"}});
    for(const auto& value:std::vector<Json>{nullptr,7,Json::array()}) {
        j=mc::minimal();j["modeChoiceSets"]=value;CHECK(j["modeChoiceSets"]==value);
        refusedWith(j,{{bad,"modeChoiceSets"}});
    }
    j=mc::minimal();j["modeChoiceSets"][0].erase("id");j["modeChoiceSets"][0]["extra"]=1;
    CHECK(j["modeChoiceSets"][0].contains("extra"));refusedWith(j,{{bad,sp}});
    j=mc::minimal();j["modeChoiceSets"][0]["source"]["extra"]=1;
    CHECK(j["modeChoiceSets"][0]["source"].size()==3);refusedWith(j,{{bad,sp+".source"}});
    j=mc::minimal();j["modeChoiceSets"][0]["purposes"][0]["extra"]=1;
    CHECK(j["modeChoiceSets"][0]["purposes"][0].contains("extra"));refusedWith(j,{{bad,pp}});
    j=mc::minimal();auto& a=j["modeChoiceSets"][0]["purposes"][0]["alternatives"][1];a.erase("id");a["extra"]=1;
    CHECK(a.contains("extra"));refusedWith(j,{{bad,ap}});
    j=mc::minimal();auto& x=j["modeChoiceSets"][0]["purposes"][0]["alternatives"][1];x.erase("costMatrix");
    CHECK(!x.contains("costMatrix") && !x.contains("modeId"));refusedWith(j,{{bad,ap}});
    j=mc::minimal();j["modeChoiceSets"][0]["purposes"][0]["alternatives"][1]["modeId"]=nullptr;
    CHECK(j["modeChoiceSets"][0]["purposes"][0]["alternatives"][1].contains("costMatrix"));refusedWith(j,{{bad,ap+".modeId"}});
    // Presence guards precede row checks; nests is checked before distributionCost.
    for(const int version:{70,71,76}) {
        j=mc::minimal(version);auto& p=j["modeChoiceSets"][0]["purposes"][0];p["nests"]=nullptr;p["distributionCost"]=nullptr;
        CHECK(p.contains("nests") && p.contains("distributionCost"));
        if(version==70)refusedWith(j,{{"EDIT_UNSUPPORTED_FIELD",pp+".nests"}});
        else if(version==71)refusedWith(j,{{"EDIT_UNSUPPORTED_FIELD",pp+".distributionCost"}});
        else refusedWith(j,{{bad,pp+".nests"},{bad,pp+".distributionCost"}});
    }
    j=mc::minimal(70);j["modeChoiceSets"][0].erase("id");j["modeChoiceSets"][0]["extra"]=1;
    j["modeChoiceSets"][0]["purposes"][0]["nests"]=nullptr;
    CHECK(!j["modeChoiceSets"][0].contains("id"));refusedWith(j,{{"EDIT_UNSUPPORTED_FIELD",pp+".nests"}});
}
TEST(project, mode_choice_reader_keeps_semantic_refusals_and_complete_issue_order) { // MCS1
    auto j=mc::minimal();auto& p=j["modeChoiceSets"][0]["purposes"][0];p["alternatives"][0]["constant"]=1;
    CHECK(p["alternatives"][0]["constant"]!=0 && p["alternatives"][1]["constant"]!=0);refusedWith(j,{{bad,pp}});
    j=mc::minimal();j["modeChoiceSets"].push_back(mc::set());CHECK(j["modeChoiceSets"].size()==2);
    refusedWith(j,{{"DUPLICATE_ID","modeChoiceSets[1].id"}});
    j=mc::minimal();j["modeChoiceSets"][0]["purposes"].push_back(j["modeChoiceSets"][0]["purposes"][0]);
    CHECK(j["modeChoiceSets"][0]["purposes"].size()==2);refusedWith(j,{{"DUPLICATE_ID",sp+".purposes[1].id"}});
    j=mc::minimal();j["modeChoiceSets"][0]["purposes"][0]["alternatives"][1]["id"]="car";
    CHECK(j["modeChoiceSets"][0]["purposes"][0]["alternatives"][0]["id"]=="car");refusedWith(j,{{"DUPLICATE_ID",ap+".id"}});
    j=mc::minimal(71);auto n=mc::nest();n["extra"]=1;j["modeChoiceSets"][0]["purposes"][0]["nests"]=Json::array({n});
    CHECK(n.contains("extra"));refusedWith(j,{{bad,np}});
    for(const auto& value:std::vector<Json>{0,1.001}) {
        j=mc::minimal(71);n=mc::nest();n["phi"]=value;j["modeChoiceSets"][0]["purposes"][0]["nests"]=Json::array({n});
        CHECK(n["phi"]==value);refusedWith(j,{{bad,np+".phi"}});
    }
    j=mc::minimal(71);n=mc::nest();n["alternatives"][0]="missing";
    j["modeChoiceSets"][0]["purposes"][0]["nests"]=Json::array({n});CHECK(n["alternatives"][0]=="missing");
    refusedWith(j,{{bad,np+".alternatives[0]"}});
    j=mc::minimal(71);n=mc::nest();n["alternatives"]=Json::array({"car"});auto second=n;second["id"]="other";
    j["modeChoiceSets"][0]["purposes"][0]["nests"]=Json::array({n,second});CHECK(n["alternatives"]==second["alternatives"]);
    refusedWith(j,{{"DUPLICATE_ID",pp+".nests[1].alternatives[0]"}});
    second["id"]="motor";second["alternatives"]=Json::array({"bus"});
    j["modeChoiceSets"][0]["purposes"][0]["nests"]=Json::array({n,second});CHECK(n["id"]==second["id"]);
    refusedWith(j,{{"DUPLICATE_ID",pp+".nests[1].id"}});
    for(const bool zero:{false,true}) {
        j=mc::minimal(76);auto& q=j["modeChoiceSets"][0]["purposes"][0];q["distributionCost"]="logsum";
        if(zero)for(auto& a:q["alternatives"])a["timeCoefficient"]=0;
        else q["alternatives"][1]["timeCoefficient"]=-0.2;
        CHECK(q["distributionCost"]=="logsum");refusedWith(j,{{bad,pp+".distributionCost"}});
    }
    // Set fields, purpose id, alternative fields, zero reference, nests, then logsum.
    j=mc::minimal(76);auto& s=j["modeChoiceSets"][0];s["id"]="";s["name"]=7;s["costUnit"]="s";
    s["source"]={{"citation",7},{"table",7}};auto& q=s["purposes"][0];q["id"]="";
    auto& a=q["alternatives"][0];a["id"]="";a["modeId"]=nullptr;a["costMatrix"]=7;
    a["constant"]=1;a["timeCoefficient"]=1;a["occupancy"]=0;
    n=mc::nest();n["phi"]=0;n["alternatives"]=Json::array({0,"missing","bus","bus"});
    q["nests"]=Json::array({n});q["distributionCost"]="other";CHECK(q["alternatives"][0]["constant"]==1);
    const auto first=pp+".alternatives[0]";
    refusedWith(j,{{"INVALID_ID",sp+".id"},{bad,sp+".name"},{bad,sp+".costUnit"},{bad,sp+".source.citation"},{bad,sp+".source.table"},
        {"INVALID_ID",pp+".id"},{"INVALID_ID",first+".id"},{bad,first+".modeId"},{bad,first+".costMatrix"},
        {bad,first+".timeCoefficient"},{bad,first+".occupancy"},{bad,pp},{bad,np+".phi"},
        {bad,np+".alternatives[0]"},{bad,np+".alternatives[1]"},{"DUPLICATE_ID",np+".alternatives[3]"},{bad,pp+".distributionCost"}});
}
TEST(project, mode_choice_structural_refusals_are_also_refused_by_the_codec) { // MCS3 (codec half)
    for(const auto& c:mc::cases()) {
        if(c.schemaAccepts)continue;
        CHECK(c.document.contains("schemaVersion"));bool accepted=true;
        try {(void)parseDocument(c.document);}catch(const std::exception&) {accepted=false;}
        if(accepted)throw std::runtime_error("Codec accepted mode-choice structural refusal: "+c.name);
    }
}
TEST(project, mode_choice_schema_floors_closed_rows_and_inclusive_cost_sources) { // MCS2
    const auto schema=projectDocumentSchema();
    const auto resolve=[&](const Json& ref)->const Json& {return schema.at(Json::json_pointer(ref.at("$ref").get<std::string>().substr(1)));};
    for(int v=1;v<=kLatestSchema;++v) {
        const Json* root=nullptr;
        for(const auto& branch:schema.at("allOf"))
            for(const auto& x:branch.at("if").at("properties").at("schemaVersion").at("enum"))if(x==v)root=&branch.at("then").at("properties");
        CHECK(root!=nullptr);CHECK((root->at("modeChoiceSets")==false)==(v<mc::from));
        if(v<mc::from)continue;
        const auto& list=root->at("modeChoiceSets");CHECK(list.at("minItems")==1);
        const auto& s=resolve(list.at("items"));CHECK(s.at("additionalProperties")==false);
        CHECK(s.at("required")==Json::array({"id","name","source","costUnit","purposes"}));
        const auto& f=s.at("properties");CHECK(f.at("costUnit").at("const")=="min");
        CHECK(f.at("source")==resolve(root->at("tripRateSets").at("items")).at("properties").at("source"));
        const auto& source=resolve(f.at("source"));CHECK(source.at("required").empty());CHECK(source.at("additionalProperties")==false);
        CHECK(f.at("purposes").at("minItems")==1);
        const auto& p=resolve(f.at("purposes").at("items"));CHECK(p.at("additionalProperties")==false);
        CHECK(p.at("required")==Json::array({"id","alternatives"}));const auto& pf=p.at("properties");
        CHECK(pf.at("alternatives").at("minItems")==1);
        const auto& a=resolve(pf.at("alternatives").at("items"));CHECK(a.at("additionalProperties")==false);
        CHECK(a.at("required")==Json::array({"id","constant","timeCoefficient"}));
        CHECK(a.at("anyOf")==Json::array({Json{{"required",Json::array({"modeId"})}},Json{{"required",Json::array({"costMatrix"})}}}));
        CHECK(!a.contains("oneOf") && !a.contains("allOf"));const auto& af=a.at("properties");
        for(const auto* key:{"id","modeId","costMatrix"})CHECK(af.at(key)==Json{{"type","string"}});
        CHECK(af.at("constant")==Json{{"type","number"}});CHECK(af.at("timeCoefficient").at("maximum")==0);
        CHECK(af.at("occupancy").at("minimum")==1);
        if(v<71)CHECK(pf.at("nests")==false);
        else {
            CHECK(pf.at("nests").at("minItems")==1);const auto& n=resolve(pf.at("nests").at("items"));
            CHECK(n.at("additionalProperties")==false);CHECK(n.at("required")==Json::array({"id","phi","alternatives"}));
            CHECK(n.at("properties").at("phi").at("exclusiveMinimum")==0);CHECK(n.at("properties").at("phi").at("maximum")==1);
            const auto& members=n.at("properties").at("alternatives");CHECK(members.at("minItems")==1);
            CHECK(members.at("items")==Json{{"type","string"}});CHECK(!members.contains("uniqueItems"));
        }
        if(v<76)CHECK(pf.at("distributionCost")==false);else CHECK(pf.at("distributionCost").at("const")=="logsum");
    }
}
