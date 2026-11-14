#pragma once
#include "demand_schema_cases.hpp"
#include "../src/project/latest_schema.hpp"

// Owned definition.compositions with type-share and interval rows (D558). The same base
// document is read by parseDocument at every version (no read gate); refusals change one key.
namespace compositionCases {
struct Case {std::string name;multra::Json document;bool schemaAccepts;};
inline multra::Json share() {return {{"vehicleTypeId","car"},{"share",1}};}
inline multra::Json minimal(int version=19) {
    using namespace multra;
    auto j=demandCases::minimal(version);
    j["definition"]["compositions"]=Json::array({Json{{"id","mix"},{"name","Mix"},{"types",Json::array({share()})},
        {"intervals",Json::array({Json{{"startTime",0},{"endTime",60},{"types",Json::array({share()})}}})}}});
    return j;
}
inline std::vector<Case> cases() {
    using namespace multra;
    std::vector<Case> result;
    for(int v=1;v<=kLatestSchema;++v) {
        const auto base=minimal(v);
        const std::string root="/definition/compositions",c=root+"/0",s=c+"/types/0",p=c+"/intervals/0",ps=p+"/types/0";
        const auto set=[&](const std::string& path,Json replacement,bool valid) {
            auto j=base;j[Json::json_pointer(path)]=std::move(replacement);result.push_back({path,std::move(j),valid});
        };
        const auto missing=[&](const std::string& path,const char* key,bool valid) {
            auto j=base;j[Json::json_pointer(path)].erase(key);result.push_back({path+" missing "+key,std::move(j),valid});
        };
        result.push_back({"minimal",base,true});missing("/definition","compositions",true);
        set(root,nullptr,false);set(root,7,false);set(root,Json::array(),true);
        set(c,nullptr,false);set(c,7,false);set(s,nullptr,false);set(p,nullptr,false);set(ps,nullptr,false);
        for(const auto& [path,keys]:std::vector<std::pair<std::string,std::vector<const char*>>>{
            {c,{"id","types"}},{s,{"vehicleTypeId","share"}},{p,{"startTime","endTime","types"}},{ps,{"vehicleTypeId","share"}}}) {
            for(const auto* key:keys) {
                missing(path,key,false);set(path+"/"+key,nullptr,false);set(path+"/"+key,true,false);
            }
            set(path+"/extra",1,false); // closed at every file version, including below 7
        }
        set(c+"/name",nullptr,false);set(c+"/name","",true);set(c+"/name",7,false);missing(c,"name",true);
        set(c+"/types",Json::array(),true);set(c+"/types",7,false);
        set(c+"/intervals",nullptr,false);set(c+"/intervals",Json::array(),true);set(c+"/intervals",7,false);
        missing(c,"intervals",true);set(p+"/types",Json::array(),true);set(p+"/types",7,false);
        set(s+"/share",-1,true);set(s+"/share","bad",false);set(ps+"/share","bad",false);
        set(p+"/startTime",-1,true);set(p+"/endTime","bad",false);
    }
    return result;
}
}
