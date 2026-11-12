// A route's surface tint and its origin/destination bars, stored or previewed (D96, D435, D508,
// D509). The tint lies between the two bars only, inset from the road edges as a conflict area
// is, in its own colour; a preview re-tints in place while the pointer moves along one road.
#include "canvas.hpp"
#include "canvas_style.hpp"
#include "../project/route_destination.hpp"
#include "../model/network/conflict_display.hpp"
#include <QGraphicsPathItem>
#include <QPainterPathStroker>
#include <algorithm>
#include <cmath>

namespace multra {
namespace {
QPainterPath polylinePath(const std::vector<Point>& points) {
    QPainterPath path;
    if(points.empty())return path;
    path.moveTo(points.front().x,points.front().y);
    for(std::size_t i=1;i<points.size();++i)path.lineTo(points[i].x,points[i].y);
    return path;
}
// The chain up to where it last crosses the destination bar (lengthened a little past both rails,
// so a kerb lane's centre still meets it). A chain the bar does not cross is kept whole.
std::vector<Point> cutAtBar(const std::vector<Point>& chain,const RoadCrossbar& bar) {
    QLineF across(bar.first.x,bar.first.y,bar.second.x,bar.second.y);
    const auto grow=across.length()>0?QPointF(across.dx(),across.dy())/across.length()*2.0:QPointF();
    across=QLineF(across.p1()-grow,across.p2()+grow);
    std::optional<std::pair<std::size_t,QPointF>> cut;
    for(std::size_t i=1;i<chain.size();++i) {
        QPointF at;
        if(QLineF(chain[i-1].x,chain[i-1].y,chain[i].x,chain[i].y).intersects(across,&at)==QLineF::BoundedIntersection)cut={{i,at}};
    }
    if(!cut)return chain;
    std::vector<Point> out(chain.begin(),chain.begin()+static_cast<std::ptrdiff_t>(cut->first));
    out.push_back({cut->second.x(),cut->second.y()});
    return out;
}
// D509: like a conflict area's band, half a metre inside the painted rails, at most a fifth of a lane.
double tintInset(const Network& n,const std::vector<std::string>& segments) {
    double narrowest=std::numeric_limits<double>::infinity();
    for(const auto& id:segments) {
        for(const auto& l:n.links)if(l.id==id)for(const auto& lane:l.lanes)narrowest=std::min(narrowest,lane.width);
        for(const auto& c:n.connectors)if(c.id==id)for(const auto w:c.laneWidths)narrowest=std::min(narrowest,w);
    }
    return std::isfinite(narrowest)?std::min(kConflictRailOffset,0.2*narrowest):0;
}
QPainterPath tintPath(const std::vector<std::vector<Point>>& chains,const QPainterPath& roads,double width,
                      const std::optional<RoadCrossbar>& end) {
    QPainterPathStroker stroke;stroke.setWidth(width);stroke.setCapStyle(Qt::FlatCap);
    QPainterPath corridors;
    for(const auto& chain:chains)corridors=corridors.united(stroke.createStroke(polylinePath(end?cutAtBar(chain,*end):chain)));
    return roads.intersected(corridors);
}
double widestLane(const Network& n) {
    double width=0;
    for(const auto& l:n.links)for(const auto& lane:l.lanes)width=std::max(width,lane.width);
    for(const auto& c:n.connectors)for(const auto w:c.laneWidths)width=std::max(width,w);
    return width;
}
std::optional<RoadCrossbar> originBar(const Network& n,const std::string& object,std::optional<double> station) {
    if(station)for(const auto& link:n.links)if(link.id==object) {
        const auto axis=linkCentreline(link,n.drivingSide);
        return roadCrossbar(axis,laneBoundaryGeometry(link,0,n.drivingSide),laneBoundaryGeometry(link,link.lanes.size(),n.drivingSide),
            matchedStation(link.geometry,axis,*station),link.level);
    }
    return objectCrossbar(n,object,false);
}
}
// Where a click or a Ctrl+right release on the hovered Link would put the origin: its station,
// snapped to a placed decision within 10 px; none (the Link's start) within 10 px of the start.
std::optional<double> EditorCanvas::hoverOriginStation() const {
    if(!routeHover_ || !isLink(routeHover_->objectId))return std::nullopt;
    double station=routeHover_->station;
    if(document_->definition)for(const auto& decision:document_->definition->routingDecisions)
        if(decision.linkId==routeHover_->objectId && decision.position && std::abs(*decision.position-station)*viewScale()<=10)station=*decision.position;
    if(station*viewScale()<=10)return std::nullopt;
    return station;
}
// The preview's destination: a traced or released end on the preview's last road, else where a
// Ctrl+right release on the hovered road would place it (none, its far end, within 10 px of it).
std::optional<RouteDestination> EditorCanvas::previewDestination() const {
    const auto& segments=routePreview_.empty()?routeDraft_:routePreview_;
    if(segments.empty())return std::nullopt;
    if(routeEnd_ && routeEnd_->objectId==segments.back())return routeEnd_;
    if(routeTracing_ || routePress_ || segments==routeDraft_ || !routeHover_ || routeHover_->objectId!=segments.back())return std::nullopt;
    const auto geometry=routeDestinationGeometry(document_->network,routeHover_->objectId);
    if(geometry.size()<2 || (polylineLength(geometry)-routeHover_->station)*viewScale()<=10)return std::nullopt;
    return routeHover_;
}
void EditorCanvas::drawRouteOverlay(const std::vector<std::string>& segments,const std::string& id,bool preview) {
    if(segments.empty())return;
    const auto& n=document_->network;
    auto destination=preview?previewDestination():std::optional<RouteDestination>{};
    if(!preview && document_->definition) {
        const auto found=document_->definition->routeDestinations.find(id);
        if(found!=document_->definition->routeDestinations.end())destination=found->second;
    }
    auto position=preview?routeStartPosition_:std::optional<double>{};
    if(!preview && document_->definition)for(const auto& decision:document_->definition->routingDecisions)
        for(const auto& entry:decision.routes)if(entry.routeId==id && decision.position)position=decision.position;
    // Keep D96's compiled spans: tint never extends upstream of a mid-Link arrival.
    // Clip a lane-centre corridor to real road surfaces rather than inventing another road.
    QPainterPath roads;
    for(const auto& object:segments) {
        const auto level=objectLevel(object);
        if(level && levelVisible(*level))roads=roads.united(objectShape(object));
    }
    if(const double inset=tintInset(n,segments);inset>0) {
        // Close the seams where a Link meets a Connector first (their shapes touch, not share an
        // edge), so only the outer road edge is inset, not each object's ends.
        constexpr double seam=0.1;
        QPainterPathStroker close;close.setWidth(2*seam);close.setJoinStyle(Qt::MiterJoin);
        roads=roads.united(close.createStroke(roads)).simplified();
        QPainterPathStroker edge;edge.setWidth(2*(inset+seam));edge.setJoinStyle(Qt::MiterJoin);
        roads=roads.subtracted(edge.createStroke(roads));
    }
    std::vector<std::vector<Point>> chains;
    for(auto geometry:routeGeometries(n,segments)) {
        if(position && !geometry.empty())for(const auto& link:n.links)if(link.id==segments.front())
            for(const auto& lane:link.lanes) {
                const auto origin=laneGeometry(link,lane.id,n.drivingSide);
                if(origin.empty() || std::hypot(geometry.front().x-origin.front().x,geometry.front().y-origin.front().y)>1e-7)continue;
                geometry=polylineSpan(geometry,matchedStation(link.geometry,origin,*position),polylineLength(geometry));break;
            }
        chains.push_back(std::move(geometry));
    }
    const auto endObject=destination?destination->objectId:segments.back();
    const auto endBar=objectCrossbar(n,endObject,true,destination?std::optional<double>(destination->station):std::nullopt);
    // D509: the tint stops at the destination bar; vehicles still drive on to the road's end.
    auto* overlay=scene_.addPath(tintPath(chains,roads,widestLane(n),destination?endBar:std::nullopt),QPen(Qt::NoPen),QBrush(canvasStyle::routeTint(preview)));
    overlay->setZValue(200004);overlay->setData(0,preview?QStringLiteral("route-draft"):QStringLiteral("route-overlay"));
    overlay->setData(1,QString::fromStdString(id));
    if(preview){previewTint_=overlay;previewChains_=std::move(chains);previewRoads_=roads;}
    const auto mark=[&](std::optional<RoadCrossbar> bar,bool end) {
        if(!bar || !levelVisible(bar->level))return;
        // D435: origin green, destination red; D508: the inputs' thin bar; D509: canvas tones that
        // read on every road, with no Base casing even while the bar moves (owner: no white frame).
        auto* line=new RoadCrossbarItem(*bar,end?canvasStyle::routeEnd():canvasStyle::routeStart(),viewScale());
        scene_.addItem(line);line->setZValue(200008.5); // D508: above a placed decision's bar of the same width
        if(preview && end)previewRouteEnd_=line;
        line->setData(0,end?QStringLiteral("route-end"):QStringLiteral("route-start"));line->setData(1,QString::fromStdString(id));
    };
    mark(originBar(n,segments.front(),position),false);
    // D508: a draft that is only its origin has no destination yet, so no red bar at its far end.
    if(!preview || destination || segments.size()>1)mark(endBar,true);
}
// The preview follows the pointer along one road without a scene rebuild: the destination bar
// moves and the tint is re-cut at it from the chains drawRouteOverlay kept.
void EditorCanvas::retintRoutePreview() {
    if(!document_ || !previewTint_)return;
    const auto& n=document_->network;
    const auto& segments=routePreview_.empty()?routeDraft_:routePreview_;
    if(segments.empty())return;
    const auto destination=previewDestination();
    const auto bar=objectCrossbar(n,destination?destination->objectId:segments.back(),true,
                                  destination?std::optional<double>(destination->station):std::nullopt);
    if(previewRouteEnd_ && bar)previewRouteEnd_->setLine(bar->first.x,bar->first.y,bar->second.x,bar->second.y);
    previewTint_->setPath(tintPath(previewChains_,previewRoads_,widestLane(n),destination?bar:std::nullopt));
}
// D509: before a route starts, a green bar shows where a click or Ctrl+right release would put
// the origin on the hovered road.
std::optional<RoadCrossbar> EditorCanvas::hoverOriginBar() const {
    if(!document_ || !routeHover_)return std::nullopt;
    return originBar(document_->network,routeHover_->objectId,hoverOriginStation());
}
void EditorCanvas::drawRouteOrigin() {
    if(tool_!=Tool::route || !routeDraft_.empty() || routePress_ || !routeHover_ || !hoverReachable_)return;
    const auto bar=hoverOriginBar();
    if(!bar || !levelVisible(bar->level))return;
    auto* line=new RoadCrossbarItem(*bar,canvasStyle::routeStart(),viewScale());
    scene_.addItem(line);line->setZValue(200008.5);
    line->setData(0,QStringLiteral("route-start"));
    previewRouteStart_=line;
}
}
