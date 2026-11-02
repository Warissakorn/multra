#include "road_crossbar.hpp"
#include <QPainter>
#include "ui_design_tokens.hpp"
#include <QPainterPathStroker>
#include <algorithm>
#include <cmath>

namespace multra {
namespace {
// At a mouth the normal can meet one Connector rail and one attached Link rail.
// These are actual road edges, never extrapolated imaginary rail segments.
std::pair<std::vector<Point>,std::vector<Point>> attachmentRails(const Network& n,
    const LaneReference& first,const LaneReference& last) {
    for(const auto& l:n.links)if(l.id==first.linkId && l.id==last.linkId) {
        std::size_t a=l.lanes.size(),b=l.lanes.size();
        for(std::size_t k=0;k<l.lanes.size();++k) {
            if(l.lanes[k].id==first.laneId)a=k;
            if(l.lanes[k].id==last.laneId)b=k;
        }
        if(a<l.lanes.size() && b<l.lanes.size())return {
            laneBoundaryGeometry(l,std::min(a,b),n.drivingSide),laneBoundaryGeometry(l,std::max(a,b)+1,n.drivingSide)};
    }
    return {};
}
}
std::optional<RoadCrossbar> roadCrossbar(const std::vector<Point>& centre,
    const std::vector<Point>& firstRail, const std::vector<Point>& secondRail,
    double station, int level,const std::vector<Point>& firstContinuation,const std::vector<Point>& secondContinuation) {
    if(centre.size()<2 || firstRail.size()<2 || secondRail.size()<2 || !std::isfinite(station))return {};
    const double length=polylineLength(centre);
    if(!(length>0) || !std::isfinite(length))return {};
    station=std::clamp(station,0.,length);
    const auto origin=pointAlong(centre,station),u=directionAlong(centre,station,station==length);
    const auto meet=[&](const std::vector<Point>& rail,const std::vector<Point>& continuation)->std::optional<Point> {
        const auto guess=pointAlong(rail,matchedStation(centre,rail,station));
        std::optional<Point> result;double nearest=INFINITY;
        const auto scan=[&](const std::vector<Point>& edge) {
        for(std::size_t i=1;i<edge.size();++i) {
            const Point d{edge[i].x-edge[i-1].x,edge[i].y-edge[i-1].y};
            const double denominator=d.x*u.x+d.y*u.y;
            if(std::abs(denominator)<1e-12)continue;
            const double t=((origin.x-edge[i-1].x)*u.x+(origin.y-edge[i-1].y)*u.y)/denominator;
            if(t < -1e-8 || t > 1+1e-8)continue;
            const Point hit{edge[i-1].x+std::clamp(t,0.,1.)*d.x,edge[i-1].y+std::clamp(t,0.,1.)*d.y};
            const double distance=std::hypot(hit.x-guess.x,hit.y-guess.y);
            if(distance<nearest){nearest=distance;result=hit;}
        }
        };
        scan(rail);scan(continuation);
        return result;
    };
    const auto a=meet(firstRail,firstContinuation),b=meet(secondRail,secondContinuation);
    if(!a || !b || std::hypot(a->x-b->x,a->y-b->y)<1e-9)return {};
    return RoadCrossbar{*a,*b,level,u};
}
std::optional<RoadCrossbar> objectCrossbar(const Network& n,const std::string& id,bool end,std::optional<double> station) {
    try {
        for(const auto& l:n.links)if(l.id==id) {
            const auto axis=linkCentreline(l,n.drivingSide);
            return roadCrossbar(axis,laneBoundaryGeometry(l,0,n.drivingSide),
                laneBoundaryGeometry(l,l.lanes.size(),n.drivingSide),station?matchedStation(l.geometry,axis,*station):end?polylineLength(axis):0,l.level);
        }
        for(const auto& c:n.connectors)if(c.id==id) {
            const auto rails=connectorBoundaries(n,c);
            if(rails.size()<2)return {};
            const auto paths=connectorPaths(n,c);
            if(paths.empty() || rails.front().size()!=rails.back().size())return {};
            // Rail midpoints agree with edited Connector paths, including its mouths.
            std::vector<Point> axis;
            for(std::size_t i=0;i<rails.front().size();++i)
                axis.push_back({(rails.front()[i].x+rails.back()[i].x)/2,(rails.front()[i].y+rails.back()[i].y)/2});
            const auto extra=attachmentRails(n,end?paths.front().to:paths.front().from,end?paths.back().to:paths.back().from);
            return roadCrossbar(axis,rails.front(),rails.back(),station?matchedStation(objectGeometry(n,id),axis,*station):end?polylineLength(axis):0,c.level,extra.first,extra.second);
        }
    } catch(const std::exception&) {}
    return {};
}
std::optional<RoadCrossbar> signalCrossbar(const Network& n,const NetworkSignalHead& h) {
    try {
        if(h.connectorId.empty())for(const auto& l:n.links)if(l.id==h.lane.linkId)
            for(std::size_t k=0;k<l.lanes.size();++k)if(l.lanes[k].id==h.lane.laneId)
                return roadCrossbar(laneGeometry(l,h.lane.laneId,n.drivingSide),
                    laneBoundaryGeometry(l,k,n.drivingSide),laneBoundaryGeometry(l,k+1,n.drivingSide),h.position,l.level);
        for(const auto& c:n.connectors) {
            const auto paths=connectorPaths(n,c);
            for(std::size_t k=0;k<paths.size();++k)if(paths[k].id==h.connectorId) {
                const auto rails=connectorBoundaries(n,c);
                if(k+1>=rails.size())return {};
                const double length=connectorPathLength(paths[k]);
                const auto origin=connectorPathPoint(paths[k],h.position);
                const bool source=laneContains(n,paths[k].from,origin);
                const bool target=laneContains(n,paths[k].to,origin);
                const auto ref=source && (!target || h.position<length/2)?paths[k].from:paths[k].to;
                const auto extra=source || target?attachmentRails(n,ref,ref):std::pair<std::vector<Point>,std::vector<Point>>{};
                return roadCrossbar(paths[k].geometry,rails[k],rails[k+1],h.position,c.level,extra.first,extra.second);
            }
        }
    } catch(const std::exception&) {}
    return {};
}
RoadCrossbarItem::RoadCrossbarItem(const RoadCrossbar& bar,QColor colour,double scale,bool preview,SignalCue cue)
    : QGraphicsLineItem(bar.first.x,bar.first.y,bar.second.x,bar.second.y),
      hitWidth_(editorDesign::crossbarHitPixels/std::max(1e-9,std::abs(scale))), cue_(cue), preview_(preview) {
    QPen pen(colour,editorDesign::crossbarPixels,Qt::SolidLine);
    pen.setCosmetic(true);pen.setCapStyle(Qt::FlatCap);setPen(pen);
}
QPainterPath RoadCrossbarItem::shape() const {
    QPainterPath linePath;linePath.moveTo(line().p1());linePath.lineTo(line().p2());
    // Widen along the road, never past the lane edges into a neighbouring head.
    QPainterPathStroker stroke;stroke.setWidth(hitWidth_);stroke.setCapStyle(Qt::FlatCap);
    return stroke.createStroke(linePath);
}
QRectF RoadCrossbarItem::boundingRect() const { return shape().boundingRect(); }
void RoadCrossbarItem::paint(QPainter* painter,const QStyleOptionGraphicsItem* option,QWidget* widget) {
    if(preview_) {
        QColor casing=editorDesign::role(QPalette::Base);casing.setAlpha(170);
        QPen under(casing,pen().widthF()+3);under.setCosmetic(true);under.setCapStyle(Qt::FlatCap);
        painter->setPen(under);painter->drawLine(line());
    }
    if(cue_==SignalCue::none||cue_==SignalCue::stop){QGraphicsLineItem::paint(painter,option,widget);return;}
    const auto a=line().p1(),b=line().p2(),third=(b-a)/3;
    painter->setPen(pen());
    painter->drawLine(a,a+third);painter->drawLine(b-third,b);
    if(cue_==SignalCue::caution) {
        auto thin=pen();thin.setWidthF(std::max(1.0,pen().widthF()/2));
        painter->setPen(thin);painter->drawLine(a+third,b-third);
    }
}
SignalHeadItem::SignalHeadItem(const RoadCrossbar& bar,QColor colour,double scale,bool preview,SignalCue cue)
    : RoadCrossbarItem(bar,colour,scale,preview,SignalCue::none),travel_(bar.travel),scale_(std::max(1e-9,std::abs(scale))),lamp_(cue) {
    auto thin=pen();thin.setWidthF(2);setPen(thin);
}
std::optional<SignalHeadItem::HeadFrame> SignalHeadItem::frame() const {
    // A rounded body 3 lamps long, across the lane, one gap upstream of the stop line. Sizes are
    // screen pixels, capped so the head never overhangs its lane.
    const auto a=line().p1(),b=line().p2();const double lane=std::hypot(b.x()-a.x(),b.y()-a.y());
    const double t=std::hypot(travel_.x,travel_.y);
    if(!(lane>0) || !(t>0))return {};
    const double length=std::min(30/scale_,0.9*lane),depth=length/3,gap=3/scale_;
    const QPointF across((b.x()-a.x())/lane,(b.y()-a.y())/lane),back(-travel_.x/t,-travel_.y/t);
    return HeadFrame{(a+b)/2+back*(gap+depth/2),across,back,length,depth};
}
QPainterPath SignalHeadItem::housing() const {
    const auto f=frame();if(!f)return {};
    QPainterPath local;local.addRoundedRect(QRectF(-f->length/2,-f->depth/2,f->length,f->depth),f->depth/3,f->depth/3);
    return QTransform(f->across.x(),f->across.y(),f->back.x(),f->back.y(),f->centre.x(),f->centre.y()).map(local);
}
QPainterPath SignalHeadItem::shape() const { return RoadCrossbarItem::shape().united(housing()); }
QRectF SignalHeadItem::boundingRect() const { return shape().boundingRect().adjusted(-4/scale_,-4/scale_,4/scale_,4/scale_); }
void SignalHeadItem::paint(QPainter* painter,const QStyleOptionGraphicsItem* option,QWidget* widget) {
    RoadCrossbarItem::paint(painter,option,widget);
    const auto f=frame();if(!f)return;
    painter->setRenderHint(QPainter::Antialiasing);
    QPen edge(pen().color(),1.5);edge.setCosmetic(true);
    painter->setPen(edge);painter->setBrush(QColor(38,42,48));painter->drawPath(housing());
    // Lamps along the body's long axis, red / amber / green from the first end of the bar.
    const QColor hues[]={QColor(230,57,53),QColor(245,180,30),QColor(46,184,92)};
    const SignalCue cues[]={SignalCue::stop,SignalCue::caution,SignalCue::go};
    for(int k=0;k<3;++k) {
        QColor lampColour=hues[k];
        if(lamp_==SignalCue::none)lampColour.setAlpha(150); // editing: the head, not a state
        else if(lamp_!=cues[k])lampColour=QColor(70,74,80);
        painter->setPen(Qt::NoPen);painter->setBrush(lampColour);
        painter->drawEllipse(f->centre+f->across*((k-1)*f->depth),f->depth*0.36,f->depth*0.36);
    }
}
}
