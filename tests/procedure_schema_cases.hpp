#pragma once
#include "demand_schema_cases.hpp"
#include <functional>

// D636: structural labels only. Blank ids, unique ids, known references and the run's step rules
// remain codec or run checks, labelled accepted here. Feedback (75) is refused after parsing.
namespace procedureCases {
struct Case {std::string name;multra::Json document;bool schemaAccepts;};
inline constexpr int from=73,feedbackFrom=75;
inline multra::Json generation() {return {{"kind","tripGeneration"},{"rateSet","rates"}};}
inline multra::Json distribution() {return {{"kind","tripDistribution"},{"deterrenceSet","gravity"},{"skimMode","car"},{"costMatrix","times"}};}
inline multra::Json modeChoice() {return {{"kind","modeChoice"},{"modeChoiceSet","mnl"},{"periods",multra::Json::array({"am"})}};}
inline multra::Json assignment() {return {{"kind","assignment"},{"period","am"}};}
inline multra::Json procedure() {
    return {{"id","four-step"},{"name","Four step"},{"steps",multra::Json::array({generation(),distribution(),modeChoice(),assignment()})}};
}
inline multra::Json feedback() {return {{"method","msa"},{"maxIterations",10},{"tolerance",0.01}};}
inline multra::Json minimal(int version=from) {
    auto j=demandCases::minimal(version);
    if(version>=from)j["procedures"]=multra::Json::array({procedure()});
    return multra::Json::parse(j.dump());
}
inline std::vector<Case> cases() {
    using namespace multra;
    std::vector<Case> result;
    for(int v=1;v<=kLatestSchema;++v) {
        const auto base=minimal(v);
        const auto add=[&](const std::string& name,const std::function<void(Json&)>& edit,bool valid) {
            auto j=base;edit(j);result.push_back({name,Json::parse(j.dump()),valid});
        };
        result.push_back({"minimal",base,true});
        add("procedures absent",[](Json& j){j.erase("procedures");},true);
        for(const auto& x:std::vector<Json>{nullptr,7,true,"x",Json::object(),Json::array()})
            add("procedures "+x.dump(),[&](Json& j){j["procedures"]=x;},false);
        if(v<from) {add("procedures below 73",[](Json& j){j["procedures"]=Json::array({procedure()});},false);continue;}
        const auto at=[](Json& j)->Json& {return j["procedures"][0];};
        const auto step=[&](Json& j,std::size_t k)->Json& {return at(j)["steps"][k];};
        for(const auto& x:std::vector<Json>{nullptr,7,true,"x",Json::array()})
            add("procedure "+x.dump(),[&](Json& j){at(j)=x;},false);
        add("procedure extra",[&](Json& j){at(j)["extra"]=1;},false);
        for(const auto* key:{"id","name","steps"}) {
            add(std::string("procedure missing ")+key,[&,key](Json& j){at(j).erase(key);},false);
            for(const auto& x:std::vector<Json>{nullptr,7,true,Json::object()})
                add(std::string("procedure ")+key+" "+x.dump(),[&,key](Json& j){at(j)[key]=x;},false);
        }
        add("procedure id blank",[&](Json& j){at(j)["id"]=" ";},true);
        add("procedure name blank",[&](Json& j){at(j)["name"]="";},true);
        add("procedure id duplicate",[&](Json& j){j["procedures"].push_back(procedure());},true);
        add("two procedures",[&](Json& j){auto p=procedure();p["id"]="other";j["procedures"].push_back(p);},true);
        add("steps empty",[&](Json& j){at(j)["steps"]=Json::array();},false);
        add("one step",[&](Json& j){at(j)["steps"]=Json::array({assignment()});},true);
        add("steps repeated kind",[&](Json& j){at(j)["steps"].push_back(generation());},true);
        for(const auto& x:std::vector<Json>{nullptr,7,true,"x",Json::array()})
            add("step "+x.dump(),[&](Json& j){step(j,0)=x;},false);
        add("step kind missing",[&](Json& j){step(j,0).erase("kind");},false);
        for(const auto& x:std::vector<Json>{nullptr,7,true,"assign",""})
            add("step kind "+x.dump(),[&](Json& j){step(j,0)["kind"]=x;},false);
        for(std::size_t k=0;k<4;++k) {
            const auto name="step "+std::to_string(k)+" ";
            add(name+"extra",[&,k](Json& j){step(j,k)["extra"]=1;},false);
            add(name+"active false",[&,k](Json& j){step(j,k)["active"]=false;},true);
            add(name+"active true",[&,k](Json& j){step(j,k)["active"]=true;},true);
            for(const auto& x:std::vector<Json>{nullptr,0,"false"})
                add(name+"active "+x.dump(),[&,k](Json& j){step(j,k)["active"]=x;},false);
        }
        // A key of another kind's row is refused on this one.
        add("generation with period",[&](Json& j){step(j,0)["period"]="am";},false);
        add("assignment with rateSet",[&](Json& j){step(j,3)["rateSet"]="rates";},false);
        add("modeChoice with skimMode",[&](Json& j){step(j,2)["skimMode"]="car";},false);
        for(const auto& [k,key]:std::vector<std::pair<std::size_t,const char*>>{{0,"rateSet"},{1,"deterrenceSet"},{2,"modeChoiceSet"}}) {
            add(std::string("missing ")+key,[&,k,key](Json& j){step(j,k).erase(key);},false);
            for(const auto& x:std::vector<Json>{nullptr,7,Json::array()})
                add(std::string(key)+" "+x.dump(),[&,k,key](Json& j){step(j,k)[key]=x;},false);
            add(std::string(key)+" blank",[&,k,key](Json& j){step(j,k)[key]=" ";},true);
        }
        for(const auto* key:{"skimMode","costMatrix"}) {
            add(std::string("distribution without ")+key,[&,key](Json& j){step(j,1).erase(key);},true);
            for(const auto& x:std::vector<Json>{nullptr,7})
                add(std::string(key)+" "+x.dump(),[&,key](Json& j){step(j,1)[key]=x;},false);
            add(std::string(key)+" blank",[&,key](Json& j){step(j,1)[key]="";},true);
        }
        add("modeChoice without periods",[&](Json& j){step(j,2).erase("periods");},true);
        for(const auto& x:std::vector<Json>{nullptr,"am",Json::array(),Json::array({7}),Json::array({nullptr})})
            add("periods "+x.dump(),[&](Json& j){step(j,2)["periods"]=x;},false);
        add("periods blank item",[&](Json& j){step(j,2)["periods"]=Json::array({"am"," "});},true);
        add("assignment macroRun",[&](Json& j){step(j,3).erase("period");step(j,3)["macroRun"]="run";},true);
        add("assignment neither",[&](Json& j){step(j,3).erase("period");},false);
        add("assignment both",[&](Json& j){step(j,3)["macroRun"]="run";},false);
        for(const auto& x:std::vector<Json>{nullptr,7})
            add("assignment period "+x.dump(),[&](Json& j){step(j,3)["period"]=x;},false);
        add("assignment macroRun null",[&](Json& j){step(j,3).erase("period");step(j,3)["macroRun"]=nullptr;},false);
        add("assignment period blank",[&](Json& j){step(j,3)["period"]=" ";},true);
        // Feedback: refused below 75 (after parsing), then a closed row.
        const bool fb=v>=feedbackFrom;
        add("feedback msa",[&](Json& j){at(j)["feedback"]=feedback();},fb);
        add("feedback null",[&](Json& j){at(j)["feedback"]=nullptr;},false);
        if(!fb)continue;
        const auto f=[&](Json& j)->Json& {at(j)["feedback"]=feedback();return at(j)["feedback"];};
        for(const auto& x:std::vector<Json>{7,true,"msa",Json::array()})
            add("feedback "+x.dump(),[&](Json& j){at(j)["feedback"]=x;},false);
        add("feedback extra",[&](Json& j){f(j)["extra"]=1;},false);
        for(const auto* key:{"method","maxIterations","tolerance"})
            add(std::string("feedback missing ")+key,[&,key](Json& j){f(j).erase(key);},false);
        for(const auto& x:std::vector<Json>{nullptr,7,"MSA","constant"})
            add("method "+x.dump(),[&](Json& j){f(j)["method"]=x;},false);
        add("msa with weight",[&](Json& j){f(j)["weight"]=0.5;},false);
        add("constantWeight without weight",[&](Json& j){f(j)["method"]="constantWeight";},false);
        for(const auto& [w,ok]:std::vector<std::pair<Json,bool>>{{0,true},{0.5,true},{0.999,true},{1,false},{-0.1,false},{nullptr,false},{"0.5",false}})
            add("constantWeight weight "+w.dump(),[&,w](Json& j){auto& x=f(j);x["method"]="constantWeight";x["weight"]=w;},ok);
        // 2.0 is an integer to JSON Schema but not a native integer token: the codec alone refuses it.
        for(const auto& [n,ok]:std::vector<std::pair<Json,bool>>{{1,false},{2,true},{1000,true},{1001,false},{-5,false},{2.0,true},{10.5,false},{nullptr,false},{"10",false}})
            add("maxIterations "+n.dump(),[&,n](Json& j){f(j)["maxIterations"]=n;},ok);
        for(const auto& [t,ok]:std::vector<std::pair<Json,bool>>{{0,true},{1e-9,true},{5,true},{-0.001,false},{nullptr,false},{"0",false}})
            add("tolerance "+t.dump(),[&,t](Json& j){f(j)["tolerance"]=t;},ok);
    }
    return result;
}
}
