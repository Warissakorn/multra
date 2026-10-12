#include "route_markers.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace trafficsim {
std::vector<Point> routeMarkerGeometry(const Network& network,const std::string& id) {
    for(const auto& link:network.links)if(link.id==id)return link.geometry;
    for(const auto& connector:network.connectors)if(connector.id==id) {
        try {return connectorCentreline(network,connector);}catch(const std::exception&){return {};}
    }
    return {};
}
Json routeEndMarkersJson(const ProjectDocument& d) {
    auto result=Json::object();
    for(const auto& [id,marker]:d.routeEndMarkers)
        result[id]={{"objectId",marker.objectId},{"station",marker.station}};
    return result;
}
void parseRouteEndMarkers(const Json& json,int schema,ProjectDocument& d) {
    if(!json.contains("routeEndMarkers"))return;
    if(schema<30)throw std::invalid_argument("UNSUPPORTED_FIELD: routeEndMarkers");
    const auto& markers=json.at("routeEndMarkers");
    if(!markers.is_object())throw std::invalid_argument("EDIT_ROUTE_MARKER");
    for(auto entry=markers.begin();entry!=markers.end();++entry) {
        const auto& value=entry.value();
        if(entry.key().empty() || !value.is_object() || value.size()!=2 || !value.contains("objectId") ||
           !value.at("objectId").is_string() || !value.contains("station") || !value.at("station").is_number())
            throw std::invalid_argument("EDIT_ROUTE_MARKER");
        d.routeEndMarkers[entry.key()]={value.at("objectId").get<std::string>(),value.at("station").get<double>()};
    }
}
void validateRouteEndMarkers(const ProjectDocument& d) {
    for(const auto& [id,marker]:d.routeEndMarkers) {
        const Route* route=nullptr;
        if(d.definition)for(const auto& value:d.definition->routes)if(value.id==id)route=&value;
        const auto geometry=routeMarkerGeometry(d.network,marker.objectId);
        if(!route || route->segmentIds.empty() ||
           std::find(route->segmentIds.begin(),route->segmentIds.end(),marker.objectId)==route->segmentIds.end() ||
           geometry.size()<2 || !std::isfinite(marker.station) || marker.station<0 || marker.station>polylineLength(geometry))
            throw std::invalid_argument("EDIT_ROUTE_MARKER");
        if(route->segmentIds.size()==1 && d.definition)for(const auto& decision:d.definition->routingDecisions)
            if(decision.linkId==marker.objectId && decision.position)
                for(const auto& choice:decision.routes)if(choice.routeId==id && marker.station<*decision.position)
                    throw std::invalid_argument("EDIT_ROUTE_MARKER");
    }
}
}
