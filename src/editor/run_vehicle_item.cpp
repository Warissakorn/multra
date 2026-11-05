#include "run_vehicle_item.hpp"
#include "canvas_style.hpp"
#include <QPainter>
#include <QPaintDevice>
#include <algorithm>

namespace multra {
RunVehicleItem::RunVehicleItem(const QPainterPath& body,QColor fill,double pixelsPerMetre)
    : QGraphicsPathItem(body), margin_((shadowPixels+2)/std::max(1e-9,pixelsPerMetre)) {
    setPen(Qt::NoPen);setBrush(fill);
}
RunVehicleItem::RunVehicleItem(const VehicleDrawing& drawing,QColor fill,double pixelsPerMetre)
    : RunVehicleItem(drawing.body,fill,pixelsPerMetre) {setDrawing(drawing);}
void RunVehicleItem::setDrawing(const VehicleDrawing& drawing) {
    setPath(drawing.body);glass_=drawing.glass;panels_=drawing.panels;lamps_=drawing.lamps;update();
}
void RunVehicleItem::setPixelsPerMetre(double pixelsPerMetre) {
    const double margin=(shadowPixels+2)/std::max(1e-9,pixelsPerMetre); // casing and shadow
    if(margin!=margin_){prepareGeometryChange();margin_=margin;}
}
QColor RunVehicleItem::casingColor() const {
    return wireframe_?canvasStyle::connectorBoundaryColor(brush().color()):canvasStyle::kerb();
}
QRectF RunVehicleItem::boundingRect() const {
    return QGraphicsPathItem::boundingRect().adjusted(-margin_,-margin_,margin_,margin_);
}
void RunVehicleItem::paint(QPainter* painter,const QStyleOptionGraphicsItem*,QWidget*) {
    painter->setPen(Qt::NoPen);
    // D465 (slice C, Q10): a soft shadow -- the body again in translucent black, offset a fixed
    // shadowPixels down and right on screen whatever the view's rotation, flip or heading-up. No blur
    // (a blur effect re-renders every vehicle every frame), and none below the detail size, where
    // the body is its floored minimum symbol.
    // D467: deviceTransform() counts device pixels, which on a high-DPI screen are 1/ratio of a logical
    // pixel, so the offset and the threshold are scaled by the device's pixel ratio.
    const double ratio=painter->device()?std::max(1.0,painter->device()->devicePixelRatioF()):1.0;
    const QTransform device=painter->deviceTransform();
    const QRectF onScreen=device.mapRect(path().boundingRect());
    if(device.isInvertible() && std::max(onScreen.width(),onScreen.height())>=shadowMinPixels*ratio) {
        const QTransform back=device.inverted();
        const QPointF offset=back.map(QPointF(shadowPixels*ratio,shadowPixels*ratio))-back.map(QPointF(0,0));
        painter->save();painter->translate(offset); // the same path, moved: no copy per vehicle per paint
        painter->setBrush(shadowColor());painter->drawPath(path());painter->restore();
    }
    painter->setBrush(brush());painter->drawPath(path());
    // All details stay inside the body, and inside this one reusable scene item.
    // Clip also protects custom dimensions with very narrow aspect ratios.
    if(!glass_.isEmpty() || !panels_.isEmpty() || !lamps_.isEmpty()) {
        painter->save();painter->setClipPath(path(),Qt::IntersectClip);
        painter->setBrush(brush().color().lighter(115));painter->drawPath(panels_);
        painter->setBrush(QColor("#172c3b"));painter->drawPath(glass_);
        painter->setBrush(QColor("#f5f1d8"));painter->drawPath(lamps_);
        painter->restore();
    }
    QPen casing(casingColor(),1);casing.setCosmetic(true);casing.setJoinStyle(Qt::MiterJoin);
    painter->setPen(casing);painter->setBrush(Qt::NoBrush);painter->drawPath(path());
    // A followed vehicle's outline (D165) stays on top, as before.
    if(pen().style()!=Qt::NoPen){painter->setPen(pen());painter->drawPath(path());}
}
}
