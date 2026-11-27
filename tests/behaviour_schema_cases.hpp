#pragma once
#include "vehicle_type_schema_cases.hpp"
#include "../src/project/latest_schema.hpp"

// Owned definition.behaviours, slice B1 (D563): the model tag and the per-model rows; the rows
// stay open and the ten presence floors are B2's, so a case using a floored key is not labelled.
namespace behaviourCases {
struct Case {std::string name;multra::Json document;bool schemaAccepts;};
inline constexpr int libraryFrom=21,w74From=25,w99From=85,w99CurveFrom=86;
enum class Model {prototype,w74,w99};
inline const char* const prototypeRequired[]={"standstillDistance","additiveSafetyDistance",
    "multiplicativeSafetyDistance","followingTime","speedThreshold"};
inline multra::Json prototype(int version) { // data/driver-behaviour/, without the schema-28 key
    multra::Json b={{"id","wiedemann-inspired-prototype"},{"standstillDistance",2},{"additiveSafetyDistance",2},
        {"multiplicativeSafetyDistance",3},{"followingTime",1.5},{"speedThreshold",0.2},{"maxDecelerationCooperativeBraking",3}};
    if(version>=libraryFrom)b["model"]="prototype";
    return b;
}
inline multra::Json w74() { // tests/w74_codec_tests.cpp's kP
    return {{"id","w74-test"},{"model","w74"},{"ax",2},{"bxAdd",2},{"bxMult",1},{"exAdd",2},{"exMult",0.5},
        {"cxAdd",16},{"cxMult",4},{"opdvAdd",0.5},{"opdvMult",0.5},{"dMax",20},{"bMaxAdd",1},{"bMaxMult",1},
        {"bMaxSpeedRoot",3},{"bNullAdd",0.25},{"bNullMult",0.5},{"bMinAdd",-1},{"leaderAccelerationWeight",0.5},
        {"emergencyLeaderWeight",0.25}};
}
inline multra::Json w99() { // tests/w99_codec_tests.cpp's kP (W99 §5's fixture values)
    return {{"id","w99-test"},{"model","w99"},{"cc0",1.5},{"cc1",1.3},{"cc2",4},{"cc3",-12},{"cc4",-0.25},
        {"cc5",0.35},{"cc6",6},{"cc7",0.25},{"cc8",2},{"cc9",1.5}};
}
inline multra::Json minimal(int version,Model model=Model::prototype) {
    // Owned behaviours need owned vehicle types; the car keeps the prototype (index 1 beside a W74 or W99 row).
    auto j=vehicleTypeCases::minimal(version);
    j["definition"]["behaviours"]=model==Model::prototype?multra::Json::array({prototype(version)})
        :multra::Json::array({model==Model::w74?w74():w99(),prototype(version)});
    return j;
}
inline std::vector<std::pair<std::string,int>> floors();
inline std::vector<Case> cases() {
    using namespace multra;
    std::vector<Case> result;
    for(int v=1;v<=kLatestSchema;++v)for(const auto model:{Model::prototype,Model::w74,Model::w99}) {
        if((model==Model::w74 && v<w74From) || (model==Model::w99 && v<w99From))continue;
        const bool isW74=model==Model::w74,isW99=model==Model::w99,isPrototype=model==Model::prototype;
        const auto base=minimal(v,model);const std::string b="/definition/behaviours/0";const bool lib=v>=libraryFrom;
        const auto set=[&](const std::string& path,Json replacement,bool valid) {
            auto j=base;j[Json::json_pointer(path)]=std::move(replacement);result.push_back({path,std::move(j),valid});
        };
        const auto missing=[&](const char* key,bool valid) {
            auto j=base;j[Json::json_pointer(b)].erase(key);result.push_back({std::string(b)+" missing "+key,std::move(j),valid});
        };
        result.push_back({"minimal",base,true});
        set("/definition/behaviours",nullptr,false);set("/definition/behaviours",7,false);set("/definition/behaviours",Json::array(),true);
        set(b,nullptr,false);set(b,7,false);set(b+"/extra",1,!lib); // D564: closed from 21
        missing("id",false);set(b+"/id",nullptr,false);set(b+"/id",true,false);
        const auto own=isW74?w74():isW99?w99():prototype(v);
        for(const auto& [key,value]:own.items()) {
            if(key=="id" || key=="model")continue;
            missing(key.c_str(),key=="maxDecelerationCooperativeBraking");set(b+"/"+key,nullptr,false);set(b+"/"+key,"x",false);
        }
        // The other model's keys, at every version.
        if(!isPrototype)for(const auto* key:prototypeRequired)set(b+"/"+key,1,false);
        if(!isW74)for(const auto* key:{"ax","bMinAdd","emergencyLeaderWeight"})set(b+"/"+key,1,false);
        if(!isW99)for(const auto* key:{"cc0","cc6","cc9"})set(b+"/"+key,1,false); // D639: at every version
        const Json curve=Json::array({{{"value",0.6},{"share",0}},{{"value",1.5},{"share",1}}});
        if(!isW99)set(b+"/cc1Curve",curve,false); // D646: refused on the other models at every version
        else { // D646: exactly one of cc1 and cc1Curve, the curve from 86
            set(b+"/cc1Curve",curve,false); // both
            auto only=base;only[Json::json_pointer(b)].erase("cc1");only[Json::json_pointer(b+"/cc1Curve")]=curve;
            result.push_back({"cc1Curve only",only,v>=w99CurveFrom});
            auto extra=only;extra[Json::json_pointer(b+"/cc1Curve/1/extra")]=1;result.push_back({"cc1Curve point extra",extra,false});
            auto shareless=only;shareless[Json::json_pointer(b+"/cc1Curve/0")].erase("share");result.push_back({"cc1Curve point without share",shareless,false});
            auto text=only;text[Json::json_pointer(b+"/cc1Curve/0/value")]="x";result.push_back({"cc1Curve value text",text,false});
        }
        if(!isPrototype)set(b+"/discretionaryLaneChangeHoldTime",1,false);
        set(b+"/name","Name",lib);set(b+"/name",nullptr,false);set(b+"/name",7,false);
        if(!lib) {set(b+"/model","prototype",false);set(b+"/model",nullptr,false);}
        else {
            missing("model",false);set(b+"/model",nullptr,false);set(b+"/model",7,false);set(b+"/model","w100",false);
            if(!isW99)set(b+"/model","w99",false); // another row tagged w99 lacks the cc keys (and is refused below 85)
            if(!isW74)set(b+"/model","w74",false); // a row tagged w74 lacks the W74 keys (and is refused below 25)
            if(!isPrototype)set(b+"/model","prototype",false);
        }
        set(b+"/maxDecelerationCooperativeBraking","x",false);
        for(const auto& [key,floor]:floors()) { // D564: refused below each floor, even null
            set(b+"/"+key,2,v>=floor);set(b+"/"+key,nullptr,false);set(b+"/"+key,"x",false);
        }
        set(b+"/observedVehiclesAhead",2.5,false);
    }
    return result;
}
}
namespace behaviourCases {
// D564 (B2): the ten presence floors, one row per key (perception and lateral: every key of the list).
inline std::vector<std::pair<std::string,int>> floors() {
    std::vector<std::pair<std::string,int>> result{{"amberDeceleration",28},{"curveLateralAcceleration",34},
        {"kerbLaneBias",35},{"commitmentDeceleration",36},{"mandatoryDecelerationOwn",37},
        {"mandatoryDecelerationTrailingVehicle",37},{"stopDwellTime",41},{"driverFactorSpread",43},
        {"redRunningShare",44},{"redRunningTime",44},{"missedExitAfter",78},{"diffusionTime",82}};
    for(const auto* key:{"lookAheadDistance","observedVehiclesAhead","lookBackDistance","rearTimeToCollision","reactionTime"})
        result.push_back({key,32});
    for(const auto* key:{"lateralMinDistanceStanding","lateralMinDistanceAt50","maxLateralSpeed"})result.push_back({key,33});
    return result;
}
}
