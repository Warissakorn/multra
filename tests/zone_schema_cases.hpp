#pragma once
#include "demand_schema_cases.hpp"
#include "street_segment_zone_fixture.hpp"
#include "test.hpp"

// D566: root zones, zoneConnectors, odMatrices and parkingLots, refused below 31 even null;
// nonempty arrays of closed rows. A matrix takes periodId or both times, modeId or
// compositionId; a connector takes linkId or the street-segment keys (57; renamed at 59).
namespace zoneCases {
struct Case {std::string name;multra::Json document;bool schemaAccepts;};
inline constexpr int from=31,streetFrom=57,renamed=59,modeFrom=49,generatedFrom=70;
inline multra::Json square(double x) {
    return multra::Json::array({multra::Json::array({x,0}),multra::Json::array({x+10,0}),
        multra::Json::array({x+10,10}),multra::Json::array({x,10})});
}
inline multra::Json zone(const char* id,double x) {
    return {{"id",id},{"name",id},{"polygon",square(x)},{"attributes",{{"households",10}}}};
}
inline multra::Json matrix() {
    return {{"id","od"},{"zoneIds",multra::Json::array({"a","b"})},{"cells",multra::Json::array({multra::Json::array({0,5}),multra::Json::array({5,0})})},
        {"compositionId","mix"},{"startTime",0},{"endTime",60}};
}
// Parsed from text so the lot's counts are unsigned tokens, as a saved file holds them.
inline multra::Json minimal(int version=from) {
    using namespace multra;
    auto j=demandCases::minimal(version);
    j["zones"]=Json::array({zone("a",0),zone("b",50)});
    j["zoneConnectors"]=Json::array({Json{{"id","a-out"},{"zoneId","a"},{"linkId","link"},{"direction","origin"},{"share",1}}});
    j["odMatrices"]=Json::array({matrix()});
    j["parkingLots"]=Json::array({Json{{"id","lot"},{"zoneId","a"},{"capacity",10},{"initialOccupancy",0},
        {"initialCompositionId",""},{"gateServiceTime",2},{"dwell",Json::array({Json::array({0,0}),Json::array({30,1})})}}});
    return Json::parse(j.dump());
}
// A period-based matrix needs its period (S1.1); from 49 the base also carries one.
inline multra::Json withPeriod(multra::Json j) {
    j["studyPeriods"]=multra::Json::array({multra::Json{{"id","am"},{"name","AM"},{"start",0},{"end",3600}}});
    return j;
}
// Street-segment connectors (RD30's fixture), saved at its own version and renamed below 59.
inline multra::Json street(int version) {
    return test::atSchema(multra::documentJson(test::streetSegmentZoneFixture(test::root()/"tests/macro-two-route.traffic.json")),version);
}
inline std::vector<Case> cases() {
    using namespace multra;
    std::vector<Case> result;
    for(int v=1;v<=kLatestSchema;++v) {
        const bool ok=v>=from;
        auto base=minimal(v);if(v>=modeFrom)base=withPeriod(base);
        const auto set=[&](const std::string& path,Json replacement,bool valid) {
            auto j=base;j[Json::json_pointer(path)]=std::move(replacement);result.push_back({path,std::move(j),valid && ok});
        };
        const auto missing=[&](const std::string& path,const char* key,bool valid) {
            auto j=base;j[Json::json_pointer(path)].erase(key);result.push_back({path+" missing "+key,std::move(j),valid && ok});
        };
        result.push_back({"minimal",base,ok});
        {auto j=base;for(const auto* key:{"zones","zoneConnectors","odMatrices","parkingLots"})j.erase(key);result.push_back({"layer removed",std::move(j),true});}
        for(const auto* key:{"zones","zoneConnectors","odMatrices","parkingLots"}) {
            const auto root="/"+std::string(key);
            {auto j=base;j[key]=nullptr;result.push_back({root+" null",std::move(j),false});} // even null below 31
            set(root,Json::array(),false);set(root,7,false);set(root+"/0",7,false);set(root+"/0",nullptr,false);
            set(root+"/0/extra",1,false);missing(root+"/0","id",false);set(root+"/0/id",nullptr,false);set(root+"/0/id",7,false);
        }
        const std::string z="/zones/0",c="/zoneConnectors/0",m="/odMatrices/0",l="/parkingLots/0";
        for(const auto* key:{"name","polygon","attributes"}) {missing(z,key,false);set(z+"/"+key,nullptr,false);}
        set(z+"/name",7,false);set(z+"/polygon",7,false);set(z+"/polygon/0",Json::array({0}),false);
        set(z+"/polygon/0",Json::array({0,0,0}),false);set(z+"/polygon/0",Json::array({0,"bad"}),false);set(z+"/polygon/0",7,false);
        set(z+"/attributes",7,false);set(z+"/attributes/households","bad",false);set(z+"/attributes",Json::object(),true);
        for(const auto* key:{"zoneId","linkId","direction","share"}) {missing(c,key,false);set(c+"/"+key,nullptr,false);}
        set(c+"/zoneId",7,false);set(c+"/linkId",7,false);set(c+"/direction","destination",true);set(c+"/direction","sideways",false);
        set(c+"/share","bad",false);
        for(const auto* key:{"roadId","roadDirection","streetSegmentId","streetSegmentDirection"})set(c+"/"+key,"x",false);
        for(const auto* key:{"zoneIds","cells"}) {missing(m,key,false);set(m+"/"+key,nullptr,false);set(m+"/"+key,7,false);}
        set(m+"/zoneIds/0",7,false);set(m+"/cells/0",7,false);set(m+"/cells/0/0","bad",false);
        missing(m,"compositionId",false);set(m+"/compositionId",nullptr,false);set(m+"/compositionId",7,false);
        missing(m,"startTime",false);missing(m,"endTime",false);set(m+"/startTime","bad",false);set(m+"/endTime",nullptr,false);
        { // modeId replaces compositionId from 49; both, or modeId beside it, is refused
            auto j=base;j["odMatrices"][0].erase("compositionId");j["odMatrices"][0]["modeId"]="car-mode";
            result.push_back({m+" modeId",j,ok && v>=modeFrom});
            j["odMatrices"][0]["modeId"]=7;result.push_back({m+" modeId 7",j,false});
            j["odMatrices"][0]["modeId"]=nullptr;result.push_back({m+" modeId null",j,false});
            set(m+"/modeId","car-mode",false);
        }
        { // periodId replaces both times from 49
            auto j=base;j["odMatrices"][0].erase("startTime");j["odMatrices"][0].erase("endTime");j["odMatrices"][0]["periodId"]="am";
            result.push_back({m+" periodId",j,ok && v>=modeFrom});
            j["odMatrices"][0]["periodId"]=7;result.push_back({m+" periodId 7",j,false});
            j["odMatrices"][0]["periodId"]="am";j["odMatrices"][0]["startTime"]=0;result.push_back({m+" periodId and startTime",j,false});
            j["odMatrices"][0].erase("startTime");j["odMatrices"][0]["endTime"]=60;result.push_back({m+" periodId and endTime",j,false});
            set(m+"/periodId","am",false);
        }
        set(m+"/generatedBy","mode-choice",v>=generatedFrom);set(m+"/generatedBy","survey",false);set(m+"/generatedBy",nullptr,false);
        for(const auto* key:{"zoneId","capacity","initialOccupancy","initialCompositionId","gateServiceTime","dwell"}) {
            missing(l,key,false);set(l+"/"+key,nullptr,false);
        }
        set(l+"/zoneId",7,false);set(l+"/initialCompositionId",7,false);set(l+"/gateServiceTime","bad",false);
        set(l+"/capacity",-1,false);set(l+"/capacity",4294967296ULL,false);set(l+"/capacity","bad",false);
        set(l+"/initialOccupancy",-1,false);set(l+"/dwell",7,false);set(l+"/dwell/0",Json::array({0}),false);
        set(l+"/dwell/0",Json::array({0,"bad"}),false);
        // Structural success does not promise the model's checks (zoneLayerIssues).
        set(c+"/share",-1,true);set(m+"/cells",Json::array(),true);set(z+"/polygon",Json::array(),true);
    }
    for(int v=streetFrom;v<=kLatestSchema;++v) {
        const auto base=street(v);const auto id=v<renamed?"roadId":"streetSegmentId",direction=v<renamed?"roadDirection":"streetSegmentDirection";
        const auto oldId=v<renamed?"streetSegmentId":"roadId",oldDirection=v<renamed?"streetSegmentDirection":"roadDirection";
        const std::string c="/zoneConnectors/0";
        const auto set=[&](const std::string& path,Json replacement,bool valid) {
            auto j=base;j[Json::json_pointer(path)]=std::move(replacement);result.push_back({"street "+path,std::move(j),valid});
        };
        const auto missing=[&](const char* key,bool valid) {
            auto j=base;j[Json::json_pointer(c)].erase(key);result.push_back({"street "+c+" missing "+key,std::move(j),valid});
        };
        result.push_back({"street minimal",base,true});
        missing(direction,false);missing(id,false);missing("direction",false);missing("share",false);
        set(c+"/"+id,"",false);set(c+"/"+id,nullptr,false);set(c+"/"+id,7,false);
        set(c+"/"+direction,"backward",true);set(c+"/"+direction,"sideways",false);set(c+"/"+direction,nullptr,false);
        set(c+"/linkId","x",false);set(c+"/extra",1,false);set(c+"/"+oldId,"west",false);set(c+"/"+oldDirection,"forward",false);
        {auto j=base;auto& row=j[Json::json_pointer(c)];row[oldId]=row.at(id);row[oldDirection]=row.at(direction);row.erase(id);row.erase(direction);
            result.push_back({"street other version's keys",std::move(j),false});}
    }
    return result;
}
}
