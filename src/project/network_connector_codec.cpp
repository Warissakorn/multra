#include "network_fields.hpp"
#include "node_codec.hpp"
#include "../core/validate.hpp"
#include <nlohmann/json.hpp>
#include <tuple>

namespace multra::networkCodec {
LaneReference readReference(const Json& j,int version) {
    LaneReference result{value(j,Object::reference,"linkId").get<std::string>(),value(j,Object::reference,"laneId").get<std::string>()};
    const auto* key=positionKey(version);
    const auto* wrong=version<stationVersion?"station":"fraction";
    if(j.contains(wrong))throw std::invalid_argument("EDIT_VERSION");
    knownFields(j,Object::reference,"reference",version);
    if(j.contains(key))result.station=value(j,Object::reference,key).get<double>();
    return result;
}
std::vector<Connector> readConnectors(const Json& network,int version) {
    std::vector<Connector> result;
    for(const auto& c:value(network,Object::network,"connectors")) {
        const auto where="connectors["+std::to_string(result.size())+"]";
        knownFields(c,Object::connector,where,version);
        Connector connector{value(c,Object::connector,"id").get<std::string>(),
            readReference(value(c,Object::connector,"from"),version),readReference(value(c,Object::connector,"to"),version),
            readGeometry(c,Object::connector,version)};
        for(const auto& [name,member]:{std::pair{"fromLaneCount",&Connector::fromLaneCount},
                std::pair{"toLaneCount",&Connector::toLaneCount},std::pair{"level",&Connector::level}})
            if(c.contains(name))connector.*member=value(c,Object::connector,name).get<int>();
        if(c.contains("displayType"))connector.displayType=value(c,Object::connector,"displayType").get<std::string>();
        if(present(c,"name"))connector.name=value(c,Object::connector,"name").get<std::string>();
        for(const auto& [name,member,error]:{
                std::tuple{"laneBlend",&Connector::laneBlend,"INVALID_GEOMETRY"},
                std::tuple{"laneWidths",&Connector::laneWidths,"INVALID_WIDTH"}})
            if(c.contains(name))for(const auto& n:value(c,Object::connector,name)) {
                if(!n.is_number())throw std::invalid_argument(error);
                (connector.*member).push_back(n.get<double>());
            }
        if(c.contains("laneMarkings"))for(const auto& m:value(c,Object::connector,"laneMarkings")) {
            if(!m.is_string())throw std::invalid_argument("INVALID_MARKING");
            connector.laneMarkings.push_back(markingFromName(m.get<std::string>()));
        }
        if(version>=laneSideVersion && present(c,"laneChangeSide")) {
            const auto side=value(c,Object::connector,"laneChangeSide").get<std::string>();
            if(side!="left" && side!="right")throw ValidationError({{"EDIT_LANE_RANGE",where+".laneChangeSide"}});
            connector.laneChangeSide=side=="left"?LaneSide::left:LaneSide::right;
        }
        if(version>=behaviourVersion && c.contains("behaviourType"))connector.behaviourTypeId=value(c,Object::connector,"behaviourType").get<std::string>();
        connector.generated=generatedFlag(c,where,version);
        result.push_back(std::move(connector));
    }
    return result;
}
}
