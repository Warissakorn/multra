#include "network_fields.hpp"
#include "node_codec.hpp"
#include <nlohmann/json.hpp>

namespace multra::networkCodec {
std::vector<Point> readGeometry(const Json& item,Object object,int version) {
    std::vector<Point> result;
    for(const auto& p:value(item,object,"geometry")) {
        knownFields(p,Object::point,"geometry["+std::to_string(result.size())+"]",version);
        result.push_back({value(p,Object::point,"x").get<double>(),value(p,Object::point,"y").get<double>()});
    }
    return result;
}
std::vector<Link> readLinks(const Json& network,int version) {
    std::vector<Link> result;
    for(const auto& item:value(network,Object::network,"links")) {
        const auto path="links["+std::to_string(result.size())+"]";
        if(version<62 && item.is_object() && item.contains("generated"))
            throw ValidationError({{"EDIT_UNSUPPORTED_FIELD","network."+path+".generated"}});
        knownFields(item,Object::link,path,version);
        Link link{value(item,Object::link,"id").get<std::string>(),readGeometry(item,Object::link,version),{}};
        for(const auto& lane:value(item,Object::link,"lanes")) {
            knownFields(lane,Object::lane,path+".lanes["+std::to_string(link.lanes.size())+"]",version);
            link.lanes.push_back({value(lane,Object::lane,"id").get<std::string>(),value(lane,Object::lane,"width").get<double>()});
        }
        if(item.contains("level"))link.level=value(item,Object::link,"level").get<int>();
        if(item.contains("laneOffset"))link.laneOffset=value(item,Object::link,"laneOffset").get<double>();
        if(item.contains("displayType"))link.displayType=value(item,Object::link,"displayType").get<std::string>();
        if(present(item,"name"))link.name=value(item,Object::link,"name").get<std::string>();
        if(item.contains("boundaryMarkings"))for(const auto& m:value(item,Object::link,"boundaryMarkings")) {
            if(!m.is_string())throw std::invalid_argument("INVALID_MARKING");
            link.boundaryMarkings.push_back(markingFromName(m.get<std::string>()));
        }
        if(version>=behaviourVersion && item.contains("behaviourType"))link.behaviourTypeId=value(item,Object::link,"behaviourType").get<std::string>();
        link.generated=generatedFlag(item,"network."+path,version);
        result.push_back(std::move(link));
    }
    return result;
}
}
