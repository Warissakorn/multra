#include "network_fields.hpp"
#include "node_codec.hpp"
#include "../core/validate.hpp"
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace multra::networkCodec {
ControlPathRef readControlPath(const Json& j,const std::string& path,int version) {
    knownFields(j,Object::controlPath,path,version);
    const auto text=[&](const char* key) {
        return present(j,key)?value(j,Object::controlPath,key).get<std::string>():std::string{};
    };
    // Non-object values have always decoded as an empty reference; model validation
    // reports INVALID_CONTROL_PATH. Do not add a structural object check here.
    return {text("linkId"),text("laneId"),text("connectorId"),text("fromLaneId"),text("toLaneId")};
}
namespace {
ConflictSide readSide(const Json& j,const std::string& path,int version) {
    knownFields(j,Object::conflictSide,path,version);
    const auto field=[&](const char* key)->const Json& {return value(j,Object::conflictSide,key);};
    ConflictSide side{readControlPath(field("path"),path+".path",version),field("entryStation").get<double>(),
        field("exitStation").get<double>(),field("waitingLineId").get<std::string>()};
    // D594: empty is the derived line from schema 79; before it, a side always named its line.
    if(side.waitingLineId.empty() && version<derivedLineVersion)
        throw ValidationError({{"UNKNOWN_WAITING_LINE",path+".waitingLineId"}});
    return side;
}
// D618 (YIELD_IMPATIENCE §3): the three keys together or none; a lone one is refused at a missing one.
std::optional<GapReduction> readGapReduction(const Json& j,Object object,const std::string& path) {
    const char* keys[]={"finalGapTime","gapReductionStart","gapReductionEnd"};
    int count=0;
    for(const char* key:keys)count+=present(j,key)?1:0;
    if(count==0)return std::nullopt;
    for(const char* key:keys)if(!present(j,key))throw ValidationError({{"INVALID_PRIORITY_RULE",path+"."+key}});
    return GapReduction{value(j,object,"finalGapTime").get<double>(),value(j,object,"gapReductionStart").get<double>(),
                        value(j,object,"gapReductionEnd").get<double>()};
}
}
RightOfWay readRightOfWay(const Json& j,int version) {
    knownFields(j,Object::rightOfWay,"network.rightOfWay",version);
    RightOfWay row;
    if(present(j,"frontStopDistance"))row.frontStopDistance=value(j,Object::rightOfWay,"frontStopDistance").get<double>();
    if(present(j,"waitingLines"))for(const auto& w:value(j,Object::rightOfWay,"waitingLines")) {
        const auto path="rightOfWay.waitingLines["+std::to_string(row.waitingLines.size())+"]";
        knownFields(w,Object::waitingLine,path,version);
        const auto field=[&](const char* key)->const Json& {return value(w,Object::waitingLine,key);};
        const auto& point=field("point");
        knownFields(point,Object::controlPoint,path+".point",version);
        row.waitingLines.push_back({field("id").get<std::string>(),present(w,"name")?field("name").get<std::string>():"",
            {readControlPath(value(point,Object::controlPoint,"path"),path+".point.path",version),
                value(point,Object::controlPoint,"station").get<double>()}});
        row.waitingLines.back().generated=generatedFlag(w,path,version);
    }
    if(present(j,"conflictAreas"))for(const auto& a:value(j,Object::rightOfWay,"conflictAreas")) {
        const auto path="rightOfWay.conflictAreas["+std::to_string(row.conflictAreas.size())+"]";
        knownFields(a,Object::conflictArea,path,version);
        const auto field=[&](const char* key)->const Json& {return value(a,Object::conflictArea,key);};
        row.conflictAreas.push_back({field("id").get<std::string>(),present(a,"name")?field("name").get<std::string>():"",
            conflictKindFromName(field("kind").get<std::string>()),readSide(field("first"),path+".first",version),
            readSide(field("second"),path+".second",version),conflictPriorityFromName(field("priority").get<std::string>())});
        row.conflictAreas.back().generated=generatedFlag(a,path,version);
    }
    if(present(j,"priorityRules"))for(const auto& r:value(j,Object::rightOfWay,"priorityRules")) {
        const auto path="rightOfWay.priorityRules["+std::to_string(row.priorityRules.size())+"]";
        knownFields(r,Object::priorityRule,path,version);
        const auto field=[&](const char* key)->const Json& {return value(r,Object::priorityRule,key);};
        // D606, schema 83: a waiting line with markers instead of an area; the other form's keys
        // are refused at the rule (the model reports a rule carrying both, INVALID_PRIORITY_RULE).
        const bool markers=version>=priorityMarkerVersion && present(r,"waitingLineId");
        if(markers)for(const char* key:{"conflictAreaId","gapTime","headway"})
            if(r.contains(key))throw ValidationError({{"INVALID_PRIORITY_RULE",path+"."+key}});
        if(!markers && version>=priorityMarkerVersion && r.contains("markers"))throw ValidationError({{"INVALID_PRIORITY_RULE",path+".markers"}});
        row.priorityRules.push_back({field("id").get<std::string>(),present(r,"name")?field("name").get<std::string>():"",
            markers?"":field("conflictAreaId").get<std::string>(),markers?0:field("gapTime").get<double>(),markers?0:field("headway").get<double>()});
        auto& rule=row.priorityRules.back();
        rule.generated=generatedFlag(r,path,version);
        if(present(r,"visibility"))rule.visibility=field("visibility").get<double>(); // D607
        if(!markers)rule.gapReduction=readGapReduction(r,Object::priorityRule,path); // D618
        const auto texts=[](const Json& list,const char* key) {
            std::vector<std::string> result;
            for(const auto& id:list) {
                if(!id.is_string())throw std::invalid_argument(std::string("Expected text: ")+key);
                result.push_back(id.get<std::string>());
            }
            return result;
        };
        if(!markers)continue;
        rule.waitingLineId=field("waitingLineId").get<std::string>();
        if(present(r,"vehicleClassIds"))rule.vehicleClassIds=texts(field("vehicleClassIds"),"vehicleClassIds");
        for(const auto& m:field("markers")) {
            const auto at=path+".markers["+std::to_string(rule.markers.size())+"]";
            knownFields(m,Object::priorityMarker,at,version);
            const auto mark=[&](const char* key)->const Json& {return value(m,Object::priorityMarker,key);};
            const auto& point=mark("point");
            knownFields(point,Object::controlPoint,at+".point",version);
            PriorityMarker marker{{readControlPath(value(point,Object::controlPoint,"path"),at+".point.path",version),
                value(point,Object::controlPoint,"station").get<double>()},mark("gapTime").get<double>(),mark("headway").get<double>(),{},{}};
            if(present(m,"maxSpeed"))marker.maxSpeed=mark("maxSpeed").get<double>();
            if(present(m,"vehicleClassIds"))marker.vehicleClassIds=texts(mark("vehicleClassIds"),"vehicleClassIds");
            marker.gapReduction=readGapReduction(m,Object::priorityMarker,at); // D618
            rule.markers.push_back(std::move(marker));
        }
    }
    if(version>=15 && present(j,"stopControls"))for(const auto& c:value(j,Object::rightOfWay,"stopControls")) {
        const auto path="rightOfWay.stopControls["+std::to_string(row.stopControls.size())+"]";
        knownFields(c,Object::stopControl,path,version);
        const auto field=[&](const char* key)->const Json& {return value(c,Object::stopControl,key);};
        StopControl control{field("id").get<std::string>(),present(c,"name")?field("name").get<std::string>():"",
            field("waitingLineId").get<std::string>(),stopModeFromName(field("mode").get<std::string>()),{}};
        for(const auto& area:field("conflictAreaIds")) {
            if(!area.is_string())throw std::invalid_argument("Expected text: conflictAreaIds");
            control.conflictAreaIds.push_back(area.get<std::string>());
        }
        control.generated=generatedFlag(c,path,version);row.stopControls.push_back(std::move(control));
    }
    return row;
}
}
