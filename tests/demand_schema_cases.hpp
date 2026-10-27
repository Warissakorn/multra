#pragma once
#include "../src/project/document.hpp"
#include <nlohmann/json.hpp>

namespace demandCases {
struct Case {std::string name;multra::Json document;bool schemaAccepts;};
inline multra::Json minimal(int version=17) {
    using namespace multra;
    ProjectDocument d;d.definition.emplace();auto j=documentJson(d);j["schemaVersion"]=version;
    j["network"]["links"]=Json::array({Json{{"id","link"},
        {"geometry",Json::array({Json{{"x",0},{"y",0}},Json{{"x",100},{"y",0}}})},
        {"lanes",Json::array({Json{{"id","lane"},{"width",3.5}}})}}});
    j["definition"]["duration"]=60;j["definition"]["timeStep"]=0.1;
    j["definition"]["routes"]=Json::array({Json{{"id","route"},{"segmentIds",Json::array({"link"})}}});
    j["definition"]["inputs"]=Json::array({Json{{"id","input"},{"routeId","route"},{"vehicleTypeId","car"},
        {"vehiclesPerHour",100},{"startTime",0},{"endTime",60}}});
    return j;
}
inline std::vector<Case> cases() {
    using namespace multra;
    std::vector<Case> result;
    for(int v=1;v<=kLatestSchema;++v) {
        const auto base=minimal(v);const std::string d="/definition",r=d+"/routes/0",i=d+"/inputs/0";
        const auto set=[&](const std::string& path,Json replacement,bool valid) {
            auto j=base;j[Json::json_pointer(path)]=std::move(replacement);
            result.push_back({path,std::move(j),valid});
        };
        const auto missing=[&](const std::string& path,const char* key,bool valid) {
            auto j=base;j[Json::json_pointer(path)].erase(key);result.push_back({path+" missing "+key,std::move(j),valid});
        };
        result.push_back({"minimal",base,true});
        for(const auto& [path,keys]:std::vector<std::pair<std::string,std::vector<const char*>>>{
            {d,{"duration","timeStep","routes","inputs"}},{r,{"id","segmentIds"}},
            {i,{"id","routeId","vehicleTypeId","vehiclesPerHour","startTime","endTime"}}}) {
            for(const auto* key:keys) {
                missing(path,key,false);set(path+"/"+key,nullptr,false);set(path+"/"+key,true,false);
            }
            set(path+"/extra",1,true);
        }
        set(r,nullptr,false);set(i,nullptr,false);set(r,7,false);set(i,7,false);
        for(const auto& path:{d+"/routes",d+"/inputs",r+"/segmentIds"}) {
            set(path,Json::array(),true);set(path,7,false);
        }
        set(r+"/segmentIds",Json::array({7}),false);set(r+"/segmentIds",Json::array({nullptr}),false);
        for(const auto& path:{d+"/duration",d+"/timeStep",i+"/vehiclesPerHour",i+"/startTime",i+"/endTime"}) {
            set(path,"bad",false);set(path,-1,true);
        }
        for(const auto* key:{"compositionId","routingDecisionId","linkId"}) {
            set(i+"/"+key,nullptr,false);set(i+"/"+key,7,false);set(i+"/"+key,"",true);
        }
        set(i+"/laneShares",nullptr,false);set(i+"/laneShares",Json::array(),true);
        set(i+"/laneShares",Json::array({1.0,0.0}),true);set(i+"/laneShares",Json::array({-1.0}),true);
        set(i+"/laneShares",Json::array({"bad"}),false);
        for(const auto& value:std::vector<Json>{nullptr,true,false,7,"true"})
            set(i+"/volumeFromCounts",value,v>=26 && value.is_boolean());
        set(i+"/intervals",nullptr,false);set(i+"/intervals",7,false);set(i+"/intervals",Json::array(),true);
        const Json interval={{"startTime",10},{"endTime",30},{"vehiclesPerHour",200}};
        const auto withInterval=[&](Json period,bool valid) {set(i+"/intervals",Json::array({std::move(period)}),valid);};
        withInterval(interval,true);withInterval(nullptr,false);withInterval(7,false);
        for(const auto* key:{"startTime","endTime","vehiclesPerHour"}) {
            auto p=interval;p.erase(key);withInterval(p,false);
            for(const auto& value:std::vector<Json>{nullptr,"bad",true,Json::object(),-1}) {
                p=interval;p[key]=value;withInterval(p,value.is_number());
            }
        }
        auto extra=interval;extra["extra"]=1;withInterval(extra,true);
        missing("","definition",true);set(d,nullptr,true);
    }
    return result;
}
}
