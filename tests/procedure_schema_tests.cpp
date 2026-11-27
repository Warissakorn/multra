#include "test.hpp"
#include "schema_refusal.hpp"
#include "procedure_schema_cases.hpp"
#include "../src/project/network_schema.hpp"

using namespace multra;
namespace pc=procedureCases;
using schemaRefusal::refusedWith;
using schemaRefusal::Fault;
namespace {
constexpr auto bad="INVALID_PROCEDURE";
const std::string pp="procedures[0]",sp=pp+".steps",fp=pp+".feedback";
Json withFeedback(int version,const Json& feedback) {auto j=pc::minimal(version);j["procedures"][0]["feedback"]=feedback;return j;}
}
TEST(project, procedure_reader_keeps_floors_and_the_post_parse_feedback_guard) { // PRS1
    for(int v=pc::from;v<=kLatestSchema;++v) {
        const auto d=parseDocument(pc::minimal(v));CHECK(d.procedures.size()==1u);CHECK(d.procedures[0].steps.size()==4u);
        auto j=withFeedback(v,pc::feedback());
        if(v<pc::feedbackFrom)refusedWith(j,{{"EDIT_UNSUPPORTED_FIELD",fp}});
        else CHECK(parseDocument(j).procedures[0].feedback.has_value());
        // Malformed feedback below 75 fails the row check first: the guard runs after parsing.
        j=withFeedback(v,Json{{"method","x"},{"maxIterations",10},{"tolerance",0}});CHECK(j["procedures"][0].contains("feedback"));
        refusedWith(j,{{bad,fp+".method"}});
        j=withFeedback(v,nullptr);CHECK(j["procedures"][0]["feedback"].is_null());refusedWith(j,{{bad,fp}});
    }
    for(const int v:{72,1,31}) {
        auto j=pc::minimal(v);j["procedures"]=nullptr;CHECK(j.contains("procedures"));
        refusedWith(j,{{"EDIT_UNSUPPORTED_FIELD","procedures"}});
    }
    for(const auto& value:std::vector<Json>{nullptr,7,Json::object(),Json::array()}) {
        auto j=pc::minimal();j["procedures"]=value;CHECK(j["procedures"]==value);refusedWith(j,{{bad,"procedures"}});
    }
}
TEST(project, procedure_reader_checks_kind_then_active_then_keys) { // PRS1
    auto j=pc::minimal();auto& s=j["procedures"][0]["steps"][0];
    s["kind"]="nope";s["active"]=7;s["extra"]=1;CHECK(s.size()==4u);refusedWith(j,{{bad,sp+"[0].kind"}});
    j=pc::minimal();j["procedures"][0]["steps"][0].erase("kind");refusedWith(j,{{bad,sp+"[0]"}});
    j=pc::minimal();j["procedures"][0]["steps"][0]["kind"]=7;refusedWith(j,{{bad,sp+"[0]"}});
    j=pc::minimal();j["procedures"][0]["steps"][0]["active"]=7;j["procedures"][0]["steps"][0]["extra"]=1;
    refusedWith(j,{{bad,sp+"[0].active"},{bad,sp+"[0]"}});
    j=pc::minimal();j["procedures"][0]["steps"][0]["active"]=false;CHECK(!parseDocument(j).procedures[0].steps[0].active);
    // Each row's keys, and another kind's key on it.
    for(const auto& [k,key]:std::vector<std::pair<int,const char*>>{{0,"period"},{1,"rateSet"},{2,"skimMode"},{3,"modeChoiceSet"}}) {
        j=pc::minimal();j["procedures"][0]["steps"][k][key]="x";CHECK(j["procedures"][0]["steps"][k].contains(key));
        refusedWith(j,{{bad,sp+"["+std::to_string(k)+"]"}});
    }
    j=pc::minimal();j["procedures"][0]["steps"][1].erase("skimMode");j["procedures"][0]["steps"][1].erase("costMatrix");
    j["procedures"][0]["steps"][2].erase("periods");CHECK(parseDocument(j).procedures[0].steps[2].periods.empty());
}
TEST(project, procedure_reader_keeps_assignment_exclusive_and_ids) { // PRS1
    auto j=pc::minimal();auto& a=j["procedures"][0]["steps"][3];
    a.erase("period");a["macroRun"]="run";CHECK(parseDocument(j).procedures[0].steps[3].macroRun=="run");
    a.erase("macroRun");CHECK(a.size()==1u);refusedWith(j,{{bad,sp+"[3]"}});
    a["period"]="am";a["macroRun"]="run";refusedWith(j,{{bad,sp+"[3]"}});
    a.erase("macroRun");a["period"]=" ";refusedWith(j,{{bad,sp+"[3].period"}});
    j=pc::minimal();j["procedures"][0].erase("id");refusedWith(j,{{"INVALID_ID",pp+".id"}});
    j=pc::minimal();j["procedures"][0]["id"]=7;refusedWith(j,{{"INVALID_ID",pp+".id"}});
    j=pc::minimal();j["procedures"][0]["id"]=" ";refusedWith(j,{{"INVALID_ID",pp+".id"}});
    j=pc::minimal();j["procedures"].push_back(pc::procedure());refusedWith(j,{{"DUPLICATE_ID","procedures[1].id"}});
    j=pc::minimal();j["procedures"][0]["steps"]=Json::array();refusedWith(j,{{bad,pp+".steps"}});
    j=pc::minimal();j["procedures"][0]["extra"]=1;refusedWith(j,{{bad,pp}});
}
TEST(project, procedure_reader_keeps_feedback_bounds) { // PRS1
    const int v=pc::feedbackFrom;
    auto f=pc::feedback();f["weight"]=0.5;refusedWith(withFeedback(v,f),{{bad,fp+".weight"}});
    f=pc::feedback();f["method"]="constantWeight";refusedWith(withFeedback(v,f),{{bad,fp+".weight"}});
    for(const auto& [w,ok]:std::vector<std::pair<Json,bool>>{{0,true},{0.999,true},{1,false},{-0.1,false}}) {
        f=pc::feedback();f["method"]="constantWeight";f["weight"]=w;
        if(ok)CHECK(parseDocument(withFeedback(v,f)).procedures[0].feedback->weight==w.get<double>());
        else refusedWith(withFeedback(v,f),{{bad,fp+".weight"}});
    }
    f=pc::feedback();f["weight"]="0.5";refusedWith(withFeedback(v,f),{{bad,fp+".weight"}});
    for(const auto& [n,ok]:std::vector<std::pair<Json,bool>>{{2,true},{1000,true},{1,false},{1001,false},{-5,false},
                                                             {2.0,false},{Json(std::uint64_t{18446744073709551615ULL}),false}}) {
        f=pc::feedback();f["maxIterations"]=n;CHECK(f["maxIterations"]==n);
        if(ok)CHECK(parseDocument(withFeedback(v,f)).procedures[0].feedback->maxIterations==n.get<int>());
        else refusedWith(withFeedback(v,f),{{bad,fp+".maxIterations"}});
    }
    for(const auto& [t,ok]:std::vector<std::pair<Json,bool>>{{0,true},{5,true},{-0.001,false},{"0",false}}) {
        f=pc::feedback();f["tolerance"]=t;
        if(ok)CHECK(parseDocument(withFeedback(v,f)).procedures[0].feedback.has_value());
        else refusedWith(withFeedback(v,f),{{bad,fp+".tolerance"}});
    }
    f=pc::feedback();f["extra"]=1;refusedWith(withFeedback(v,f),{{bad,fp}});
    f=pc::feedback();f.erase("method");refusedWith(withFeedback(v,f),{{bad,fp+".method"}});
}
TEST(project, procedure_reader_keeps_a_complete_issue_order) { // PRS1
    auto j=pc::minimal(pc::feedbackFrom);auto& p=j["procedures"][0];
    p["steps"][0]["active"]="no";p["steps"][1]["deterrenceSet"]=" ";p["steps"][1]["skimMode"]=7;
    p["steps"][2]["periods"]=Json::array({"am",7});p["steps"][3]["macroRun"]="run";
    p["feedback"]=Json{{"method","x"},{"weight","w"},{"extra",1}};
    auto q=pc::procedure();q["name"]=7;q["steps"]=Json::array();j["procedures"].push_back(q);
    refusedWith(j,{{bad,sp+"[0].active"},{bad,sp+"[1].deterrenceSet"},{bad,sp+"[1].skimMode"},{bad,sp+"[2].periods[1]"},
        {bad,sp+"[3]"},{bad,fp},{bad,fp+".method"},{bad,fp+".weight"},{bad,fp+".maxIterations"},{bad,fp+".tolerance"},
        {bad,"procedures[1].name"},{bad,"procedures[1].steps"}});
}
TEST(project, procedure_schema_refusals_fail_parse_document) { // PRS3
    std::size_t refused=0;
    for(const auto& c:pc::cases()) {
        if(c.schemaAccepts)continue;
        ++refused;bool threw=false;
        try {(void)parseDocument(Json::parse(c.document.dump()));} catch(const ValidationError&) {threw=true;}
        if(!threw)throw std::runtime_error("Schema-refused case parsed: "+c.name);
    }
    CHECK(refused>100u);
}
TEST(project, procedure_schema_floors_closed_rows_and_exclusive_assignment) { // PRS2
    const auto schema=projectDocumentSchema();
    const auto resolve=[&](const Json& ref)->const Json& {return schema.at(Json::json_pointer(ref.at("$ref").get<std::string>().substr(1)));};
    for(int v=1;v<=kLatestSchema;++v) {
        const Json* root=nullptr;
        for(const auto& branch:schema.at("allOf"))
            for(const auto& x:branch.at("if").at("properties").at("schemaVersion").at("enum"))if(x==v)root=&branch.at("then").at("properties");
        CHECK(root!=nullptr);CHECK((root->at("procedures")==false)==(v<pc::from));
        if(v<pc::from)continue;
        const auto& list=root->at("procedures");CHECK(list.at("minItems")==1);
        const auto& p=resolve(list.at("items"));CHECK(p.at("additionalProperties")==false);
        CHECK(p.at("required")==Json::array({"id","name","steps"}));
        const auto& steps=p.at("properties").at("steps");CHECK(steps.at("minItems")==1);
        // tripGeneration → tripDistribution → modeChoice, else the assignment row.
        const Json* branch=&steps.at("items");
        for(const auto* kind:{"tripGeneration","tripDistribution","modeChoice"}) {
            CHECK(branch->at("if").at("properties").at("kind").at("const")==kind);
            const auto& row=resolve(branch->at("then"));CHECK(row.at("additionalProperties")==false);
            CHECK(row.at("properties").at("kind").at("enum").size()==4u);CHECK(row.at("properties").at("active")==Json{{"type","boolean"}});
            branch=&branch->at("else");
        }
        const auto& a=resolve(*branch);CHECK(a.at("required")==Json::array({"kind"}));
        CHECK(a.at("allOf")==Json::array({Json{{"if",{{"required",Json::array({"macroRun"})}}},
            {"then",{{"properties",{{"period",false}}}}},{"else",{{"required",Json::array({"period"})}}}}}));
        if(v<pc::feedbackFrom) {CHECK(p.at("properties").at("feedback")==false);continue;}
        const auto& f=resolve(p.at("properties").at("feedback"));CHECK(f.at("additionalProperties")==false);
        CHECK(f.at("required")==Json::array({"method","maxIterations","tolerance"}));
        const auto& ff=f.at("properties");
        CHECK(ff.at("method").at("enum")==Json::array({"msa","constantWeight"}));
        CHECK(ff.at("maxIterations").at("minimum")==2);CHECK(ff.at("maxIterations").at("maximum")==1000);
        CHECK(ff.at("maxIterations").at("x-multra-nativeType")=="integer-token");
        CHECK(ff.at("weight").at("minimum")==0);CHECK(ff.at("weight").at("exclusiveMaximum")==1);CHECK(ff.at("tolerance").at("minimum")==0);
        CHECK(f.at("allOf").size()==1u);CHECK(f.at("allOf")[0].at("then")==Json{{"required",Json::array({"weight"})}});
    }
}
