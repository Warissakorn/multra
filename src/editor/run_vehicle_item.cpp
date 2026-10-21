#include "run_vehicle_item.hpp"
#include "canvas_style.hpp"
#include <QPainter>
#include <algorithm>

namespace multra {
RunVehicleItem::RunVehicleItem(const QPainterPath& body,QColor fill,double pixelsPerMetre)
    : QGraphicsPathItem(body), margin_(2/std::max(1e-9,pixelsPerMetre)) {
    setPen(Qt::NoPen);setBrush(fill);
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
    QPen casing(canvasStyle::connectorBoundaryColor(brush().color()),1);casing.setCosmetic(true);casing.setJoinStyle(Qt::MiterJoin);
    painter->setPen(casing);painter->setBrush(Qt::NoBrush);painter->drawPath(path());
    // A followed vehicle's outline (D165) stays on top, as before.
    if(pen().style()!=Qt::NoPen){painter->setPen(pen());painter->drawPath(path());}
}
}
