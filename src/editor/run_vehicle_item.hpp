#pragma once
#include <QGraphicsPathItem>
#include "vehicle_shape.hpp"

namespace multra {
// Body, interior decoration and a 1 px cosmetic casing, in one reusable scene item.
// Decoration and casing are paint-only: picking uses the solid body path and item pen.
class RunVehicleItem : public QGraphicsPathItem {
public:
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
