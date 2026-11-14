#pragma once
#include "demand_schema_cases.hpp"
#include "../src/project/latest_schema.hpp"

// Owned definition.vehicleTypes (D560). The row is open below 21 and closed from it
// (parseBehaviourLibrary); curve, kerbLanes and pcu are refused below 40, 42 and 49 even when null.
namespace vehicleTypeCases {
struct Case {std::string name;multra::Json document;bool schemaAccepts;};
inline constexpr int closedFrom=21,curveFrom=40,kerbLanesFrom=42,pcuFrom=49;
inline const char* const requiredKeys[]={"id","length","width","desiredSpeed","maxAcceleration",
    "comfortableDeceleration","maxDeceleration","behaviourId"};
inline multra::Json type() { // data/vehicle-types/car.json with a name
    return {{"id","car"},{"name","Car"},{"length",4.5},{"width",1.8},
        {"axles",{{"wheelbase",2.7},{"frontOverhang",0.9},{"rearOverhang",0.9}}},
        {"desiredSpeed",{{"min",11.11},{"max",16.67}}},{"maxAcceleration",2.5},{"comfortableDeceleration",2},
        {"maxDeceleration",8},{"behaviourId","wiedemann-inspired-prototype"}};
}
inline multra::Json minimal(int version=closedFrom) {
    auto j=demandCases::minimal(version);j["definition"]["vehicleTypes"]=multra::Json::array({type()});return j;
}
inline std::vector<Case> cases() {
    using namespace multra;
    std::vector<Case> result;
    for(int v=1;v<=kLatestSchema;++v) {
        const auto base=minimal(v);
        const std::string root="/definition/vehicleTypes",t=root+"/0",a=t+"/axles",s=t+"/desiredSpeed",c=s+"/curve";
        const auto set=[&](const std::string& path,Json replacement,bool valid) {
            auto j=base;j[Json::json_pointer(path)]=std::move(replacement);result.push_back({path,std::move(j),valid});
        };
        const auto missing=[&](const std::string& path,const char* key,bool valid) {
            auto j=base;j[Json::json_pointer(path)].erase(key);result.push_back({path+" missing "+key,std::move(j),valid});
        };
        result.push_back({"minimal",base,true});missing("/definition","vehicleTypes",true);
        set(root,nullptr,false);set(root,7,false);set(root,Json::array(),true);
        set(t,nullptr,false);set(t,7,false);set(t+"/extra",1,v<closedFrom);
        for(const auto* key:requiredKeys) {missing(t,key,false);set(t+"/"+key,nullptr,false);set(t+"/"+key,true,false);}
        set(t+"/name",nullptr,false);set(t+"/name",7,false);set(t+"/name","",true);missing(t,"name",true);
        set(a,nullptr,false);set(a,7,false);missing(t,"axles",true);set(a+"/extra",1,false);
        for(const auto* key:{"wheelbase","frontOverhang","rearOverhang"}) {missing(a,key,false);set(a+"/"+key,nullptr,false);set(a+"/"+key,"x",false);}
        set(s,7,false);set(s+"/extra",1,true);
        for(const auto* key:{"min","max"}) {missing(s,key,false);set(s+"/"+key,nullptr,false);set(s+"/"+key,"x",false);}
        const bool curve=v>=curveFrom,kerb=v>=kerbLanesFrom,pcu=v>=pcuFrom;
        const Json point={{"speed",10},{"share",1}};
        set(c,Json::array(),curve);set(c,Json::array({point}),curve);set(c,nullptr,false);set(c,7,false);
        {auto p=point;p["extra"]=1;set(c,Json::array({p}),curve);}
        {auto p=point;p.erase("share");set(c,Json::array({p}),false);}
        {auto p=point;p["speed"]="x";set(c,Json::array({p}),false);}
        set(t+"/kerbLanes",2,kerb);set(t+"/kerbLanes",2.0,kerb);set(t+"/kerbLanes",0,kerb);
        set(t+"/kerbLanes","x",false);set(t+"/kerbLanes",nullptr,false);
        const Json good={{"value",1},{"source","HCM"}};
        set(t+"/pcu",good,pcu);set(t+"/pcu",nullptr,false);set(t+"/pcu",7,false);
        {auto p=good;p.erase("source");set(t+"/pcu",p,false);}
        {auto p=good;p["extra"]=1;set(t+"/pcu",p,false);}
        {auto p=good;p["value"]="x";set(t+"/pcu",p,false);}
    }
    return result;
}
}
