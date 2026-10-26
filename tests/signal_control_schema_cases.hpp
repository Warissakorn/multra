#pragma once
#include "../src/project/document.hpp"
#include <nlohmann/json.hpp>
#include <limits>

namespace signalControlCases {
struct Case {std::string name;multra::Json document;bool schemaAccepts;};
inline multra::Json minimal(int version=17) {
    using namespace multra;
    ProjectDocument d;d.definition.emplace();
    auto j=documentJson(d);j["schemaVersion"]=version;
    j["definition"]["signalControllers"]=Json::array({Json{{"id","controller"},{"cycle",60},{"offset",0},
        {"groups",Json::array({Json{{"number",1},{"greenStart",0},{"greenEnd",20},{"amber",3}}})}}});
    j["definition"]["signalPrograms"]=Json::array({Json{{"id","legacy"},{"offset",0},
        {"phases",Json::array({Json{{"duration",10},{"color","green"}},Json{{"duration",10},{"color","red"}}})}}});
    return j;
}
inline std::vector<Case> cases() {
    using namespace multra;
    std::vector<Case> result;
    for(int v=1;v<=kLatestSchema;++v) {
        const auto base=minimal(v);result.push_back({"minimal",base,true});
        const auto set=[&](const std::string& path,Json replacement,bool valid) {
            auto j=base;j[Json::json_pointer(path)]=std::move(replacement);
            result.push_back({path,std::move(j),valid});
        };
        const auto missing=[&](const std::string& parent,const char* key,bool valid) {
            auto j=base;j[Json::json_pointer(parent)].erase(key);
            result.push_back({parent+" missing "+key,std::move(j),valid});
        };
        const std::string c="/definition/signalControllers/0",g=c+"/groups/0";
        const std::string p="/definition/signalPrograms/0",f=p+"/phases/0";
        for(const auto& [path,keys]:std::vector<std::pair<std::string,std::vector<const char*>>>{
            {c,{"id","cycle","offset","groups"}},{g,{"number","greenStart","greenEnd","amber"}},
            {p,{"id","offset","phases"}},{f,{"duration","color"}}}) {
            set(path,nullptr,false);set(path,7,false);
            for(const auto* key:keys) {
                missing(path,key,false);set(path+"/"+key,nullptr,false);
                set(path+"/"+key,true,false);set(path+"/"+key,Json::object(),false);
            }
            set(path+"/extra",1,path==p || path==f);
        }
        for(const auto& path:{c,g}) {
            set(path+"/name",nullptr,true);set(path+"/name","ไทย",true);set(path+"/name",1,false);
        }
        for(const auto& path:{c+"/groups",p+"/phases"}) {
            set(path,Json::array(),true);set(path,"bad",false);
        }
        for(const auto* key:{"signalControllers","signalPrograms"}) {
            set(std::string("/definition/")+key,nullptr,false);
            set(std::string("/definition/")+key,Json::array(),true);
            missing("/definition",key,std::string(key)=="signalControllers");
        }
        for(const auto& path:{c+"/cycle",c+"/offset",g+"/greenStart",g+"/greenEnd",g+"/amber",p+"/offset",f+"/duration"}) {
            set(path,"bad",false);set(path,-1,true);
        }
        for(const auto& [n,valid]:std::vector<std::pair<Json,bool>>{{1.0,true},{1.5,false},{0,true},{-1,true},
            {std::numeric_limits<int>::min(),true},{std::numeric_limits<int>::max(),true},
            {static_cast<std::int64_t>(std::numeric_limits<int>::max())+1,false},
            {static_cast<std::int64_t>(std::numeric_limits<int>::min())-1,false},{"1",false}})
            set(g+"/number",n,valid);
        for(const auto& [color,valid]:std::vector<std::pair<Json,bool>>{{"red",true},{"amber",true},{"green",true},
            {"Red",false},{"",false},{1,false}})set(f+"/color",color,valid);
        set(c+"/generated",true,v>=54);set(c+"/generated",false,false);set(c+"/generated",nullptr,false);
        set(g+"/generated",true,false);set(p+"/generated",false,true);
        missing("","definition",true);set("/definition",nullptr,true);
    }
    return result;
}
}
