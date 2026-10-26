#include "network_fields.hpp"
#include "node_codec.hpp"
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
    return {readControlPath(field("path"),path+".path",version),field("entryStation").get<double>(),
        field("exitStation").get<double>(),field("waitingLineId").get<std::string>()};
}
}
RightOfWay readRightOfWay(const Json& j,int version) {
    knownFields(j,Object::rightOfWay,"network.rightOfWay",version);
    RightOfWay row;
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
        row.priorityRules.push_back({field("id").get<std::string>(),present(r,"name")?field("name").get<std::string>():"",
            field("conflictAreaId").get<std::string>(),field("gapTime").get<double>(),field("headway").get<double>()});
        row.priorityRules.back().generated=generatedFlag(r,path,version);
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
