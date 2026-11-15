#pragma once
#include "demand_schema_cases.hpp"
#include <functional>

// D573: structural labels only. Zero reference constants, blank/unique ids, nest membership
// and shared strictly negative logsum coefficients remain codec checks, labelled accepted here.
namespace modeChoiceCases {
struct Case {std::string name;multra::Json document;bool schemaAccepts;};
inline constexpr int from=70;
inline multra::Json set() {
    using multra::Json;
    return {{"id","own"},{"name","Own choice set"},{"source",Json{{"citation","c"},{"table","t"}}},{"costUnit","min"},
        {"purposes",Json::array({Json{{"id","HBW"},{"alternatives",Json::array({
            Json{{"id","car"},{"modeId","car"},{"constant",0},{"timeCoefficient",-0.1},{"occupancy",1.2}},
            Json{{"id","bus"},{"costMatrix","bus-times"},{"constant",-1},{"timeCoefficient",-0.1}}})}}})}};
}
inline multra::Json nest() {return {{"id","motor"},{"phi",0.5},{"alternatives",multra::Json::array({"car","bus"})}};}
inline multra::Json minimal(int version=from) {
    auto j=demandCases::minimal(version);
    if(version>=from)j["modeChoiceSets"]=multra::Json::array({set()});
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
        add("modeChoiceSets absent",[](Json& j){j.erase("modeChoiceSets");},true);
        for(const auto& x:std::vector<Json>{nullptr,7,true,Json::object(),Json::array()})
            add("modeChoiceSets "+x.dump(),[&](Json& j){j["modeChoiceSets"]=x;},false);
        if(v<from) {add("modeChoiceSets below 70",[](Json& j){j["modeChoiceSets"]=Json::array({set()});},false);continue;}
        const auto at=[](Json& j)->Json& {return j["modeChoiceSets"][0];};
        for(const auto& x:std::vector<Json>{nullptr,7,true,Json::array()})
            add("set "+x.dump(),[&](Json& j){at(j)=x;},false);
        add("set extra",[&](Json& j){at(j)["extra"]=1;},false);
        for(const auto* key:{"id","name","source","costUnit","purposes"}) {
            add(std::string("set missing ")+key,[&,key](Json& j){at(j).erase(key);},false);
            for(const auto& x:std::vector<Json>{nullptr,7,true})
                add(std::string("set ")+key+" "+x.dump(),[&,key](Json& j){at(j)[key]=x;},false);
        }
        add("set id blank",[&](Json& j){at(j)["id"]=" ";},true);
        add("set name empty",[&](Json& j){at(j)["name"]="";},true);
        add("costUnit unknown",[&](Json& j){at(j)["costUnit"]="s";},false);
        add("source {}",[&](Json& j){at(j)["source"]=Json::object();},true);
        add("source extra",[&](Json& j){at(j)["source"]["extra"]=1;},false);
        for(const auto* key:{"citation","table"}) {
            add(std::string("source without ")+key,[&,key](Json& j){at(j)["source"].erase(key);},true);
            for(const auto& x:std::vector<Json>{nullptr,7,""})
                add(std::string("source ")+key+" "+x.dump(),[&,key](Json& j){at(j)["source"][key]=x;},x.is_string());
        }
        add("two sets one id",[](Json& j){j["modeChoiceSets"].push_back(set());},true);
        add("purposes []",[&](Json& j){at(j)["purposes"]=Json::array();},false);
        add("purpose 7",[&](Json& j){at(j)["purposes"][0]=7;},false);
        const auto p=[&](const std::string& name,const std::function<void(Json&)>& edit,bool valid) {
            add(name,[&](Json& j){edit(at(j)["purposes"][0]);},valid);
        };
        p("purpose extra",[](Json& x){x["extra"]=1;},false);
        p("purpose id blank",[](Json& x){x["id"]="";},true);
        for(const auto* key:{"id","alternatives"}) {
            p(std::string("purpose missing ")+key,[key](Json& x){x.erase(key);},false);
            for(const auto& x:std::vector<Json>{nullptr,7,true})
                p(std::string("purpose ")+key+" "+x.dump(),[&](Json& y){y[key]=x;},false);
        }
        p("alternatives []",[](Json& x){x["alternatives"]=Json::array();},false);
        p("alternative 7",[](Json& x){x["alternatives"][1]=7;},false);
        const auto a=[&](const std::string& name,const std::function<void(Json&)>& edit,bool valid) {
            p(name,[&](Json& x){edit(x["alternatives"][1]);},valid);
        };
        a("alternative extra",[](Json& x){x["extra"]=1;},false);
        for(const auto* key:{"id","constant","timeCoefficient"}) {
            a(std::string("alternative missing ")+key,[key](Json& x){x.erase(key);},false);
            for(const auto& x:std::vector<Json>{nullptr,true,Json::array()})
                a(std::string("alternative ")+key+" "+x.dump(),[&](Json& y){y[key]=x;},false);
        }
        a("alternative id blank",[](Json& x){x["id"]=" ";},true);
        for(const auto* key:{"modeId","costMatrix"}) {
            for(const auto& x:std::vector<Json>{nullptr,7,true,"","mode"})
                a(std::string(key)+" "+x.dump(),[&](Json& y){y[key]=x;},x.is_string());
        }
        a("both cost sources",[](Json& x){x["modeId"]="car";},true);
        a("mode source only",[](Json& x){x.erase("costMatrix");x["modeId"]="car";},true);
        a("neither cost source",[](Json& x){x.erase("costMatrix");x.erase("modeId");},false);
        for(const auto& x:std::vector<Json>{"-1",-1,0,1})
            a("constant "+x.dump(),[&](Json& y){y["constant"]=x;},x.is_number());
        for(const auto& x:std::vector<Json>{"-0.1",-1,0,0.001})
            a("coefficient "+x.dump(),[&](Json& y){y["timeCoefficient"]=x;},x.is_number() && x.get<double>()<=0);
        for(const auto& x:std::vector<Json>{nullptr,true,"1.2",0,0.999,1,2})
            a("occupancy "+x.dump(),[&](Json& y){y["occupancy"]=x;},x.is_number() && x.get<double>()>=1);
        p("no zero constant",[](Json& x){x["alternatives"][0]["constant"]=1;},true);
        p("two alternatives one id",[](Json& x){x["alternatives"][1]["id"]="car";},true);
        add("two purposes one id",[&](Json& j){at(j)["purposes"].push_back(at(j)["purposes"][0]);},true);
        p("nests valid",[](Json& x){x["nests"]=Json::array({nest()});},v>=71);
        for(const auto& x:std::vector<Json>{nullptr,7,true,Json::array()})
            p("nests "+x.dump(),[&](Json& y){y["nests"]=x;},false);
        const auto n=[&](const std::string& name,const std::function<void(Json&)>& edit,bool valid) {
            p(name,[&](Json& x){x["nests"]=Json::array({nest()});edit(x["nests"][0]);},valid && v>=71);
        };
        n("nest 7",[](Json& x){x=7;},false);n("nest extra",[](Json& x){x["extra"]=1;},false);
        for(const auto* key:{"id","phi","alternatives"}) {
            n(std::string("nest missing ")+key,[key](Json& x){x.erase(key);},false);
            for(const auto& x:std::vector<Json>{nullptr,true,Json::object()})
                n(std::string("nest ")+key+" "+x.dump(),[&](Json& y){y[key]=x;},false);
        }
        n("nest id 7",[](Json& x){x["id"]=7;},false);n("nest id blank",[](Json& x){x["id"]=" ";},true);
        for(const auto& x:std::vector<Json>{"0.5",0,-0.1,0.001,1,1.001})
            n("phi "+x.dump(),[&](Json& y){y["phi"]=x;},x.is_number() && x.get<double>()>0 && x.get<double>()<=1);
        n("members []",[](Json& x){x["alternatives"]=Json::array();},false);
        n("members 7",[](Json& x){x["alternatives"]=7;},false);
        for(const auto& x:std::vector<Json>{nullptr,7,true,"","missing","bus"})
            n("member "+x.dump(),[&](Json& y){y["alternatives"][0]=x;},x.is_string());
        p("repeated nest and members",[](Json& x){x["nests"]=Json::array({nest(),nest()});},v>=71);
        p("member in two nests",[](Json& x){auto b=nest();b["id"]="second";x["nests"]=Json::array({nest(),b});},v>=71);
        for(const auto& x:std::vector<Json>{nullptr,7,true,"","other","logsum"})
            p("distributionCost "+x.dump(),[&](Json& y){y["distributionCost"]=x;},v>=76 && x=="logsum");
        p("logsum different coefficients",[](Json& x){x["distributionCost"]="logsum";x["alternatives"][1]["timeCoefficient"]=-0.2;},v>=76);
        p("logsum zero coefficient",[](Json& x){x["distributionCost"]="logsum";for(auto& y:x["alternatives"])y["timeCoefficient"]=0;},v>=76);
        p("nested logsum",[](Json& x){x["distributionCost"]="logsum";x["nests"]=Json::array({nest()});},v>=76);
    }
    return result;
}
}
