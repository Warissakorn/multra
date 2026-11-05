#pragma once
#include <QGraphicsPathItem>
#include <QColor>
#include "vehicle_shape.hpp"

namespace multra {
// Body, interior decoration and a 1 px cosmetic casing, in one reusable scene item.
// Decoration and casing are paint-only: picking uses the solid body path and item pen.
class RunVehicleItem : public QGraphicsPathItem {
public:
    // D465 (slice C): the shadow's screen offset, the body's on-screen size below which it has none
    // (the D97 detail length), and its colour.
    static constexpr double shadowPixels = 2, shadowMinPixels = 14;
    static QColor shadowColor() { return QColor(0, 0, 0, 70); }
    RunVehicleItem(const QPainterPath& body, QColor fill, double pixelsPerMetre);
    RunVehicleItem(const VehicleDrawing&, QColor fill, double pixelsPerMetre);
    void setDrawing(const VehicleDrawing&);
    // D283: a frame reuses the item; the zoom sets the casing's reach.
    void setPixelsPerMetre(double pixelsPerMetre);
    QRectF boundingRect() const override;
    void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override;
private:
    QPainterPath glass_,panels_,lamps_;
    double margin_{}; // the casing's reach outside the path, in metres at the frame's zoom
};
}
