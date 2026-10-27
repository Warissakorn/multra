#pragma once
#include "../src/project/document.hpp"
#include <nlohmann/json.hpp>

namespace measurementCases {
struct Case {std::string name;multra::Json document;bool schemaAccepts;};
inline multra::Json minimal(int version,bool queue=true) {
    using namespace multra;
    auto j=documentJson(ProjectDocument{});j["schemaVersion"]=version;
    j["network"]["links"]=Json::array({Json{{"id","link"},
        {"geometry",Json::array({Json{{"x",0},{"y",0}},Json{{"x",100},{"y",0}}})},
        {"lanes",Json::array({Json{{"id","lane"},{"width",3.5}}})}}});
    if(queue)j["network"]["queueCounters"]=Json::array({Json{{"id","counter"},{"lines",Json::array({Json{
        {"point",{{"path",{{"linkId","link"},{"laneId","lane"}}},{"station",10}}}}})}}});
    else j["network"]["travelTimeSections"]=Json::array({Json{{"id","section"},
        {"start",{{"linkId","link"},{"station",10}}},{"end",{{"linkId","link"},{"station",80}}}}});
    return j;
}
inline std::vector<Case> cases() {
    using namespace multra;
    std::vector<Case> result;
    for(int v=1;v<=kLatestSchema;++v)for(bool queue:{true,false}) {
        const auto base=minimal(v,queue);const int gate=queue?16:23;
        const std::string root=queue?"/network/queueCounters":"/network/travelTimeSections",row=root+"/0";
        const auto expected=[&](bool valid){return v<7 || (v>=gate && valid);};
        const auto set=[&](const std::string& path,Json replacement,bool valid) {
            auto j=base;j[Json::json_pointer(path)]=std::move(replacement);
            result.push_back({path,std::move(j),expected(valid)});
        };
        const auto missing=[&](const std::string& path,const char* key,bool valid) {
            auto j=base;j[Json::json_pointer(path)].erase(key);
            result.push_back({path+" missing "+key,std::move(j),expected(valid)});
        };
        result.push_back({"minimal",base,expected(true)});
        auto absent=base;absent["network"].erase(queue?"queueCounters":"travelTimeSections");
        result.push_back({"absent",std::move(absent),true});
        set(root,nullptr,true);set(root,Json::array(),true);set(root,7,false);set(row,nullptr,false);set(row,7,false);
        set(row+"/extra",1,false);missing(row,"id",false);set(row+"/id",nullptr,false);set(row+"/id",7,false);
        set(row+"/name",nullptr,true);set(row+"/name","ไทย",true);set(row+"/name",7,false);set(row+"/generated",true,false);
        if(queue) {
            const auto line=row+"/lines/0",point=line+"/point",path=point+"/path";
            missing(row,"lines",false);set(row+"/lines",nullptr,false);set(row+"/lines",Json::array(),true);
            set(row+"/lines",true,false);set(line+"/extra",1,false);
            // Non-objects are decoded as empty measurement sources; semantics rejects them.
            for(const auto& value:std::vector<Json>{nullptr,7,true,"ignored",Json::array(),Json::object()})set(line,value,true);
            missing(line,"point",true);set(point,nullptr,true);set(point,7,false);set(point,Json::object(),false);
            set(point+"/extra",1,false);missing(point,"path",false);missing(point,"station",false);
            set(path,nullptr,false);set(point+"/station",nullptr,false);set(point+"/station","bad",false);set(point+"/station",-1,true);
            for(const auto& value:std::vector<Json>{7,true,"ignored",Json::array(),Json::object()})set(path,value,true);
            for(const auto* key:{"linkId","laneId","connectorId","fromLaneId","toLaneId"}) {
                set(path+"/"+key,nullptr,true);set(path+"/"+key,"",true);set(path+"/"+key,7,false);
            }
            set(path+"/extra",1,false);
            set(line+"/referenceId",nullptr,true);set(line+"/referenceId","",true);
            set(line+"/referenceId","missing",true);set(line+"/referenceId",7,false);
        } else {
            for(const auto* name:{"start","end"}) {
                const auto line=row+"/"+name;
                missing(row,name,false);set(line,nullptr,false);set(line,7,false);set(line+"/extra",1,false);
                for(const auto* key:{"linkId","station"}) {missing(line,key,false);set(line+"/"+key,nullptr,false);}
                set(line+"/linkId",7,false);set(line+"/station","bad",false);set(line+"/station",-1,true);
            }
            for(const auto& value:std::vector<Json>{nullptr,"signalised","unsignalised","invalid",7})
                set(row+"/controlType",value,v>=24 && (value.is_null() || value=="signalised" || value=="unsignalised"));
        }
    }
    return result;
}
}
