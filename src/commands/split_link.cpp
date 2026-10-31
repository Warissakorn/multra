#include "network_commands.hpp"
#include "pocket_commands.hpp"
#include "detail.hpp"
#include <algorithm>
#include <cmath>
#include <map>
#include <stdexcept>

namespace multra {
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
std::string splitSpan(ProjectDocument& d, const std::string& id, double near, double far,
                      std::optional<PocketSide> pocket, bool widening) {
    const auto original = editableLink(d, id);
    const auto total = polylineLength(original.geometry);
    if (pocket && original.lanes.size() >= 12) throw std::invalid_argument("EDIT_LANES");
    // An attachment in the continuity/taper span cannot remain on either child link.
    // The station is already measured along this link's own geometry, so the cut compares directly.
    for(const auto& c:d.network.connectors)for(const auto& ref:{c.from,c.to})
        if(ref.linkId==id && ref.station && *ref.station>near && *ref.station<far)
            throw std::invalid_argument("EDIT_SPLIT_ATTACHMENT");
    if(d.definition)for(const auto& [route,end]:d.definition->routeDestinations)
        if(end.objectId==id && end.station>near && end.station<far)
            throw std::invalid_argument("EDIT_ROUTE_DESTINATION_INVALID");
    if (d.definition) for (const auto& decision : d.definition->routingDecisions)
        if (decision.linkId == id && decision.position && *decision.position >= near && *decision.position < far)
            throw std::invalid_argument("EDIT_SPLIT_CONTROL");
    if (d.definition) for (const auto& speed : d.definition->speedDecisions) // B-6, as a routing decision
        if (speed.linkId == id && speed.station >= near && speed.station < far)
            throw std::invalid_argument("EDIT_SPLIT_CONTROL");
    if (d.definition) for (const auto& area : d.definition->reducedSpeedAreas) // B-6 §7: a cut inside an area
        if (area.linkId == id && area.from < far && area.to > near)
            throw std::invalid_argument("EDIT_SPLIT_CONTROL");
    detail::checkSplitControls(d.network, id, near, far); // M3.2.2b: refuse before changing anything
    auto downstream = original;
    downstream.generated = false; // A manual split is refined; J3b marks its own generated pieces.
    downstream.id = allocateId(d, "link");
    downstream.streetSegmentId.reset(); // S3.3: both pieces leave the StreetSegment (the upstream one through changeGeometry)
    downstream.geometry = section(original.geometry, far, total);
    std::map<std::string, std::pair<std::string, std::string>> replacements;
    for (auto& lane : downstream.lanes) {
        const auto old = lane.id;
        lane.id = allocateId(d, "lane");
        replacements[old] = {allocateId(d, "connector"), lane.id};
    }
    if (pocket) {
        const bool kerb = *pocket == PocketSide::kerb;
        auto lanes = downstream.lanes;
        lanes.insert(kerb ? lanes.begin() : lanes.end(),
                     {allocateId(d,"lane"), (kerb ? original.lanes.front() : original.lanes.back()).width});
        replaceLaneBundle(downstream,std::move(lanes),kerb);
    }
    editableLink(d, id).geometry = section(original.geometry, 0, near);
    d.network.links.push_back(downstream);
    // Stations carry across the cut by arithmetic alone: the upstream child's polyline is a
    // prefix of the original, and the downstream child starts at the far side of the span.
    for(auto& c:d.network.connectors)for(auto* ref:{&c.from,&c.to})if(ref->linkId==id) {
        if(!ref->station) {
            if(ref==&c.from)*ref={downstream.id,replacements.at(ref->laneId).second};
            continue;
        }
        if(*ref->station>=far) {
            ref->linkId=downstream.id;ref->laneId=replacements.at(ref->laneId).second;
            ref->station=*ref->station-(far);
        }
    }
    std::map<std::string, std::string> laneMap;
    for (const auto& [old, pair] : replacements) laneMap[old] = pair.second;
    detail::splitControls(d, id, downstream.id, far, laneMap);
    if(d.definition)for(auto& [route,end]:d.definition->routeDestinations)
        if(end.objectId==id && end.station>=far) {
            end.objectId=downstream.id;end.station-=far;
        }
    // M8.2: a zone connector sits at a Link end; the upstream piece keeps the start and the id.
    for(auto& c:d.zones.zoneConnectors)
        if(c.linkId==id && c.direction==ZoneDirection::destination)c.linkId=downstream.id;
    // D383: Node analysis names an in-leg by the Link ending at the Node, so its rows follow that end;
    // an out-leg starts there and keeps the id.
    for (auto& node : d.network.nodes) {
        if (node.analysis) {
            for (auto& t : node.analysis->turns) if (t.in == id) t.in = downstream.id;
            for (auto& r : node.analysis->approaches) if (r.link == id) r.link = downstream.id;
        }
        if (auto direction = node.approachDirections.extract(id); !direction.empty()) {
            direction.key() = downstream.id; node.approachDirections.insert(std::move(direction));
        }
    }
    // Existing lane footprints are unchanged, so their external attachments still fit.
    changeGeometry(d, id, editableLink(d, id).geometry);
    std::map<std::string, ConnectorPath> bridges;
    for (const auto& lane : original.lanes) {
        const auto& [connector, nextLane] = replacements.at(lane.id);
        Connector c{connector, {id, lane.id}, {downstream.id, nextLane}, {},1,1,original.level,original.displayType};
        const bool kerb = pocket && *pocket == PocketSide::kerb;
        const bool outer = lane.id == (kerb ? original.lanes.front() : original.lanes.back()).id;
        if (widening && outer) {
            c.toLaneCount = 2;
            if (kerb) c.to.laneId = downstream.lanes.front().id;
            else c.laneChangeSide = d.network.drivingSide == DrivingSide::left ? LaneSide::right : LaneSide::left;
        }
        const auto a = laneAttachment(d.network,c.from,true), b = laneAttachment(d.network,c.to,false);
        if (widening) {
            // Preserve the reference spine across the taper. Its station weights are also what
            // approachLayout uses to remove the endpoint lane offsets and measure taper length.
            c.geometry = section(original.geometry,near,far);
            double station = 0;
            c.laneBlend.resize(c.geometry.size());
            const auto first = c.geometry.front(), last = c.geometry.back();
            for (std::size_t j = 1; j < c.geometry.size(); ++j) {
                station += std::hypot(c.geometry[j].x-c.geometry[j-1].x,c.geometry[j].y-c.geometry[j-1].y);
                c.laneBlend[j] = station/(far-near);
            }
            c.laneBlend.front() = 0; c.laneBlend.back() = 1;
            for (std::size_t j = 0; j < c.geometry.size(); ++j) {
                const double t = c.laneBlend[j];
                c.geometry[j].x += (a.x-first.x)*(1-t)+(b.x-last.x)*t;
                c.geometry[j].y += (a.y-first.y)*(1-t)+(b.y-last.y)*t;
            }
            c.geometry.front() = a; c.geometry.back() = b;
            c.generated = true;
        } else c.geometry = {a,b};
        d.network.connectors.push_back(c);
        if (widening) {
            const auto paths = connectorPaths(d.network,c);
            bridges[lane.id] = paths[outer && kerb ? 1 : 0];
        } else bridges[lane.id] = {c.id,c.from,c.to,c.geometry};
    }
    // Classify by centreline station, then project the original world point onto the new
    // owning lane/span. At the first cut boundary the upstream lane owns the head; at
    // the second boundary the downstream lane owns it. Interior gap heads stay on the
    // explicit connector, never silently shifted to a lane endpoint.
    for(auto& h:d.network.signalHeads) if(h.connectorId.empty() && h.lane.linkId==id) {
        const auto oldLane=h.lane.laneId;
        const auto world=pointAlong(laneGeometry(original,oldLane,d.network.drivingSide),h.position);
        const double station=stationOfClosestPoint(original.geometry,world);
        const auto& nextLane=replacements.at(oldLane).second;
        std::vector<Point> geometry;
        if(station<=near) geometry=laneGeometry(editableLink(d,id),oldLane,d.network.drivingSide);
        else if(station>=far) {
            h.lane={downstream.id,nextLane};geometry=laneGeometry(downstream,nextLane,d.network.drivingSide);
        } else {
            h.connectorId=bridges.at(oldLane).id; h.lane={};
            geometry=bridges.at(oldLane).geometry;
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
    if (d.definition) {
        auto& def = *d.definition;
        for (auto& speed : def.speedDecisions)
            if (speed.linkId == id && speed.station >= far) { speed.linkId = downstream.id; speed.station -= far; }
        for (auto& area : def.reducedSpeedAreas) {
            if (area.linkId != id || area.from < far) continue;
            area.linkId = downstream.id; area.from -= far; area.to -= far;
            for (auto& lane : area.laneIds) if (const auto to = laneMap.find(lane); to != laneMap.end()) lane = to->second;
        }
        std::map<std::string, std::string> suffixes;
        for (auto& decision : def.routingDecisions) {
            if (decision.linkId != id || !decision.position || *decision.position < far) continue;
            decision.linkId = downstream.id; *decision.position -= far;
            for (auto& input : def.inputs) if (input.routingDecisionId == decision.id) {
                input.routingDecisionId.clear(); input.linkId = id; // source stays on the original entry
            }
            for (auto& entry : decision.routes) {
                if (entry.destinationLinkId == id) entry.destinationLinkId = downstream.id;
                if (entry.routeId.empty()) continue;
                const auto originalId = entry.routeId;
                if (!suffixes.contains(originalId)) {
                    const auto found = std::find_if(def.routes.begin(), def.routes.end(),
                        [&](const auto& route) { return route.id == originalId; });
                    if (found == def.routes.end()) continue; // validation names a missing route
                    auto route = *found;
                    const auto begin = std::find(route.segmentIds.begin(), route.segmentIds.end(), downstream.id);
                    // A route that never reaches the moved station cannot start there: refuse, not empty it.
                    if (begin == route.segmentIds.end()) throw std::invalid_argument("ROUTING_DECISION_MIXED_ORIGIN");
                    route.segmentIds.erase(route.segmentIds.begin(), begin);
                    route.id = allocateId(d, "route");
                    suffixes.emplace(originalId, route.id);
                    if (const auto marker = def.routeDestinations.find(originalId); marker != def.routeDestinations.end() &&
                        std::find(route.segmentIds.begin(), route.segmentIds.end(), marker->second.objectId) != route.segmentIds.end())
                        def.routeDestinations.emplace(route.id, marker->second);
                    def.routes.push_back(std::move(route)); // original full route remains for fixed inputs
                }
                entry.routeId = suffixes.at(originalId);
            }
        }
    }
    return downstream.id;
}
} // namespace
std::string splitLink(ProjectDocument& d, const std::string& id, double distance, bool pocket) {
    const double total = polylineLength(editableLink(d,id).geometry);
    if (!std::isfinite(distance) || distance <= .2 || distance >= total-.2)
        throw std::invalid_argument("EDIT_SPLIT_RANGE");
    return splitSpan(d,id,distance-.1,distance+.1,pocket ? std::optional(PocketSide::far) : std::nullopt,false);
}
std::string addTurnPocket(ProjectDocument& d, const std::string& id, double storage, double taper, PocketSide side) {
    const double total = polylineLength(editableLink(d,id).geometry);
    if (!std::isfinite(storage) || !std::isfinite(taper) || storage < kMinSectionLength ||
        taper < kMinSectionLength || storage+taper >= total-kMinSectionLength ||
        (side != PocketSide::kerb && side != PocketSide::far))
        throw std::invalid_argument("EDIT_SPLIT_RANGE");
    const double far = total-storage;
    return splitSpan(d,id,far-taper,far,side,true);
}
}
