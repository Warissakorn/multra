#pragma once
#include "demand_schema_cases.hpp"
#include "study_area_fixture.hpp"

// D567: root studyPeriods and definition.clockStart (49), root studyArea (55); each refused
// below its floor even null. Period rows, the area and its gate-flow rows are closed.
namespace studyCases {
struct Case {std::string name;multra::Json document;bool schemaAccepts;};
inline constexpr int periodFrom=49,areaFrom=55;
inline multra::Json period() {return {{"id","am"},{"name","AM"},{"start",0},{"end",3600}};}
inline multra::Json minimal(int version=periodFrom) {
    auto j=demandCases::minimal(version);
    j["studyPeriods"]=multra::Json::array({period()});j["definition"]["clockStart"]=3600;
    return j;
}
// STUDY_AREA's corridor with one Node kept and one typed gate flow (SA2), saved at 59 and renamed below.
inline multra::Json area(int version) {
    using namespace multra;
    auto c=studyfix::corridor();auto d=c.d;setStudyAreaNodes(d,{c.a});
    d.zones.studyArea.gateFlows={{"p",entryGateName("x"),exitGateName("y"),"car-only","",120}};
    return test::atSchema(documentJson(d),version);
}
inline std::vector<Case> cases() {
    using namespace multra;
    std::vector<Case> result;
    for(int v=1;v<=kLatestSchema;++v) {
        const auto base=minimal(v);const bool ok=v>=periodFrom;
        const auto set=[&](const std::string& path,Json replacement,bool valid) {
            auto j=base;j[Json::json_pointer(path)]=std::move(replacement);result.push_back({path,std::move(j),valid && ok});
        };
        const auto missing=[&](const std::string& path,const char* key,bool valid) {
            auto j=base;j[Json::json_pointer(path)].erase(key);result.push_back({path+" missing "+key,std::move(j),valid && ok});
        };
        result.push_back({"minimal",base,ok});
        {auto j=base;j.erase("studyPeriods");j["definition"].erase("clockStart");result.push_back({"both removed",std::move(j),true});}
        {auto j=base;j["studyPeriods"]=nullptr;j["definition"].erase("clockStart");result.push_back({"studyPeriods null",std::move(j),false});}
        {auto j=base;j.erase("studyPeriods");j["definition"]["clockStart"]=nullptr;result.push_back({"clockStart null",std::move(j),false});}
        missing("","studyPeriods",true);missing("/definition","clockStart",true);
        set("/definition/clockStart","bad",false);set("/definition/clockStart",-1,true); // the range is the model's
        set("/studyPeriods",Json::array(),false);set("/studyPeriods",7,false);set("/studyPeriods/0",7,false);
        set("/studyPeriods/0",nullptr,false);set("/studyPeriods/0/extra",1,false);
        for(const auto* key:{"id","name","start","end"}) {missing("/studyPeriods/0",key,false);set("/studyPeriods/0/"+std::string(key),nullptr,false);}
        set("/studyPeriods/0/id",7,false);set("/studyPeriods/0/name",7,false);set("/studyPeriods/0/start","bad",false);
        set("/studyPeriods/0/end",true,false);set("/studyPeriods/0/name","",true);
        // Below 55 any studyArea is refused, even null; the corridor's own cases start at 55.
        if(v<areaFrom) {auto j=base;j["studyArea"]=nullptr;result.push_back({"studyArea null below 55",std::move(j),false});}
    }
    for(int v=areaFrom;v<=kLatestSchema;++v) {
        const auto base=area(v);const std::string a="/studyArea",f=a+"/gateFlows/0";
        const auto set=[&](const std::string& path,Json replacement,bool valid) {
            auto j=base;j[Json::json_pointer(path)]=std::move(replacement);result.push_back({"area "+path,std::move(j),valid});
        };
        const auto missing=[&](const std::string& path,const char* key,bool valid) {
            auto j=base;j[Json::json_pointer(path)].erase(key);result.push_back({"area "+path+" missing "+key,std::move(j),valid});
        };
        result.push_back({"area minimal",base,true});
        set(a,nullptr,false);set(a,7,false);set(a+"/extra",1,false);
        missing(a,"nodes",false);missing(a,"flowSource",false);missing(a,"gateFlows",true);
        set(a+"/nodes",Json::array(),false);set(a+"/nodes",7,false);set(a+"/nodes/0",7,false);set(a+"/nodes",nullptr,false);
        for(const auto* source:{"typed","macro","meso"})set(a+"/flowSource",source,true);
        set(a+"/flowSource","count",false);set(a+"/flowSource",nullptr,false);
        set(a+"/gateFlows",Json::array(),false);set(a+"/gateFlows",nullptr,false);set(a+"/gateFlows",7,false);set(f,7,false);
        set(f+"/extra",1,false);
        for(const auto* key:{"periodId","from","to","vehiclesPerHour","compositionId"}) {missing(f,key,false);set(f+"/"+key,nullptr,false);}
        set(f+"/periodId",7,false);set(f+"/from",7,false);set(f+"/vehiclesPerHour","bad",false);set(f+"/vehiclesPerHour",-1,true);
        set(f+"/modeId","car-mode",false); // beside compositionId
        {auto j=base;auto& row=j[Json::json_pointer(f)];row.erase("compositionId");row["modeId"]="car-mode";
            result.push_back({"area modeId",j,true});row["modeId"]=7;result.push_back({"area modeId 7",j,false});
            row["compositionId"]=7;row["modeId"]="car-mode";result.push_back({"area modeId and numeric compositionId",j,false});}
    }
    return result;
}
}
