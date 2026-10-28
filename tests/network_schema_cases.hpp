#pragma once
#include "../src/project/document.hpp"
#include <nlohmann/json.hpp>

namespace networkCases {
struct Case {std::string name;multra::Json document;bool schemaAccepts;};
inline std::vector<Case> cases() {
    using namespace multra;
    auto base=documentJson(ProjectDocument{});
    base["network"]["links"].push_back({{"id","link"},{"geometry",Json::array({Json{{"x",0},{"y",0}},Json{{"x",100},{"y",0}}})},
        {"lanes",Json::array({Json{{"id","lane"},{"width",3.5}}})}});
    std::vector<Case> result;
    for(int version=1;version<=kLatestSchema;++version) {
        auto initial=base;initial["schemaVersion"]=version;
        result.push_back({"minimal",initial,true});
        const auto set=[&](const char* path,Json replacement,bool accepts) {
            auto j=initial;j[Json::json_pointer(path)]=std::move(replacement);
            result.push_back({path,std::move(j),accepts});
        };
        const auto erase=[&](const char* path,const char* key) {
            auto j=initial;j[Json::json_pointer(path)].erase(key);
            result.push_back({std::string(path)+" missing "+key,std::move(j),false});
        };
        for(const auto* key:{"id","drivingSide","links","connectors","signalHeads"})erase("/network",key);
        for(const auto* key:{"id","geometry","lanes"})erase("/network/links/0",key);
        for(const auto* key:{"id","width"})erase("/network/links/0/lanes/0",key);
        erase("/network/links/0/geometry/0","x");
        set("/network/drivingSide","right",true);
        set("/network/drivingSide","invalid",false);
        for(const auto* path:{"/network/links","/network/connectors","/network/signalHeads",
                             "/network/links/0/id","/network/links/0/geometry","/network/links/0/lanes",
                             "/network/links/0/lanes/0/width","/network/links/0/geometry/0/x"})set(path,nullptr,false);
        for(const auto* path:{"/network/extra","/network/links/0/extra","/network/links/0/lanes/0/extra",
                             "/network/links/0/geometry/0/extra"})set(path,1,version<7);
        set("/network/links/0/name",nullptr,true);
        set("/network/links/0/name","ไทย",true);
        set("/network/links/0/name",7,false);
        set("/network/links/0/level",-1000,true);
        set("/network/links/0/level",1000,true);
        set("/network/links/0/level",1001,false);
        set("/network/links/0/level",1.0,true); // Native integer tokens still need the codec.
        set("/network/links/0/level",1.5,false);
        set("/network/links/0/level",nullptr,false);
        set("/network/links/0/displayType",nullptr,false);
        set("/network/links/0/laneOffset","bad",false);
        set("/network/links/0/boundaryMarkings",Json::array({"solid","dashed","none","double"}),true);
        set("/network/links/0/boundaryMarkings",Json::array({"bad"}),false);
        set("/network/links/0/behaviourType",nullptr,version<7);
        set("/network/links/0/behaviourType","missing-type",version<7 || version>=21);
        set("/network/links/0/linkType","missing-type",version>=50);
        set("/network/links/0/linkType",nullptr,false);
        set("/network/links/0/roadId","missing-road",version>=53 && version<59); // renamed at 59 (D389)
        set("/network/links/0/roadId",nullptr,false);
        set("/network/links/0/streetSegmentId","missing-road",version>=59);
        set("/network/links/0/streetSegmentId",nullptr,false);
        set("/network/rightOfWay",nullptr,version<7 || version>=14);
        set("/network/queueCounters",nullptr,version<7 || version>=16);
        set("/network/travelTimeSections",nullptr,version<7 || version>=23);
        set("/network/nodes",nullptr,version<7 || version>=48);
        set("/network/roads",nullptr,version>=53 && version<59);
        set("/network/streetSegments",nullptr,version>=59);
        set("/network/overpasses",nullptr,version>=53);
        // Structural success deliberately does not promise semantic model validity.
        set("/network/links/0/geometry",Json::array(),true);
        set("/network/links/0/lanes/0/width",0,true);
        set("/network/connectors",Json::array({7}),false);
    }
    return result;
}
}
