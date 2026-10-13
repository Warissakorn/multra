#include "route_destination.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace trafficsim {
std::vector<Point> routeDestinationGeometry(const Network& n,const std::string& id) {
    for(const auto& link:n.links)if(link.id==id)return link.geometry;
    return objectGeometry(n,id);
}
void validateRouteDestinations(const ProjectDocument& d) {
    if(!d.definition)return;
    for(const auto& [id,end]:d.definition->routeDestinations) {
        const auto route=std::find_if(d.definition->routes.begin(),d.definition->routes.end(),[&](const auto& r){return r.id==id;});
        const auto geometry=routeDestinationGeometry(d.network,end.objectId);
        if(route==d.definition->routes.end() ||
           std::find(route->segmentIds.begin(),route->segmentIds.end(),end.objectId)==route->segmentIds.end() ||
           geometry.size()<2 || !std::isfinite(end.station) || end.station<0 || end.station>polylineLength(geometry))
            throw std::invalid_argument("EDIT_ROUTE_DESTINATION_INVALID");
    }
}
void parseRouteDestinations(const Json& value,int schema,AuthoringDefinition& def) {
    if(!value.contains("routeDestinations"))return;
    if(schema<30)throw std::invalid_argument("UNSUPPORTED_FIELD: routeDestinations");
    const auto& entries=value.at("routeDestinations");
    if(!entries.is_object())throw std::invalid_argument("EDIT_ROUTE_DESTINATION_INVALID");
    for(const auto& [id,entry]:entries.items()) {
        if(id.empty() || !entry.is_object() || entry.size()!=2 || !entry.contains("objectId") ||
           !entry.at("objectId").is_string() || !entry.contains("station") || !entry.at("station").is_number())
            throw std::invalid_argument("EDIT_ROUTE_DESTINATION_INVALID");
        def.routeDestinations.emplace(id,RouteDestination{entry.at("objectId").get<std::string>(),entry.at("station").get<double>()});
    }
}
void addRouteDestinationsJson(const AuthoringDefinition& def,Json& value) {
    if(def.routeDestinations.empty())return;
    Json entries=Json::object();
    for(const auto& [id,end]:def.routeDestinations)entries[id]={{"objectId",end.objectId},{"station",end.station}};
    value["routeDestinations"]=std::move(entries);
}
}
