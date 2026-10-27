#pragma once
#include "demand_schema_cases.hpp"

namespace routingCases {
struct Case {std::string name;multra::Json document;bool schemaAccepts;};
inline multra::Json minimal(int version=20) {
    using namespace multra;
    auto j=demandCases::minimal(version);auto& input=j["definition"]["inputs"][0];
    input["routeId"]="";input["routingDecisionId"]="decision";
    j["definition"]["routingDecisions"]=Json::array({Json{{"id","decision"},
        {"routes",Json::array({Json{{"routeId","route"},{"relativeFlow",1},{"intervalFlows",Json::array({2,4})}}})},
        {"intervals",Json::array({Json{{"startTime",0},{"endTime",10}},Json{{"startTime",20},{"endTime",40}}})}}});
    return j;
}
inline std::vector<Case> cases() {
    using namespace multra;
    std::vector<Case> result;
    const Json type={{"vehicleTypeId","car"},{"relativeFlows",Json::array({1})},
        {"intervalFlows",Json::array({Json::array({2,4})})}};
    for(int v=1;v<=kLatestSchema;++v) {
        const auto base=minimal(v);const std::string root="/definition/routingDecisions",d=root+"/0",r=d+"/routes/0",p=d+"/intervals/0";
        const auto set=[&](const std::string& path,Json replacement,bool valid) {
            auto j=base;j[Json::json_pointer(path)]=std::move(replacement);result.push_back({path,std::move(j),valid});
        };
        const auto missing=[&](const std::string& path,const char* key,bool valid) {
            auto j=base;j[Json::json_pointer(path)].erase(key);result.push_back({path+" missing "+key,std::move(j),valid});
        };
        result.push_back({"minimal",base,true});missing("/definition","routingDecisions",true);
        set(root,nullptr,false);set(root,7,false);set(root,Json::array(),true);
        set(d,nullptr,false);set(d,7,false);set(r,nullptr,false);set(p,nullptr,false);
        for(const auto& [path,keys]:std::vector<std::pair<std::string,std::vector<const char*>>>{
            {d,{"id","routes"}},{r,{"routeId","relativeFlow"}},{p,{"startTime","endTime"}}}) {
            for(const auto* key:keys) {
                missing(path,key,false);set(path+"/"+key,nullptr,false);set(path+"/"+key,true,false);
            }
            set(path+"/extra",1,path==p);
        }
        set(d+"/routes",Json::array(),true);set(d+"/routes",7,false);
        for(const auto* key:{"name","linkId"}) {
            set(d+"/"+key,nullptr,true);set(d+"/"+key,"",true);set(d+"/"+key,7,false);
        }
        for(const auto& value:std::vector<Json>{nullptr,0,1.5,-1,"bad",true})
            set(d+"/position",value,v>=20 && (value.is_null() || value.is_number()));
        set(r+"/destinationLinkId",nullptr,true);set(r+"/destinationLinkId","",true);set(r+"/destinationLinkId",7,false);
        set(r+"/relativeFlow",-1,true);set(r+"/relativeFlow","bad",false);
        set(r+"/intervalFlows",nullptr,true);set(r+"/intervalFlows",Json::array(),true);
        set(r+"/intervalFlows",Json::array({"bad"}),false);set(r+"/intervalFlows",7,false);
        set(d+"/intervals",nullptr,true);set(d+"/intervals",Json::array(),true);set(d+"/intervals",7,false);
        set(p+"/startTime",-1,true);set(p+"/endTime","bad",false);
        set(d+"/typeRules",nullptr,false);set(d+"/typeRules",Json::array(),true);set(d+"/typeRules",7,false);
        const auto withRule=[&](Json rule,bool valid) {set(d+"/typeRules",Json::array({std::move(rule)}),valid);};
        withRule(type,true);withRule(nullptr,false);withRule(7,false);
        for(const auto* key:{"vehicleTypeId","relativeFlows","intervalFlows"}) {
            auto rule=type;rule.erase(key);withRule(rule,false);
            rule=type;rule[key]=nullptr;withRule(rule,false);rule[key]=true;withRule(rule,false);
        }
        auto extra=type;extra["extra"]=1;withRule(extra,false);
        for(const auto& [key,values]:std::vector<std::pair<const char*,std::vector<Json>>>{
            {"relativeFlows",{Json::array(),Json::array({-1}),Json::array({"bad"})}},
            {"intervalFlows",{Json::array(),Json::array({Json::array()}),Json::array({Json::array({-1})}),
                Json::array({nullptr}),Json::array({7}),Json::array({Json::array({"bad"})})}}})
            for(const auto& value:values) {
                auto rule=type;rule[key]=value;
                const bool valid=std::string(key)=="relativeFlows"?value!=Json::array({"bad"}):
                    value!=Json::array({nullptr}) && value!=Json::array({7}) && value!=Json::array({Json::array({"bad"})});
                withRule(rule,valid);
            }
        for(const auto* key:{"id","name","linkId","routes","intervals","typeRules","position"}) {
            if(std::string(key)=="position")continue;
            auto j=base;auto& decision=j["definition"]["routingDecisions"][0];decision[key]="bad";decision["position"]=nullptr;
            result.push_back({"position guard precedence",std::move(j),v>=20 && (std::string(key)=="id" || std::string(key)=="name" || std::string(key)=="linkId")});
        }
    }
    return result;
}
}
