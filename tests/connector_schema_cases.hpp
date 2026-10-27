#pragma once
#include "../src/project/document.hpp"
#include <nlohmann/json.hpp>

namespace connectorCases {
struct Case {std::string name;multra::Json document;bool schemaAccepts;};
inline multra::Json minimal(int version=17) {
    using namespace multra;
    auto j=documentJson(ProjectDocument{});j["schemaVersion"]=version;
    for(const auto& [id,start]:std::vector<std::pair<std::string,int>>{{"from",0},{"to",120}})
        j["network"]["links"].push_back({{"id",id},
            {"geometry",Json::array({Json{{"x",start},{"y",0}},Json{{"x",start+100},{"y",0}}})},
            {"lanes",Json::array({Json{{"id",id+"-lane"},{"width",3.5}}})}});
    j["network"]["connectors"].push_back({{"id","connector"},
        {"from",{{"linkId","from"},{"laneId","from-lane"}}},{"to",{{"linkId","to"},{"laneId","to-lane"}}},
        {"geometry",Json::array({Json{{"x",100},{"y",0}},Json{{"x",120},{"y",0}}})}});
    return j;
}
inline std::vector<Case> cases() {
    using namespace multra;
    std::vector<Case> result;
    for(int version=1;version<=kLatestSchema;++version) {
        const auto initial=minimal(version);
        result.push_back({"minimal",initial,true});
        const auto set=[&](const char* path,Json replacement,bool accepts) {
            auto j=initial;j[Json::json_pointer(path)]=std::move(replacement);
            result.push_back({path,std::move(j),accepts});
        };
        const auto field=[&](const char* name,Json replacement,bool accepts) {
            set((std::string("/network/connectors/0/")+name).c_str(),std::move(replacement),accepts);
        };
        for(const auto* name:{"id","from","to","geometry"}) {
            auto j=initial;j["network"]["connectors"][0].erase(name);
            result.push_back({std::string("missing ")+name,std::move(j),false});
            field(name,nullptr,false);
        }
        for(const auto* end:{"from","to"}) {
            const auto path=std::string("/network/connectors/0/")+end;
            for(const auto* name:{"linkId","laneId"}) {
                auto j=initial;j[Json::json_pointer(path)].erase(name);
                result.push_back({path+" missing "+name,std::move(j),false});
                set((path+"/"+name).c_str(),nullptr,false);
            }
            set((path+"/extra").c_str(),true,version<7);
            set((path+"/station").c_str(),0,version>=5);
            set((path+"/fraction").c_str(),0,version<5);
            set((path+"/station").c_str(),nullptr,false);
            set((path+"/fraction").c_str(),nullptr,false);
            set((path+(version<5?"/fraction":"/station")).c_str(),-1,true); // Bounds/migration remain codec-owned.
        }
        for(const auto* name:{"fromLaneCount","toLaneCount","level"}) {
            field(name,1,true);field(name,1.0,true);field(name,1.5,false);
            field(name,nullptr,false);field(name,1001,false);field(name,0,true);
        }
        field("from",7,false);field("to",Json::array(),false);
        field("extra",true,version<7);
        field("laneBlend",Json::array({0,1}),true);
        field("laneBlend",Json::array({"bad"}),false);
        field("laneWidths",Json::array({3.5}),true);
        field("laneWidths",Json::array({"bad"}),false);
        field("laneWidths",Json::array({0}),true);
        field("laneMarkings",Json::array({"solid","dashed","none","double"}),true);
        field("laneMarkings",Json::array({"bad"}),false);
        for(const auto* name:{"laneBlend","laneWidths","laneMarkings","displayType"})field(name,nullptr,false);
        field("name",nullptr,true);field("name","ไทย",true);field("name",7,false);
        field("laneChangeSide",nullptr,version<7 || version>=17);
        field("laneChangeSide","left",version<7 || version>=17);
        field("laneChangeSide","bad",version<7);
        field("behaviourType",nullptr,version<7);
        field("behaviourType","missing",version<7 || version>=21);
        field("generated",true,version<7 || version>=54);
        field("generated",false,version<7);field("generated",nullptr,version<7);
        set("/network/connectors/0/geometry/0/extra",1,version<7);
        set("/network/connectors/0/geometry/0/x","bad",false);
        field("geometry",Json::array(),true); // Structural shape does not establish usable geometry.
    }
    return result;
}
}
