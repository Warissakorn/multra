#include "canvas.hpp"
#include "canvas_style.hpp"
#include "../model/network/connector_surface.hpp"
#include <QGraphicsPathItem>
#include <QPainterPath>
#include <cmath>
#include <limits>

// Creation previews (D433): while a Link, Connector or StreetSegment is being drawn, the canvas
// paints the object the release would create -- its asphalt, kerb width and lane markings --
// translucent under the thin draft line. Nothing here touches the document.
namespace multra {
namespace {
QPainterPath polyline(const std::vector<Point>& points) {
    QPainterPath p;
    for(std::size_t i=0;i<points.size();++i)i?p.lineTo(points[i].x,points[i].y):p.moveTo(points[i].x,points[i].y);
    return p;
}
constexpr double previewZ=200005;
}
void EditorCanvas::setNewLinkLanes(int count,double width) {
    newLaneCount_=std::max(1,count);newLaneWidth_=width>0?width:3.5;
}
bool EditorCanvas::drawPreviewLink(const Link& link) {
    const double length=polylineLength(link.geometry);
    if(link.lanes.empty() || !std::isfinite(length) || length<=0)return false;
    const auto& look=style(link.displayType);
    const auto side=document_->network.drivingSide;
    try {
        const auto left=trimSelfIntersections(laneBoundaryGeometry(link,0,side));
        const auto right=trimSelfIntersections(laneBoundaryGeometry(link,link.lanes.size(),side));
        auto surface=polyline(left);for(auto it=right.rbegin();it!=right.rend();++it)surface.lineTo(it->x,it->y);surface.closeSubpath();
        QColor asphalt(QString::fromStdString(look.linkColor));asphalt.setAlphaF(canvasStyle::creationPreviewOpacity);
        auto* road=scene_.addPath(surface,QPen(Qt::NoPen),QBrush(asphalt));road->setZValue(previewZ);
        road->setData(0,QStringLiteral("creation-preview"));
        double narrowest=std::numeric_limits<double>::infinity();
        for(const auto& lane:link.lanes)narrowest=std::min(narrowest,lane.width);
        if(!canvasStyle::laneMarkingsReadable(narrowest,viewScale()))return true;
        for(const auto& marking:markingStrokes(canvasStyle::paintedLinkMarkings(linkMarkings(link,side)))) {
            auto pen=canvasStyle::markingPen(QColor(QString::fromStdString(look.laneColor)),marking.type);
            auto* mark=scene_.addPath(canvasStyle::markingPath(marking.geometry,marking.type,look.laneDash,look.laneGap),pen);
            mark->setZValue(previewZ+0.1);mark->setOpacity(canvasStyle::creationPreviewOpacity);mark->setData(0,QStringLiteral("creation-preview"));
        }
    } catch(const std::exception&) { return false; /* Transient geometry with no surface keeps the draft line. */ }
    return true;
}
bool EditorCanvas::drawPreviewConnector(const LaneReference& from,const LaneReference& to) {
    try {
        Connector c{"creation-preview",from,to,connectorCurve(document_->network,from,to)};
        for(const auto& link:document_->network.links)if(link.id==from.linkId){c.level=link.level;break;}
        const auto drawing=connectorSurface(document_->network,c);
        if(drawing.outline.size()<3)return false;
        auto surface=polyline(drawing.outline);surface.closeSubpath();surface.setFillRule(Qt::WindingFill);
        const auto& look=style(c.displayType);
        QColor asphalt(QString::fromStdString(look.linkColor));asphalt.setAlphaF(canvasStyle::creationPreviewOpacity);
        auto* road=scene_.addPath(surface,QPen(Qt::NoPen),QBrush(asphalt));road->setZValue(previewZ);
        road->setData(0,QStringLiteral("creation-preview"));
        const QColor boundary=canvasStyle::connectorBoundaryColor(QColor(QString::fromStdString(look.connectorColor)));
        for(const auto& marking:markingStrokes(drawing.markings)) {
            auto* mark=scene_.addPath(canvasStyle::markingPath(marking.geometry,marking.type,look.laneDash,look.laneGap),
                canvasStyle::markingPen(marking.edge?boundary:QColor(QString::fromStdString(look.laneColor)),marking.type));
            mark->setZValue(previewZ+0.1);mark->setOpacity(canvasStyle::creationPreviewOpacity);mark->setData(0,QStringLiteral("creation-preview"));
        }
    } catch(const std::exception&) { return false; /* Coincident or impossible ends: the draft line. */ }
    return true;
}
// Draws the preview of the gesture under way; true when a road surface stands in for the draft line.
bool EditorCanvas::drawCreationPreview() {
    if(!document_ || wireframe_)return false;
    const int level=visibleLevel_.value_or(0);bool drawn=false;
    if(creating_ && draft_.size()>=2) {
        if(creationTool_==Tool::connect) {
            if(gestureFrom_ && gestureTarget_)drawn=drawPreviewConnector(*gestureFrom_,*gestureTarget_);
        } else {
            Link link{"creation-preview",draft_,{},level};
            for(int i=0;i<newLaneCount_;++i)link.lanes.push_back({"preview-"+std::to_string(i),newLaneWidth_});
            drawn=drawPreviewLink(link);
        }
    }
    if(tool_==Tool::connect && connectorFrom_ && connectorHover_)drawn=drawPreviewConnector(*connectorFrom_,*connectorHover_)||drawn;
    if(tool_==Tool::road && streetSegmentPreview && !zoneDraft_.empty()) {
        auto centreline=zoneDraft_;if(draftPointer_)centreline.push_back(*draftPointer_);
        if(centreline.size()>=2)
            for(auto link:streetSegmentPreview(centreline)){link.level=level;link.id="creation-preview";drawn=drawPreviewLink(link)||drawn;}
    }
    return drawn;
}
}
