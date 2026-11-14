#pragma once
#include "demand_schema_cases.hpp"

// D568: the root map (M1.21), refused below 29 even null; a closed object with a closed reference.
// The schema states the codec's ranges; the CRS must also be the anchor's UTM zone (the codec's check).
namespace mapCases {
struct Case {std::string name;multra::Json document;bool schemaAccepts;};
inline constexpr int from=29;
inline multra::Json map() {
    return {{"providerId","osm-standard"},{"opacity",0.7},{"reference",{{"projectedCrs","EPSG:32647"},{"anchorLongitude",100.5},
        {"anchorLatitude",13.75},{"anchorWorldX",0},{"anchorWorldY",0},{"rotationDegrees",0}}}};
}
inline multra::Json minimal(int version=from) {auto j=demandCases::minimal(version);j["map"]=map();return j;}
inline std::vector<Case> cases() {
    using namespace multra;
    std::vector<Case> result;
    for(int v=1;v<=kLatestSchema;++v) {
        const auto base=minimal(v);const bool ok=v>=from;const std::string m="/map",r=m+"/reference";
        const auto set=[&](const std::string& path,Json replacement,bool valid) {
            auto j=base;j[Json::json_pointer(path)]=std::move(replacement);result.push_back({path,std::move(j),valid && ok});
        };
        const auto missing=[&](const std::string& path,const char* key) {
            auto j=base;j[Json::json_pointer(path)].erase(key);result.push_back({path+" missing "+key,std::move(j),false});
        };
        result.push_back({"minimal",base,ok});
        {auto j=base;j.erase("map");result.push_back({"map removed",std::move(j),true});}
        set(m,nullptr,false);set(m,7,false);set(m+"/extra",1,false);set(r,nullptr,false);set(r,7,false);set(r+"/extra",1,false);
        for(const auto* key:{"providerId","opacity","reference"}) {missing(m,key);set(m+"/"+key,nullptr,false);}
        for(const auto* key:{"projectedCrs","anchorLongitude","anchorLatitude","anchorWorldX","anchorWorldY","rotationDegrees"}) {
            missing(r,key);set(r+"/"+key,nullptr,false);set(r+"/"+key,true,false);
        }
        set(m+"/providerId",7,false);set(m+"/providerId","",false);set(m+"/providerId","Bad",false);set(m+"/providerId","a_b",false);
        set(m+"/providerId",std::string(80,'a'),true);set(m+"/providerId",std::string(81,'a'),false);set(m+"/providerId","carto-light-2",true);
        set(m+"/opacity",0,true);set(m+"/opacity",1,true);set(m+"/opacity",-0.1,false);set(m+"/opacity",1.1,false);set(m+"/opacity","bad",false);
        set(r+"/projectedCrs","EPSG:32600",false);set(r+"/projectedCrs","EPSG:32661",false);set(r+"/projectedCrs","EPSG:4326",false);
        set(r+"/projectedCrs","EPSG:32747",true); // a southern zone: well formed, but not this anchor's (the codec's check)
        set(r+"/anchorLongitude",180,true);set(r+"/anchorLongitude",180.5,false);set(r+"/anchorLongitude",-181,false);
        set(r+"/anchorLatitude",84,true);set(r+"/anchorLatitude",84.5,false);set(r+"/anchorLatitude",-80.5,false);
        for(const auto* key:{"anchorWorldX","anchorWorldY"}) {set(r+"/"+key,-1e7,true);set(r+"/"+key,1.0000001e7,false);}
        set(r+"/rotationDegrees",-180,true);set(r+"/rotationDegrees",180,true);set(r+"/rotationDegrees",180.5,false);
    }
    return result;
}
}
