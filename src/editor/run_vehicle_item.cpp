#include "run_vehicle_item.hpp"
#include "canvas_style.hpp"
#include <QPainter>
#include <algorithm>

namespace multra {
RunVehicleItem::RunVehicleItem(const QPainterPath& body,QColor fill,double pixelsPerMetre)
    : QGraphicsPathItem(body), margin_(2/std::max(1e-9,pixelsPerMetre)) {
    setPen(Qt::NoPen);setBrush(fill);
}
RunVehicleItem::RunVehicleItem(const VehicleDrawing& drawing,QColor fill,double pixelsPerMetre)
    : RunVehicleItem(drawing.body,fill,pixelsPerMetre) {setDrawing(drawing);}
void RunVehicleItem::setDrawing(const VehicleDrawing& drawing) {
    setPath(drawing.body);glass_=drawing.glass;panels_=drawing.panels;lamps_=drawing.lamps;update();
}
void RunVehicleItem::setPixelsPerMetre(double pixelsPerMetre) {
    const double margin=2/std::max(1e-9,pixelsPerMetre);
    if(margin!=margin_){prepareGeometryChange();margin_=margin;}
}
QRectF RunVehicleItem::boundingRect() const {
    return QGraphicsPathItem::boundingRect().adjusted(-margin_,-margin_,margin_,margin_);
}
void RunVehicleItem::paint(QPainter* painter,const QStyleOptionGraphicsItem*,QWidget*) {
    painter->setPen(Qt::NoPen);painter->setBrush(brush());painter->drawPath(path());
    // All details stay inside the body, and inside this one reusable scene item.
    // Clip also protects custom dimensions with very narrow aspect ratios.
    if(!glass_.isEmpty() || !panels_.isEmpty() || !lamps_.isEmpty()) {
        painter->save();painter->setClipPath(path(),Qt::IntersectClip);
        painter->setBrush(brush().color().lighter(115));painter->drawPath(panels_);
        painter->setBrush(QColor("#172c3b"));painter->drawPath(glass_);
        painter->setBrush(QColor("#f5f1d8"));painter->drawPath(lamps_);
        painter->restore();
    }
    QPen casing(canvasStyle::connectorBoundaryColor(brush().color()),1);casing.setCosmetic(true);casing.setJoinStyle(Qt::MiterJoin);
    painter->setPen(casing);painter->setBrush(Qt::NoBrush);painter->drawPath(path());
    // A followed vehicle's outline (D165) stays on top, as before.
    if(pen().style()!=Qt::NoPen){painter->setPen(pen());painter->drawPath(path());}
}
}
