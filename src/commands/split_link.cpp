#include "network_commands.hpp"
#include "detail.hpp"
#include <algorithm>
#include <cmath>
#include <map>
#include <stdexcept>

namespace trafficsim {
namespace {
std::vector<Point> section(const std::vector<Point>& geometry, double start, double end) {
    std::vector<Point> result{pointAlong(geometry, start)};
    double distance = 0;
    for (std::size_t i = 1; i < geometry.size(); ++i) {
        distance += std::hypot(geometry[i].x - geometry[i-1].x, geometry[i].y - geometry[i-1].y);
        if (distance > start && distance < end) result.push_back(geometry[i]);
    }
    result.push_back(pointAlong(geometry, end)); return result;
}
}
std::string splitLink(ProjectDocument& d, const std::string& id, double distance, bool pocket) {
    const auto original = editableLink(d, id);
    const auto total = polylineLength(original.geometry);
    // A 0.2 m connector span keeps runtime segments strictly positive, even on a straight road.
    if (!std::isfinite(distance) || distance <= 0.2 || distance >= total - 0.2)
        throw std::invalid_argument("EDIT_SPLIT_RANGE");
    if (pocket && original.lanes.size() >= 12) throw std::invalid_argument("EDIT_LANES");
    // An attachment in the small continuity span cannot remain on either child link.
    // The station is already measured along this link's own geometry, so the cut compares directly.
    for(const auto& c:d.network.connectors)for(const auto& ref:{c.from,c.to})
        if(ref.linkId==id && ref.station && *ref.station>distance-.1 && *ref.station<distance+.1)
            throw std::invalid_argument("EDIT_SPLIT_ATTACHMENT");
    if(d.definition)for(const auto& [route,end]:d.definition->routeDestinations)
        if(end.objectId==id && end.station>distance-.1 && end.station<distance+.1)
            throw std::invalid_argument("EDIT_ROUTE_DESTINATION_INVALID");
    if (d.definition) for (const auto& decision : d.definition->routingDecisions)
        if (decision.linkId == id && decision.position && *decision.position > distance-.1 && *decision.position < distance+.1)
            throw std::invalid_argument("EDIT_SPLIT_ROUTING_DECISION");
    detail::checkSplitControls(d.network, id, distance); // M3.2.2b: refuse before changing anything
    auto downstream = original;
    downstream.id = allocateId(d, "link");
    downstream.geometry = section(original.geometry, distance + 0.1, total);
    std::map<std::string, std::pair<std::string, std::string>> replacements;
    for (auto& lane : downstream.lanes) {
        const auto old = lane.id;
        lane.id = allocateId(d, "lane");
        replacements[old] = {allocateId(d, "connector"), lane.id};
    }
    if (pocket) {
        auto lanes=downstream.lanes;lanes.push_back({allocateId(d,"lane"),original.lanes.back().width});
        replaceLaneBundle(downstream,std::move(lanes),false);
    }
    editableLink(d, id).geometry = section(original.geometry, 0, distance - 0.1);
    d.network.links.push_back(downstream);
    // Stations carry across the cut by arithmetic alone: the upstream child's polyline is a
    // prefix of the original, and the downstream child starts at the far side of the span.
    for(auto& c:d.network.connectors)for(auto* ref:{&c.from,&c.to})if(ref->linkId==id) {
        if(!ref->station) {
            if(ref==&c.from)*ref={downstream.id,replacements.at(ref->laneId).second};
            continue;
        }
        if(*ref->station>=distance+.1) {
            ref->linkId=downstream.id;ref->laneId=replacements.at(ref->laneId).second;
            ref->station=*ref->station-(distance+.1);
        }
    }
    std::map<std::string, std::string> laneMap;
    for (const auto& [old, pair] : replacements) laneMap[old] = pair.second;
    detail::splitControls(d, id, downstream.id, distance, laneMap);
    if(d.definition)for(auto& [route,end]:d.definition->routeDestinations)
        if(end.objectId==id && end.station>=distance+.1) {
            end.objectId=downstream.id;end.station-=distance+.1;
        }
    if (d.definition) for (auto& decision : d.definition->routingDecisions)
        if (decision.linkId == id && decision.position && *decision.position >= distance+.1) {
            decision.linkId = downstream.id; *decision.position -= distance+.1;
            for (auto& entry : decision.routes) if (entry.destinationLinkId == id) entry.destinationLinkId = downstream.id;
            // An input targeting the decision entered on the original Link, not at its new station.
            for (auto& input : d.definition->inputs) if (input.routingDecisionId == decision.id) {
                input.routingDecisionId.clear(); input.linkId = id;
            }
        }
    // M8.2: a zone connector sits at a Link end; the upstream piece keeps the start and the id.
    for(auto& c:d.zones.zoneConnectors)
        if(c.linkId==id && c.direction==ZoneDirection::destination)c.linkId=downstream.id;
    // Reanchor external connectors, then create explicit one-to-one continuity connectors.
    changeGeometry(d, id, editableLink(d, id).geometry);
    for (const auto& lane : original.lanes) {
        const auto& [connector, nextLane] = replacements.at(lane.id);
        const auto a = laneGeometry(editableLink(d, id), lane.id, d.network.drivingSide).back();
        const auto b = laneGeometry(downstream, nextLane, d.network.drivingSide).front();
        d.network.connectors.push_back({connector, {id, lane.id}, {downstream.id, nextLane}, {a, b},1,1,original.level,original.displayType});
    }
    // Classify by centreline station, then project the original world point onto the new
    // owning lane/span. At the first cut boundary the upstream lane owns the head; at
    // the second boundary the downstream lane owns it. Interior gap heads stay on the
    // explicit connector, never silently shifted to a lane endpoint.
    for(auto& h:d.network.signalHeads) if(h.connectorId.empty() && h.lane.linkId==id) {
        const auto oldLane=h.lane.laneId;
        const auto world=pointAlong(laneGeometry(original,oldLane,d.network.drivingSide),h.position);
        const double station=stationOfClosestPoint(original.geometry,world);
        const auto& [connector,nextLane]=replacements.at(oldLane);
        std::vector<Point> geometry;
        if(station<=distance-0.1) geometry=laneGeometry(editableLink(d,id),oldLane,d.network.drivingSide);
        else if(station>=distance+0.1) {
            h.lane={downstream.id,nextLane};geometry=laneGeometry(downstream,nextLane,d.network.drivingSide);
        } else {
            h.connectorId=connector;h.lane={};
            for(const auto& c:d.network.connectors)if(c.id==connector)geometry=c.geometry;
        }
        h.position=stationOfClosestPoint(geometry,world);
    }
    if (d.definition) for (auto& r : d.definition->routes) {
        std::vector<std::string> ids;
        for (const auto& s : r.segmentIds) {
            ids.push_back(s);
            // A route names objects, so a split adds the new downstream Link after the one that
            // was cut. The bridging Connectors are one per lane and no single one of them could
            // stand in the chain; routeLaneChains finds each lane's own between two Links.
            if (s == id) ids.push_back(downstream.id);
        }
        r.segmentIds = std::move(ids);
    }
    // Decision routes start on their decision's Link. Keep full authored routes for fixed inputs,
    // and clone only the downstream suffix used by the moved decision. Interval/type weights are
    // route-order data and stay untouched; destination stations follow the new route id.
    if (d.definition) for (auto& decision : d.definition->routingDecisions) if (decision.linkId == downstream.id) {
        std::map<std::string,std::string> suffixes;
        for (auto& entry : decision.routes) if (!entry.routeId.empty()) {
            const auto old = entry.routeId;
            if (!suffixes.contains(old)) {
                const auto route = std::find_if(d.definition->routes.begin(),d.definition->routes.end(),
                                               [&](const auto& r) { return r.id == old; });
                if (route == d.definition->routes.end()) throw std::invalid_argument("UNKNOWN_ROUTE");
                auto suffix = *route;
                const auto start = std::find(suffix.segmentIds.begin(),suffix.segmentIds.end(),downstream.id);
                if (start == suffix.segmentIds.end()) throw std::invalid_argument("ROUTING_DECISION_MIXED_ORIGIN");
                suffix.segmentIds.erase(suffix.segmentIds.begin(),start);
                suffix.id = allocateId(d,"route"); suffixes[old] = suffix.id;
                if (const auto end=d.definition->routeDestinations.find(old);end!=d.definition->routeDestinations.end())
                    d.definition->routeDestinations[suffix.id]=end->second;
                d.definition->routes.push_back(std::move(suffix));
            }
            entry.routeId = suffixes.at(old);
        }
    }
    return downstream.id;
}
}
