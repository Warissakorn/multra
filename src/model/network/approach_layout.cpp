#include "approach_layout.hpp"
#include "connector_lane_mapping.hpp"
#include "polygon.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <set>

namespace multra {
namespace {
const Link& linkOf(const Network& n, const std::string& id) {
    return *std::find_if(n.links.begin(),n.links.end(),[&](const auto& l) { return l.id == id; });
}
const Connector* connectorOf(const Network& n, const std::string& id) {
    const auto c = std::find_if(n.connectors.begin(),n.connectors.end(),[&](const auto& x) { return x.id == id; });
    return c == n.connectors.end() ? nullptr : &*c;
}
// Remove the first-lane endpoint offsets from the stored Connector, using its own blend weights.
// This recovers the reference spine, including bends, without storing another length or shape.
double referenceLength(const Network& n, const Connector& c) {
    const auto& from = linkOf(n,c.from.linkId); const auto& to = linkOf(n,c.to.linkId);
    const auto first = pointAlong(from.geometry,c.from.station.value_or(polylineLength(from.geometry)));
    const auto last = pointAlong(to.geometry,c.to.station.value_or(0));
    const auto a = laneAttachment(n,c.from,true), b = laneAttachment(n,c.to,false);
    auto spine = c.geometry; const auto weights = connectorBlendWeights(c);
    for (std::size_t j = 0; j < spine.size(); ++j) {
        const double t = weights[j];
        spine[j].x -= (a.x-first.x)*(1-t)+(b.x-last.x)*t;
        spine[j].y -= (a.y-first.y)*(1-t)+(b.y-last.y)*t;
    }
    return polylineLength(spine);
}
// Walk a unique, aligned predecessor chain, so an ordinary split of the pocket does not lose
// its start. A Node-owned Connector ends this walk even when it runs straight through the Node.
std::optional<LayoutPocket> pocketOf(const Network& n, const Link& start, const LayoutLane& lane) {
    const Link* link = &start; std::string laneId = lane.id; double stop = lane.stopLine, storage = 0;
    std::set<std::pair<std::string,std::string>> visited;
    while (visited.emplace(link->id,laneId).second) {
        const Connector* incoming = nullptr; std::size_t path = 0;
        for (const auto& c : n.connectors) {
            if (c.to.linkId != link->id || c.to.station.value_or(0) > stop) continue;
            const auto pairs = connectorLanePairs(n,c);
            for (std::size_t p = 0; p < pairs.size(); ++p) if (pairs[p].to.laneId == laneId) {
                if (incoming) return {}; // no unique start
                incoming = &c; path = p;
            }
        }
        if (!incoming) return {};
        const auto& c = *incoming; const auto& previous = linkOf(n,c.from.linkId);
        for (const auto& node : n.nodes) {
            const auto runs = polygonRuns(c.geometry,node.polygon);
            if (std::any_of(runs.begin(),runs.end(),[](const auto& run) { return run.inside; })) return {};
        }
        const double from = c.from.station.value_or(polylineLength(previous.geometry)), to = c.to.station.value_or(0);
        storage += stop-to;
        const auto widths = connectorLaneWidths(n,c);
        if (widths.source[path] == 0 && widths.target[path] > 0) {
            const auto side = static_cast<int>(path) < connectorLaneShift(n,c) ? PocketSide::kerb : PocketSide::far;
            return LayoutPocket{side,storage,referenceLength(n,c),c.id};
        }
        const auto a = directionAlong(previous.geometry,from,true), b = directionAlong(link->geometry,to,false);
        if (a.x*b.x+a.y*b.y < std::cos(std::numbers::pi/4)) return {};
        storage += referenceLength(n,c);
        const auto pairs = connectorLanePairs(n,c);
        laneId = pairs[path].from.laneId; link = &previous; stop = from;
    }
    return {};
}
constexpr double kAngleEpsilon = 1e-12; // exact boundary angles after trig round-off
}
std::optional<ApproachDirection> approachDirection(const Node& node,const Link& link,double station,const ApproachDirections& overrides) {
    if(const auto x=overrides.find(link.id);x!=overrides.end())return x->second;
    if(const auto x=node.approachDirections.find(link.id);x!=node.approachDirections.end())return x->second;
    const auto d=directionAlong(link.geometry,station,true);
    if(std::abs(std::abs(d.x)-std::abs(d.y))<kAngleEpsilon)return {};
    if(std::abs(d.y)>std::abs(d.x))return d.y>0?ApproachDirection::north:ApproachDirection::south;
    return d.x>0?ApproachDirection::east:ApproachDirection::west;
}
ApproachLayout approachLayout(const Network& n, const Node& node, const ApproachDirections& overrides) {
    ApproachLayout out; out.membership = nodeMembership(n,node);
    const auto& m = out.membership;
    for (const auto& code : m.issues) out.issues.push_back({code,""});
    if (!out.issues.empty()) return out;
    for (const auto& leg : m.inLegs) {
        const auto& link = linkOf(n,leg.linkId);
        LayoutApproach a; a.inLink = link.id;
        for (const auto& lane : link.lanes) a.lanes.push_back({lane.id,lane.width,polylineLength(link.geometry),{}, {}});
        out.approaches.push_back(std::move(a));
    }
    // Same path/lane rule as NODE_ANALYSIS A1/A2: the first Connector's from range, or every
    // in-leg lane when there is no first Connector directly on that in-leg.
    for (std::size_t t = 0; t < m.turns.size(); ++t) {
        const auto& turn = m.turns[t]; auto& a = out.approaches[turn.in];
        for (const auto& path : turn.paths) {
            const Connector* first = nullptr;
            for (const auto& span : path) if ((first = connectorOf(n,span.objectId))) break;
            auto begin = a.lanes.begin(), end = a.lanes.end();
            double stop = polylineLength(linkOf(n,a.inLink).geometry);
            if (first && first->from.linkId == a.inLink) {
                begin = std::find_if(begin,end,[&](const auto& l) { return l.id == first->from.laneId; });
                if (begin != end) end = begin+std::min<std::ptrdiff_t>(first->fromLaneCount,end-begin);
                stop = first->from.station.value_or(stop);
            }
            for (auto lane = begin; lane != end; ++lane) {
                if (std::find(lane->turns.begin(),lane->turns.end(),t) == lane->turns.end()) lane->turns.push_back(t);
                lane->stopLine = std::min(lane->stopLine,stop);
            }
        }
    }
    std::vector<Point> travel;
    std::map<ApproachDirection,std::size_t> compass;
    for (std::size_t i = 0; i < out.approaches.size(); ++i) {
        auto& a = out.approaches[i]; const auto& link = linkOf(n,a.inLink);
        double stop = polylineLength(link.geometry);
        for (auto& lane : a.lanes) { stop = std::min(stop,lane.stopLine); lane.pocket = pocketOf(n,link,lane); }
        const auto d = directionAlong(link.geometry,stop,true); travel.push_back(d);
        a.bearing = std::atan2(d.x,d.y)*180/std::numbers::pi;
        if (a.bearing < 0) a.bearing += 360;
        const auto path = "inLegs["+std::to_string(i)+"]: "+a.inLink;
        const auto direction=approachDirection(node,link,stop,overrides);
        if(!direction) {
            out.issues.push_back({"JUNCTION_DIRECTION_AMBIGUOUS",path}); continue;
        }
        a.direction=*direction;
        if (!compass.emplace(a.direction,i).second) out.issues.push_back({"JUNCTION_DIRECTION_AMBIGUOUS",path});
    }
    for (std::size_t t = 0; t < m.turns.size(); ++t) {
        const auto& turn = m.turns[t]; const auto& outLink = linkOf(n,m.outLegs[turn.out].linkId);
        double station = polylineLength(outLink.geometry);
        for (const auto& path : turn.paths) if (!path.empty()) station = std::min(station,path.back().from);
        const auto a = travel[turn.in], b = directionAlong(outLink.geometry,station,false);
        const double angle = std::atan2(a.x*b.y-a.y*b.x,a.x*b.x+a.y*b.y);
        if (std::abs(angle) >= 3*std::numbers::pi/4-kAngleEpsilon) {
            out.issues.push_back({"JUNCTION_UTURN","turns["+std::to_string(t)+"]: "+turn.name}); continue;
        }
        const auto movement = std::abs(angle) < std::numbers::pi/4-kAngleEpsilon ? JunctionMovement::through
                            : angle > 0 ? JunctionMovement::left : JunctionMovement::right;
        const auto direction = out.approaches[turn.in].direction;
        const char* prefix = direction == ApproachDirection::north ? "NB" : direction == ApproachDirection::east ? "EB"
                           : direction == ApproachDirection::south ? "SB" : "WB";
        const char suffix = movement == JunctionMovement::left ? 'L' : movement == JunctionMovement::through ? 'T' : 'R';
        out.movements.push_back({t,std::string(prefix)+suffix,movement});
    }
    if (!out.issues.empty()) { out.approaches.clear(); out.movements.clear(); }
    return out;
}
}
