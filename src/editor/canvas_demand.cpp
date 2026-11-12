#include "canvas.hpp"
#include "road_crossbar.hpp"
#include "canvas_style.hpp"
#include "canvas_labels.hpp"
#include <QGuiApplication>
#include <QGraphicsPathItem>
#include <QGraphicsSimpleTextItem>
#include <QPainterPathStroker>
#include "../project/demand_paths.hpp"
#include "../project/route_destination.hpp"
#include <QMouseEvent>
#include <algorithm>
#include <cmath>

namespace multra {
// What a click means to the route and input tools: the Link or Connector under the pointer,
// which is what a route names. hitObjects already answers that question for every other tool,
// so the pick, the selection outline and the route agree about what was clicked.
std::string EditorCanvas::objectAt(Point p) const {
    if(!document_)return {};
    const auto hits=hitObjects(p);
    for(const auto& hit:hits) {
        if(isLink(hit.first))return hit.first;
        for(const auto& connector:document_->network.connectors)if(connector.id==hit.first)return hit.first;
    }
    return {};
}
std::vector<std::string> EditorCanvas::routeDraftWith(const std::string& target) const {
    if(!document_ || target.empty())return {};
    if(routeDraft_.empty())return {target};
    return routeChainTo(document_->network,routeDraft_,target);
}
void EditorCanvas::startRouteDraft(const std::string& objectId) {
    if(!document_ || objectId.empty())return;
    if(routeLaneChains(document_->network,{objectId}).empty())return;
    routeStartPosition_.reset();routeEnd_.reset();routeDraft_={objectId};routePreview_=routeDraft_;routeTracing_=true;hoverSegment_.clear();
    if(routeDraftChanged)routeDraftChanged();
    redraw();
}
std::pair<std::string,std::string> EditorCanvas::demandObjectAt(QPoint viewportPosition) const {
    std::pair<std::string,std::string> route;
    for(const auto* item:items(viewportPosition)) {
        const auto kind=item->data(0).toString();
        const auto id=item->data(1).toString().toStdString();
        if(id.empty())continue; // An unfinished Route is not an editable stored object.
        if(kind=="input-marker" || kind=="input-volume")return {"input",item->data(1).toString().toStdString()};
        if(kind=="decision-marker")return {"decision",item->data(1).toString().toStdString()};
        if(kind=="route-overlay" || kind=="route-start" || kind=="route-end")route={"route",id};
    }
    return route;
}
bool EditorCanvas::demandPress(QMouseEvent* e) {
    if(tool_!=Tool::route && tool_!=Tool::input)return false;
    const bool trace=tool_==Tool::route && e->button()==Qt::LeftButton && e->modifiers()==Qt::NoModifier;
    const bool legacy=e->button()==Qt::RightButton && (e->modifiers()&Qt::ControlModifier);
    if(!trace && !legacy)return false;
    const auto target=objectAt(world(e->pos(),false));
    if(target.empty()){clearSelection(false);return true;}
    if(tool_==Tool::input) {
        if(isLink(target)) {if(inputPlaced)inputPlaced(target);}else reject();
        return true;
    }
    if(trace) {
        if(routeDraft_.empty()){
            clearSelection(false);startRouteDraft(target);
            for(const auto& link:document_->network.links)if(link.id==target) {
                routeStartPosition_=stationOfClosestPoint(link.geometry,world(e->pos(),false));
                if(document_->definition)for(const auto& decision:document_->definition->routingDecisions)
                    if(decision.linkId==target && decision.position &&
                       std::abs(*decision.position-*routeStartPosition_)*viewScale()<=10)routeStartPosition_=decision.position;
            }
            redraw();return true;
        }
        // Re-read the click: Qt can coalesce the final hover move.
        const auto base=routePreview_.empty()?routeDraft_:routePreview_;
        auto candidate=base;
        const auto previous=std::find(candidate.begin(),candidate.end(),target);
        if(previous!=candidate.end())candidate.erase(previous+1,candidate.end());
        else {
            const auto chain=routeChainTo(document_->network,base,target);
            if(chain.empty()){reject();return true;}
            candidate.insert(candidate.end(),chain.begin(),chain.end());
        }
        if(routeLaneChains(document_->network,candidate).empty()){reject();return true;}
        updateRouteEnd(target,world(e->pos(),false));
        if(routeStartPosition_ && routeEnd_ && target==routeDraft_.front() && routeEnd_->station<*routeStartPosition_) {
            reject();return true;
        }
        // D507: a click back on the kept origin's line is not a zero-length destination.
        if(candidate.size()==1 && routeStartPosition_ && routeEnd_ &&
           std::abs(routeEnd_->station-*routeStartPosition_)*viewScale()<=10)return true;
        routeDraft_=std::move(candidate);routePreview_.clear();commitRouteDraft();return true;
    }
    // D508: Ctrl+right press shows the bar where the release will place it; moving while held
    // slides it, and the release is authoritative.
    routePress_=RoutePress{routeDraft_,routeStartPosition_};
    if(!applyRoutePress(e->pos())){routePress_.reset();reject();return true;}
    redraw();return true;
}
bool EditorCanvas::applyRoutePress(QPoint position) {
    if(!document_ || !routePress_)return false;
    const auto& n=document_->network;
    const auto pointer=world(position,false);
    const auto target=objectAt(pointer);
    if(target.empty())return false;
    auto draft=routePress_->draft;
    auto start=routePress_->start;
    std::optional<RouteDestination> end;
    if(draft.empty()) {
        if(routeLaneChains(n,{target}).empty())return false;
        draft={target};start.reset();
        for(const auto& link:n.links)if(link.id==target) {
            double station=stationOfClosestPoint(link.geometry,pointer);
            if(document_->definition)for(const auto& decision:document_->definition->routingDecisions)
                if(decision.linkId==target && decision.position && std::abs(*decision.position-station)*viewScale()<=10)station=*decision.position;
            if(station*viewScale()>10)start=station; // near the Link's start: the whole Link, as before
        }
    } else {
        const auto chain=routeChainTo(n,draft,target); // from the snapshot: each move starts again
        if(chain.empty())return false;
        draft.insert(draft.end(),chain.begin(),chain.end());
        if(routeLaneChains(n,draft).empty())return false;
        const auto geometry=routeDestinationGeometry(n,target);
        if(geometry.size()>=2) {
            const double station=stationOfClosestPoint(geometry,pointer);
            if((polylineLength(geometry)-station)*viewScale()>10)end=RouteDestination{target,station}; // else its far end
        }
    }
    routeDraft_=draft;routePreview_=std::move(draft);routeStartPosition_=start;routeEnd_=end;routeTracing_=false;
    return true;
}
bool EditorCanvas::finishRoutePress(QPoint position) {
    if(!routePress_)return false;
    applyRoutePress(position); // native platforms may coalesce the last move
    routePress_.reset();hoverSegment_.clear();
    if(routeDraftChanged)routeDraftChanged();
    redraw();return true;
}
bool EditorCanvas::isLink(const std::string& objectId) const {
    if(!document_ || objectId.empty())return false;
    for(const auto& link:document_->network.links)if(link.id==objectId)return true;
    return false;
}
bool EditorCanvas::demandHover(QMouseEvent* e) {
    if(tool_!=Tool::route && tool_!=Tool::input)return false;
    if(routePress_){if(applyRoutePress(e->pos()))redraw();return true;} // D508: the held bar follows
    const auto pointer=world(e->pos(),false);
    const auto hovered=objectAt(pointer);
    // D509: where on the hovered road the pointer is, for the bars that follow it.
    const auto place=[&]{
        routeHover_.reset();
        if(tool_!=Tool::route || hovered.empty() || !hoverReachable_)return;
        const auto geometry=routeDestinationGeometry(document_->network,hovered);
        if(geometry.size()>=2)routeHover_=RouteDestination{hovered,stationOfClosestPoint(geometry,pointer)};
    };
    // Path finding and scene rebuilds happen on object changes, not on every mouse pixel.
    if(hovered==hoverSegment_) {
        place();
        if(routeTracing_ && !routeDraft_.empty() && hoverReachable_)updateRouteEnd(hovered,pointer);
        else if(!routeDraft_.empty())retintRoutePreview();
        else if(previewRouteStart_)if(const auto bar=hoverOriginBar())previewRouteStart_->setLine(bar->first.x,bar->first.y,bar->second.x,bar->second.y);
        return true;
    }
    hoverSegment_=hovered;
    if(tool_==Tool::input)hoverReachable_=isLink(hovered);
    else if(routeDraft_.empty())hoverReachable_=!routeDraftWith(hovered).empty();
    else {
        const auto base=routeTracing_ && !routePreview_.empty()?routePreview_:routeDraft_;
        auto candidate=base;
        const auto previous=std::find(candidate.begin(),candidate.end(),hovered);
        if(previous!=candidate.end())candidate.erase(previous+1,candidate.end());
        else {
            const auto chain=routeChainTo(document_->network,base,hovered);
            if(!chain.empty())candidate.insert(candidate.end(),chain.begin(),chain.end());
            else candidate.clear();
        }
        hoverReachable_=!candidate.empty() && !routeLaneChains(document_->network,candidate).empty();
        if(hoverReachable_)routePreview_=std::move(candidate);
    }
    place();
    if(routeTracing_ && !routeDraft_.empty() && hoverReachable_)updateRouteEnd(hovered,world(e->pos(),false));
    else if(routeTracing_)routeEnd_.reset(); // a Ctrl+right draft keeps the end its release placed (D508)
    redraw();return true;
}
void EditorCanvas::updateRouteEnd(const std::string& target,Point pointer) {
    if(!document_)return;
    const auto geometry=routeDestinationGeometry(document_->network,target);
    if(geometry.size()<2)return;
    routeEnd_=RouteDestination{target,stationOfClosestPoint(geometry,pointer)};
    retintRoutePreview(); // D509: the bar and the tint's end move together
}
void EditorCanvas::commitRouteDraft() {
    if(routeDraft_.empty())return;
    const auto segments=routeTracing_ && !routePreview_.empty()?routePreview_:routeDraft_;
    const auto position=routeStartPosition_;
    const auto end=routeEnd_;
    const bool tracing=routeTracing_;
    clearRouteDraft();
    if(routeDestinationCommitted)routeDestinationCommitted(segments,position,end);
    else if(positionedRouteCommitted)positionedRouteCommitted(segments,position);
    else if(routeDraftCommitted)routeDraftCommitted(segments);
    // D507: a traced route keeps its origin, so the next click is another destination from the
    // same start (and the same routing decision). Restored after the commit, which replaced the
    // document and reset every gesture. Esc, or Backspace on the bare origin, ends it.
    if(tracing && document_ && !routeLaneChains(document_->network,{segments.front()}).empty()) {
        routeDraft_={segments.front()};routeStartPosition_=position;routeTracing_=true;
        if(routeDraftChanged)routeDraftChanged();
    }
    redraw();
}
void EditorCanvas::dropLastRouteSegment() {
    auto& path=routeTracing_?routePreview_:routeDraft_;
    if(path.empty())return;
    path.pop_back();
    if(routeTracing_ && path.empty()){routeDraft_.clear();routeStartPosition_.reset();}
    else if(!routeTracing_)routePreview_=routeDraft_;
    routeEnd_.reset();hoverSegment_.clear();
    if(routeDraftChanged)routeDraftChanged();
    redraw();
}
void EditorCanvas::clearRouteDraft() {
    routeDraft_.clear();routePreview_.clear();routeStartPosition_.reset();routeEnd_.reset();routeTracing_=false;hoverSegment_.clear();routePress_.reset();
    if(routeDraftChanged)routeDraftChanged();
}
void EditorCanvas::reject() {
    if(creationRejected)creationRejected();
    redraw();
}
void EditorCanvas::setHighlightedRoute(std::string id) {
    if(highlightedRoute_==id)return;
    highlightedRoute_=std::move(id);redraw();
}
void EditorCanvas::drawDemandOverlay() {
    if(!document_)return;
    const double scale=viewScale();
    // Hover halo: nearestLane used to pick in silence, so the author learned where a click
    // landed only after the dialog opened.
    if((tool_==Tool::route || tool_==Tool::input) && !hoverSegment_.empty()) {
        const auto geometry=objectGeometry(document_->network,hoverSegment_);
        if(geometry.size()>1) {
            const QColor colour=hoverReachable_?canvasStyle::hover():canvasStyle::error();
            QPen outline(colour,1.5);outline.setCosmetic(true);
            QColor fill=colour;fill.setAlpha(24);
            auto* item=scene_.addPath(objectShape(hoverSegment_),outline,QBrush(fill));
            item->setZValue(200003);item->setData(0,QStringLiteral("demand-hover"));
            item->setData(1,QString::fromStdString(hoverSegment_));
            item->setData(2,hoverReachable_);
        }
    }
    // Inputs use the same crossbar as route endpoints and signal heads.
    if(document_->definition)for(const auto& input:document_->definition->inputs) {
        const auto& def=*document_->definition;
        std::string origin=input.linkId;
        std::vector<std::string> segments;
        for(const auto& route:def.routes)if(route.id==input.routeId)segments=route.segmentIds;
        if(origin.empty() && !segments.empty())origin=segments.front();
        if(origin.empty())for(const auto& decision:def.routingDecisions)if(decision.id==input.routingDecisionId) {
            origin=decision.linkId;
            if(origin.empty())for(const auto& choice:decision.routes)
                for(const auto& route:def.routes)if(route.id==choice.routeId && !route.segmentIds.empty())origin=route.segmentIds.front();
        }
        const auto whole=objectCrossbar(document_->network,origin,false);
        if(!whole || !levelVisible(whole->level))continue;
        std::vector<RoadCrossbar> bars;
        std::vector<std::pair<int,int>> groups; // first lane and count of each Link bar
        for(const auto& link:document_->network.links)if(link.id==origin) {
            std::vector<bool> served(link.lanes.size(),segments.empty());
            if(!segments.empty())for(const auto& chain:routeLaneFamily(document_->network,segments))
                if(chain.lane<served.size())served[chain.lane]=true;
            const auto policy=inputLanePolicy(document_->network,def,input);
            if(policy.acceptsShares && input.laneShares.size()==served.size())
                for(std::size_t k=0;k<served.size();++k)served[k]=served[k] && input.laneShares[k]>0;
            for(std::size_t k=0;k<served.size();) {
                if(!served[k]){++k;continue;}
                const auto first=k;while(k<served.size() && served[k])++k;
                const auto a=laneBoundaryGeometry(link,first,document_->network.drivingSide);
                const auto b=laneBoundaryGeometry(link,k,document_->network.drivingSide);
                const auto centre=linkCentreline(link,document_->network.drivingSide);
                if(const auto bar=roadCrossbar(centre,a,b,0,link.level)) {
                    bars.push_back(*bar);groups.push_back({static_cast<int>(first),static_cast<int>(k-first)});
                }
            }
        }
        if(!isLink(origin))bars.push_back(*whole);
        if(bars.empty())continue;
        for(const auto& bar:bars) {
            auto* marker=new RoadCrossbarItem(bar,canvasStyle::active(),scale);
            scene_.addItem(marker);marker->setZValue(200005);
            marker->setData(0,QStringLiteral("input-marker"));marker->setData(1,QString::fromStdString(input.id));
        }
        const auto& at=bars.back().second;
        const auto value=input.intervals.empty()?input.vehiclesPerHour:0;
        const auto text=input.intervals.empty()?editorDesign::formatValue(value,0,inputRateLabel_)
            :inputPeriodsLabel_.arg(static_cast<int>(input.intervals.size()));
        // D424: placed beside the input's bar with every other label. An input has no selection of
        // its own on the canvas, so its label always shows while its origin road is selected or hovered.
        const bool lit=isSelected(origin) || origin==hoverObject_;
        // D465 (slice C): in a run a rate would lie over the vehicles entering, so only the selected or
        // hovered road's rate shows, as its box.
        const bool running=static_cast<bool>(runFrame_.scenario);
        if(running && !lit)continue;
        auto* label=addLabel(text,at,editorDesign::numericFont(),editorDesign::role(QPalette::Text),200006,
                 QStringLiteral("input-volume"),input.id,CanvasLabel::input,
                 lit || property("editorImageRendering").toBool(), // D464: a PNG shows every rate
                 CanvasLabel::Style::box); // D425
        label->setAccent(lit);
        // D460: on a Link the rate lies on one of its first served group's lanes, along the road's straight start.
        if(!groups.empty() && !running)for(const auto& link:document_->network.links)if(link.id==origin) {
            const auto& bar=bars.front();const auto [firstLane,laneCount]=groups.front();
            const QPointF from(bar.first.x,bar.first.y),to(bar.second.x,bar.second.y),direction(bar.travel.x,bar.travel.y);
            const QPointF start=(from+to)/2;
            const auto centre=linkCentreline(link,document_->network.drivingSide);
            const double offset=std::hypot(start.x()-centre.front().x,start.y()-centre.front().y);
            double reach=polylineLength(centre);
            for(const auto& point:centre) { // where the road bends off the start ray, the text stops
                const QPointF v(point.x-start.x(),point.y-start.y());
                const double ahead=v.x()*direction.x()+v.y()*direction.y(),aside=std::abs(v.x()*direction.y()-v.y()*direction.x());
                if(ahead>0 && std::abs(aside-offset)>.25*QLineF(from,to).length()/laneCount){reach=std::min(reach,ahead);break;}
            }
            label->setLaneSlot({from,to,direction,reach,firstLane,laneCount,QString::fromStdString(link.id),
                                QColor(QString::fromStdString(style(link.displayType).linkColor))});
        }
    }
    drawPositionedDecisions();
    // M2.1.1: a routing decision placed on a Link draws as a diamond a little way along it, where
    // Vissim draws its decision marker; clicking it opens the decision.
    if(document_->definition)for(const auto& decision:document_->definition->routingDecisions) {
        if(decision.linkId.empty() || decision.position)continue;
        const auto geometry=objectGeometry(document_->network,decision.linkId);
        if(geometry.size()<2)continue;
        const double r=6/scale;
        const auto at=pointAlong(geometry,std::min(12.0,polylineLength(geometry)/2));
        QPolygonF diamond;diamond<<QPointF(at.x+r,at.y)<<QPointF(at.x,at.y+r)<<QPointF(at.x-r,at.y)<<QPointF(at.x,at.y-r);
        QPen pen(canvasStyle::active(),1);pen.setCosmetic(true);
        auto* marker=scene_.addPolygon(diamond,pen,QBrush(editorDesign::role(QPalette::Midlight)));
        marker->setZValue(200005);marker->setData(0,QStringLiteral("decision-marker"));
        marker->setData(1,QString::fromStdString(decision.id));
        addLabel(QString::fromStdString(decision.name.empty()?decision.id:decision.name),at,QGuiApplication::font(),
                 editorDesign::role(QPalette::Text),200006,QStringLiteral("decision-label"),{},CanvasLabel::decision,
                 isSelected(decision.linkId) || decision.linkId==hoverObject_,CanvasLabel::Style::box);
    }
    if(document_->definition && !highlightedRoute_.empty())
        for(const auto& route:document_->definition->routes)if(route.id==highlightedRoute_)
            drawRouteOverlay(route.segmentIds,route.id,false);
    if(!routeDraft_.empty())drawRouteOverlay(routePreview_.empty()?routeDraft_:routePreview_,{},true);
    else drawRouteOrigin();
}
}
