#pragma once
#include "../src/project/document.hpp"
#include <nlohmann/json.hpp>

namespace rightOfWayCases {
struct Case {std::string name;multra::Json document;bool schemaAccepts;};
inline multra::Json minimal(int version=17) {
    using namespace multra;
    auto j=documentJson(ProjectDocument{});j["schemaVersion"]=version;
    j["network"]["links"]=Json::array({Json{{"id","link"},
        {"geometry",Json::array({Json{{"x",0},{"y",0}},Json{{"x",100},{"y",0}}})},
        {"lanes",Json::array({Json{{"id","lane"},{"width",3.5}}})}}});
    const Json path={{"linkId","link"},{"laneId","lane"}};
    const Json side={{"path",path},{"entryStation",20},{"exitStation",30},{"waitingLineId","line"}};
    j["network"]["rightOfWay"]={{"waitingLines",Json::array({Json{{"id","line"},{"point",{{"path",path},{"station",10}}}}})},
        {"conflictAreas",Json::array({Json{{"id","area"},{"kind","crossing"},{"first",side},{"second",side},{"priority","firstYields"}}})},
        {"priorityRules",Json::array({Json{{"id","rule"},{"conflictAreaId","area"},{"gapTime",3},{"headway",7}}})}};
    if(version!=14)j["network"]["rightOfWay"]["stopControls"]=Json::array({Json{{"id","stop"},{"waitingLineId","line"},
        {"mode","yield"},{"conflictAreaIds",Json::array({"area"})}}});
    return j;
}
inline std::vector<Case> cases() {
    using namespace multra;
    std::vector<Case> result;
    for(int v=1;v<=kLatestSchema;++v) {
        const auto base=minimal(v);const std::string root="/network/rightOfWay";
        const auto expected=[&](bool valid){return v<7 || (v>=14 && valid);};
        const auto set=[&](const std::string& path,Json replacement,bool valid) {
            auto j=base;j[Json::json_pointer(path)]=std::move(replacement);
            result.push_back({path,std::move(j),expected(valid)});
        };
        const auto missing=[&](const std::string& path,const char* key,bool valid) {
            auto j=base;j[Json::json_pointer(path)].erase(key);
            result.push_back({path+" missing "+key,std::move(j),expected(valid)});
        };
        result.push_back({"minimal",base,expected(true)});
        for(const auto& value:std::vector<Json>{nullptr,0,true,"ignored",Json::array(),Json::object()})set(root,value,true);
        auto absent=base;absent["network"].erase("rightOfWay");result.push_back({"absent",absent,true});
        set(root+"/extra",1,false);
        for(const auto* key:{"waitingLines","conflictAreas","priorityRules","stopControls"}) {
            if(v==14 && std::string(key)=="stopControls")continue;
            missing(root,key,true);set(root+"/"+key,nullptr,true);set(root+"/"+key,Json::array(),true);
            set(root+"/"+key,7,false);set(root+"/"+key,Json::array({nullptr}),false);
        }
        const std::string w=root+"/waitingLines/0",a=root+"/conflictAreas/0",r=root+"/priorityRules/0",c=root+"/stopControls/0";
        auto objects=std::vector<std::pair<std::string,std::vector<const char*>>>{
            {w,{"id","point"}},{w+"/point",{"path","station"}},{a,{"id","kind","first","second","priority"}},
            {a+"/first",{"path","entryStation","exitStation","waitingLineId"}},
            {a+"/second",{"path","entryStation","exitStation","waitingLineId"}},
            {r,{"id","conflictAreaId","gapTime","headway"}}};
        if(v!=14)objects.push_back({c,{"id","waitingLineId","mode","conflictAreaIds"}});
        for(const auto& [path,keys]:objects) {
            set(path,7,false);set(path+"/extra",1,false);
            for(const auto* key:keys) {
                missing(path,key,false);set(path+"/"+key,nullptr,false);
                set(path+"/"+key,true,std::string(key)=="path");
            }
        }
        auto rows=std::vector<std::string>{w,a,r};if(v!=14)rows.push_back(c);
        for(const auto& path:rows) {
            set(path+"/name",nullptr,true);set(path+"/name","ไทย",true);set(path+"/name",7,false);
            set(path+"/generated",true,v>=54);set(path+"/generated",false,false);set(path+"/generated",nullptr,false);
        }
        for(const auto& path:{w+"/point/path",a+"/first/path",a+"/second/path"}) {
            for(const auto& value:std::vector<Json>{Json::object(),0,true,"ignored",Json::array()})set(path,value,true);
            for(const auto* key:{"linkId","laneId","connectorId","fromLaneId","toLaneId"}) {
                set(path+"/"+key,nullptr,true);set(path+"/"+key,"",true);set(path+"/"+key,7,false);
            }
            set(path+"/extra",1,false);
        }
        for(const auto& path:{w+"/point/station",a+"/first/entryStation",a+"/second/exitStation",r+"/gapTime",r+"/headway"}) {
            set(path,-1,true);set(path,"bad",false);
        }
        for(const auto* kind:{"crossing","merge","branching","invalid"})set(a+"/kind",kind,std::string(kind)=="crossing" || std::string(kind)=="merge");
        for(const auto* priority:{"firstYields","secondYields","undetermined","bothGo"})set(a+"/priority",priority,std::string(priority)!="bothGo");
        if(v!=14) {
            set(c+"/mode","stop",true);set(c+"/mode","bad",false);
            set(c+"/conflictAreaIds",Json::array(),true);set(c+"/conflictAreaIds",Json::array({7}),false);
        }
        set(root+"/stopControls",nullptr,v>=15);
    }
    return result;
}
}
