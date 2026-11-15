#pragma once
#include "demand_schema_cases.hpp"
#include <functional>

// D572: root periodFactors from 69, even null. Structural labels leave blank text, the sum
// of the shares, known periods and unique purpose/period pairs with the existing codec checks.
namespace periodFactorCases {
struct Case {std::string name;multra::Json document;bool schemaAccepts;};
inline constexpr int from=69;
inline multra::Json factor() {
    return {{"purposeId","HBW"},{"periodId","am"},{"productionToAttraction",0.4},{"attractionToProduction",0.1}};
}
inline multra::Json minimal(int version=from) {
    auto j=demandCases::minimal(version);
    if(version>=49)j["studyPeriods"]=multra::Json::array({multra::Json{{"id","am"},{"name","AM"},{"start",0},{"end",60}}});
    if(version>=from)j["periodFactors"]=multra::Json::array({factor()});
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
        add("periodFactors absent",[](Json& j){j.erase("periodFactors");},true);
        for(const auto& value:std::vector<Json>{nullptr,7,true,Json::object(),Json::array()})
            add("periodFactors "+value.dump(),[&](Json& j){j["periodFactors"]=value;},false);
        if(v<from) {add("periodFactors below 69",[](Json& j){j["periodFactors"]=Json::array({factor()});},false);continue;}
        const auto at=[](Json& j)->Json& {return j["periodFactors"][0];};
        for(const auto& value:std::vector<Json>{nullptr,7,true,"row",Json::object(),Json::array()})
            add("row "+value.dump(),[&](Json& j){at(j)=value;},false);
        add("row extra",[&](Json& j){at(j)["extra"]=1;},false);
        for(const auto* key:{"purposeId","periodId","productionToAttraction","attractionToProduction"}) {
            add(std::string("missing ")+key,[&,key](Json& j){at(j).erase(key);},false);
            add(std::string(key)+" null",[&,key](Json& j){at(j)[key]=nullptr;},false);
        }
        for(const auto* key:{"purposeId","periodId"})
            for(const auto& value:std::vector<Json>{7,true,Json::array(),Json::object(),""," \t\r\n","purpose-ไทย"})
                add(std::string(key)+" "+value.dump(),[&,key](Json& j){at(j)[key]=value;},value.is_string());
        for(const auto* key:{"productionToAttraction","attractionToProduction"})
            for(const auto& value:std::vector<Json>{"0.5",true,Json::array(),Json::object(),-1,-0.000001,-0.0,0,0.5,1,1.000001,2})
                add(std::string(key)+" "+value.dump(),[&,key](Json& j){at(j)[key]=value;},
                    value.is_number() && value.get<double>()>=0 && value.get<double>()<=1);
        for(const auto& shares:std::vector<std::pair<double,double>>{{0,0},{0,1},{1,0},{0.25,0.75},{1,1}})
            add("shares "+std::to_string(shares.first)+"/"+std::to_string(shares.second),[&](Json& j){
                at(j)["productionToAttraction"]=shares.first;at(j)["attractionToProduction"]=shares.second;
            },true);
        add("unknown period",[&](Json& j){at(j)["periodId"]="gone";},true);
        add("no study periods",[](Json& j){j.erase("studyPeriods");},true);
        add("repeated pair",[](Json& j){j["periodFactors"].push_back(factor());},true);
        add("another purpose",[](Json& j){auto f=factor();f["purposeId"]="HBO";j["periodFactors"].push_back(f);},true);
        add("extra instead of required key",[&](Json& j){at(j).erase("purposeId");at(j)["extra"]=1;},false);
    }
    return result;
}
}
