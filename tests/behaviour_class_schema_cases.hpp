#pragma once
#include "behaviour_schema_cases.hpp"

// D565: definition.vehicleClasses and linkBehaviourTypes, refused below 21 even null; closed rows.
namespace behaviourClassCases {
struct Case {std::string name;multra::Json document;bool schemaAccepts;};
inline constexpr int from=21;
inline multra::Json vehicleClass() {return {{"id","cars"},{"name","Cars"},{"vehicleTypeIds",multra::Json::array({"car"})}};}
inline multra::Json linkType() {
    return {{"id","urban"},{"name","Urban"},{"defaultBehaviourId","wiedemann-inspired-prototype"},
        {"overrides",multra::Json::array({multra::Json{{"classId","cars"},{"behaviourId","wiedemann-inspired-prototype"}}})}};
}
inline multra::Json minimal(int version=from) {
    auto j=behaviourCases::minimal(version);
    j["definition"]["vehicleClasses"]=multra::Json::array({vehicleClass()});
    j["definition"]["linkBehaviourTypes"]=multra::Json::array({linkType()});
    return j;
}
inline std::vector<Case> cases() {
    using namespace multra;
    std::vector<Case> result;
    for(int v=1;v<=kLatestSchema;++v) {
        const auto base=minimal(v);const bool ok=v>=from;
        const std::string c="/definition/vehicleClasses",t="/definition/linkBehaviourTypes",o=t+"/0/overrides/0";
        const auto set=[&](const std::string& path,Json replacement,bool valid) {
            auto j=base;j[Json::json_pointer(path)]=std::move(replacement);result.push_back({path,std::move(j),valid && ok});
        };
        const auto missing=[&](const std::string& path,const char* key,bool valid) {
            auto j=base;j[Json::json_pointer(path)].erase(key);result.push_back({path+" missing "+key,std::move(j),valid && ok});
        };
        result.push_back({"minimal",base,ok});
        {auto j=base;j["definition"].erase("vehicleClasses");j["definition"].erase("linkBehaviourTypes");result.push_back({"both removed",std::move(j),true});}
        for(const auto& root:{c,t}) {set(root,nullptr,false);set(root,7,false);set(root+"/0",7,false);set(root+"/0/extra",1,false);
            missing(root+"/0","id",false);set(root+"/0/id",nullptr,false);set(root+"/0/id",7,false);
            set(root+"/0/name",nullptr,false);set(root+"/0/name",7,false);missing(root+"/0","name",true);}
        missing(c+"/0","vehicleTypeIds",false);set(c+"/0/vehicleTypeIds",7,false);set(c+"/0/vehicleTypeIds",Json::array({7}),false);
        set(t+"/0/defaultBehaviourId",nullptr,false);set(t+"/0/defaultBehaviourId",7,false);missing(t+"/0","defaultBehaviourId",true);
        set(t+"/0/overrides",nullptr,false);set(t+"/0/overrides",7,false);set(t+"/0/overrides",Json::array(),true);missing(t+"/0","overrides",true);
        set(o,7,false);set(o+"/extra",1,false);
        for(const auto* key:{"classId","behaviourId"}) {missing(o,key,false);set(o+"/"+key,nullptr,false);set(o+"/"+key,7,false);}
    }
    return result;
}
}
