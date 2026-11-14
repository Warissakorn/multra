#pragma once
#include "composition_schema_cases.hpp"
#include "../src/project/latest_schema.hpp"

// Owned definition.modes (D559): exact {id, name, compositionId} rows, refused below schema 49 even
// when null. A case that still carries modes is refused below 49; only removing them is accepted.
namespace modeCases {
struct Case {std::string name;multra::Json document;bool schemaAccepts;};
inline constexpr int modeFloor=49;
inline multra::Json mode() {return {{"id","car"},{"name","Car"},{"compositionId","mix"}};}
inline multra::Json minimal(int version=modeFloor) {
    auto j=compositionCases::minimal(version);j["definition"]["modes"]=multra::Json::array({mode()});return j;
}
inline std::vector<Case> cases() {
    using namespace multra;
    std::vector<Case> result;
    for(int v=1;v<=kLatestSchema;++v) {
        const auto base=minimal(v);const bool owned=v>=modeFloor;
        const std::string root="/definition/modes",m=root+"/0";
        const auto set=[&](const std::string& path,Json replacement,bool valid) {
            auto j=base;j[Json::json_pointer(path)]=std::move(replacement);result.push_back({path,std::move(j),valid && owned});
        };
        result.push_back({"minimal",base,owned});
        {auto j=base;j["definition"].erase("modes");result.push_back({"modes removed",std::move(j),true});}
        set(root,nullptr,false);set(root,7,false);set(root,Json::array(),true);
        set(m,nullptr,false);set(m,7,false);set(m+"/extra",1,false);
        for(const auto* key:{"id","name","compositionId"}) {
            auto j=base;j[Json::json_pointer(m)].erase(key);result.push_back({m+" missing "+key,std::move(j),false});
            set(m+"/"+key,nullptr,false);set(m+"/"+key,true,false);set(m+"/"+key,7,false);
        }
        set(m+"/name","",true);set(m+"/compositionId","nope",true);
    }
    return result;
}
}
