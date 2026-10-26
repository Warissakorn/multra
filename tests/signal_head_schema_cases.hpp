#pragma once
#include "../src/project/document.hpp"
#include <nlohmann/json.hpp>
#include <limits>

namespace signalHeadCases {
struct Case {std::string name;multra::Json document;bool schemaAccepts;};
inline multra::Json minimal(int version=17) {
    using namespace multra;
    auto j=documentJson(ProjectDocument{});j["schemaVersion"]=version;
    j["network"]["links"].push_back({{"id","link"},
        {"geometry",Json::array({Json{{"x",0},{"y",0}},Json{{"x",100},{"y",0}}})},
        {"lanes",Json::array({Json{{"id","lane"},{"width",3.5}}})}});
    j["network"]["signalHeads"].push_back({{"id","head"},{"lane",{{"linkId","link"},{"laneId","lane"}}},
        {"position",25},{"programId","legacy"}});
    return j;
}
inline std::vector<Case> cases() {
    using namespace multra;
    std::vector<Case> result;
    for(int v=1;v<=kLatestSchema;++v) {
        const auto base=minimal(v);result.push_back({"minimal",base,true});
        const auto set=[&](const char* path,Json replacement,bool valid) {
            auto j=base;j[Json::json_pointer(path)]=std::move(replacement);
            result.push_back({path,std::move(j),valid});
        };
        const auto field=[&](const char* name,Json replacement,bool valid) {
            set((std::string("/network/signalHeads/0/")+name).c_str(),std::move(replacement),valid);
        };
        for(const auto* name:{"id","lane","position","programId"}) {
            auto j=base;j["network"]["signalHeads"][0].erase(name);
            result.push_back({std::string("missing ")+name,std::move(j),false});field(name,nullptr,false);
        }
        field("lane",7,false);field("position","bad",false);field("position",-1,true);
        field("programId",7,false);field("programId","",true);
        field("name",nullptr,true);field("name","ไทย",true);field("name",true,false);
        field("connectorId",nullptr,true);field("connectorId","",true);field("connectorId",7,false);
        field("extra",1,v<7);
        field("generated",true,v<7 || v>=54);field("generated",false,v<7);field("generated",nullptr,v<7);
        set("/network/signalHeads/0/lane/extra",1,v<7);
        set("/network/signalHeads/0/lane/station",50,v>=5);
        set("/network/signalHeads/0/lane/fraction",0.5,v<5);
        set("/network/signalHeads/0/lane/station",nullptr,false);
        set("/network/signalHeads/0/lane/fraction",nullptr,false);
        // -1 denotes an absent controller/group field; null and empty string are distinct inputs.
        const std::vector<Json> controllers={nullptr,"","controller",7};
        const std::vector<Json> groups={nullptr,"ignored",Json::object(),1,1.0,1.5,
            std::numeric_limits<int>::min(),std::numeric_limits<int>::max(),
            static_cast<std::int64_t>(std::numeric_limits<int>::max())+1};
        for(int ci=-1;ci<static_cast<int>(controllers.size());++ci)
            for(int gi=-1;gi<static_cast<int>(groups.size());++gi) {
                auto j=base;auto& h=j["network"]["signalHeads"][0];
                if(ci>=0)h["controllerId"]=controllers[ci];
                if(gi>=0)h["groupNumber"]=groups[gi];
                const bool active=ci>=0 && !controllers[ci].is_null();
                const bool controllerValid=!active || controllers[ci].is_string();
                const bool groupValid=gi>=0 && (gi==3 || gi==4 || gi==6 || gi==7);
                result.push_back({"controller/group",std::move(j),controllerValid && (!active || groupValid)});
            }
    }
    return result;
}
}
