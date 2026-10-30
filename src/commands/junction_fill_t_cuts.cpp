#include "junction_fill_detail.hpp"
#include "../model/network/coarse_nodes.hpp"
#include <algorithm>
#include <cmath>
namespace multra::junctionDetail {
namespace { constexpr double kProjectionTolerance=1e-6; } // metres, numerical projection only
NodeCuts tLegCuts(Network measured,const Network& original,const Node& node,const std::set<std::string>& major) {
    try {
        const auto legs=nodeLinkLegs(measured,node);std::vector<Point> pavement;
        for(const auto& leg:legs) {
            const auto& l=*std::find_if(measured.links.begin(),measured.links.end(),[&](const auto& l){return l.id==leg.linkId;});
            if(!l.streetSegmentId||!major.contains(*l.streetSegmentId))continue;
            for(const auto edge:{std::size_t{0},l.lanes.size()}) {
                const auto rail=laneBoundaryGeometry(l,edge,measured.drivingSide);pavement.push_back(leg.in?rail.back():rail.front());
            }
        }
        for(const auto& leg:legs) {
            auto& l=*std::find_if(measured.links.begin(),measured.links.end(),[&](const auto& l){return l.id==leg.linkId;});
            if(!l.streetSegmentId||major.contains(*l.streetSegmentId))continue;
            auto& end=leg.in?l.geometry.back():l.geometry.front();
            auto toward=directionAlong(l.geometry,leg.in?polylineLength(l.geometry):0,leg.in);
            if(!leg.in){toward.x=-toward.x;toward.y=-toward.y;}
            double reach=0;for(const auto p:pavement)reach=std::max(reach,(p.x-end.x)*toward.x+(p.y-end.y)*toward.y);
            end.x+=toward.x*reach;end.y+=toward.y*reach;
        }
        auto result=nodeLegCuts(measured,node,{});if(!result.reason.empty())return result;
        // Stations on the extended out-leg have a different origin. Reference guards and
        // published spans must use the actual Link, never the measuring extension.
        for(auto& cut:result.cuts) {
            const auto l=std::find_if(original.links.begin(),original.links.end(),[&](const auto& l){return l.id==cut.linkId;});
            const auto p=cut.in?cut.geometry.back():cut.geometry.front();const double length=polylineLength(l->geometry);
            cut.station=stationOfClosestPoint(l->geometry,p);const auto on=pointAlong(l->geometry,cut.station);
            if(std::hypot(p.x-on.x,p.y-on.y)>kProjectionTolerance)return {"FILL_NODE_CUT_UNMEASURED",{}};
            cut.geometry=polylineSpan(l->geometry,cut.in?0:cut.station,cut.in?cut.station:length);
        }
        return result;
    }catch(const std::exception&){return {"FILL_NODE_CUT_UNMEASURED",{}};}
}
}
