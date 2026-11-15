#pragma once
#include "demand_schema_cases.hpp"
#include <functional>

// D571: schema-67 owned sets; constraint from 68, destination from 72. These labels describe
// structural coverage only: blank/unique ids and catalog citation requirements stay with the codec.
namespace deterrenceCases {
struct Case {std::string name;multra::Json document;bool schemaAccepts;};
inline constexpr int from=67;
inline multra::Json set() {
    return {{"id","own"},{"name","Own set"},{"source",multra::Json{{"citation","c"},{"table","t"}}},
        {"function","exponential"},{"costUnit","min"},
        {"purposes",multra::Json::array({multra::Json{{"id","HBW"},{"beta",0.1},{"intrazonalShare",0.2}}})}};
}
inline multra::Json minimal(int version=from) {
    auto j=demandCases::minimal(version);
    if(version>=from)j["deterrenceSets"]=multra::Json::array({set()});
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
        add("deterrenceSets absent",[](Json& j){j.erase("deterrenceSets");},true);
        add("deterrenceSets null",[](Json& j){j["deterrenceSets"]=nullptr;},false);
        if(v<from) {add("deterrenceSets below 67",[](Json& j){j["deterrenceSets"]=Json::array({set()});},false);continue;}
        const auto at=[](Json& j)->Json& {return j["deterrenceSets"][0];};
        add("deterrenceSets []",[](Json& j){j["deterrenceSets"]=Json::array();},false);
        add("deterrenceSets 7",[](Json& j){j["deterrenceSets"]=7;},false);
        add("set 7",[&](Json& j){at(j)=7;},false);
        add("set extra",[&](Json& j){at(j)["extra"]=1;},false);
        for(const auto* key:{"id","name","source","function","costUnit","purposes"}) {
            add(std::string("set missing ")+key,[&,key](Json& j){at(j).erase(key);},false);
            add(std::string("set ")+key+" null",[&,key](Json& j){at(j)[key]=nullptr;},false);
            add(std::string("set ")+key+" 7",[&,key](Json& j){at(j)[key]=7;},false);
        }
        add("set id blank",[&](Json& j){at(j)["id"]=" ";},true);
        add("set name empty",[&](Json& j){at(j)["name"]="";},true);
        add("function unknown",[&](Json& j){at(j)["function"]="power";},false);
        add("costUnit unknown",[&](Json& j){at(j)["costUnit"]="s";},false);
        add("source {}",[&](Json& j){at(j)["source"]=Json::object();},true);
        add("source extra",[&](Json& j){at(j)["source"]["extra"]=1;},false);
        for(const auto* key:{"citation","table"}) {
            add(std::string("source ")+key+" null",[&,key](Json& j){at(j)["source"][key]=nullptr;},false);
            add(std::string("source ")+key+" 7",[&,key](Json& j){at(j)["source"][key]=7;},false);
            add(std::string("source without ")+key,[&,key](Json& j){at(j)["source"].erase(key);},true);
            add(std::string("source ")+key+" blank",[&,key](Json& j){at(j)["source"][key]=" ";},true);
        }
        add("two sets, one id",[&](Json& j){j["deterrenceSets"].push_back(set());},true);
        add("purposes []",[&](Json& j){at(j)["purposes"]=Json::array();},false);
        add("purpose 7",[&](Json& j){at(j)["purposes"][0]=7;},false);
        const auto p=[&](const std::string& name,const std::function<void(Json&)>& edit,bool valid) {
            add(name,[&](Json& j){edit(at(j)["purposes"][0]);},valid);
        };
        p("purpose extra",[](Json& x){x["extra"]=1;},false);
        p("purpose id blank",[](Json& x){x["id"]="";},true);
        for(const auto* key:{"id","beta"}) {
            p(std::string("purpose missing ")+key,[key](Json& x){x.erase(key);},false);
            p(std::string("purpose ")+key+" null",[key](Json& x){x[key]=nullptr;},false);
        }
        p("purpose id 7",[](Json& x){x["id"]=7;},false);
        for(const auto& value:std::vector<Json>{"0.1",true,0,-0.1,0.001})
            p("beta "+value.dump(),[&](Json& x){x["beta"]=value;},value.is_number() && value.get<double>()>0);
        p("intrazonalShare absent",[](Json& x){x.erase("intrazonalShare");},true);
        for(const auto& value:std::vector<Json>{nullptr,"0.5",true,-0.1,0,0.999,1,2})
            p("intrazonalShare "+value.dump(),[&](Json& x){x["intrazonalShare"]=value;},
                value.is_number() && value.get<double>()>=0 && value.get<double>()<1);
        for(const auto& value:std::vector<Json>{nullptr,7,true,"","other","double","origin","destination"})
            p("constraint "+value.dump(),[&](Json& x){x["constraint"]=value;},
                v>=68 && (value=="double" || value=="origin" || (v>=72 && value=="destination")));
        add("two purposes, one id",[&](Json& j){at(j)["purposes"].push_back(at(j)["purposes"][0]);},true);
    }
    return result;
}
}
