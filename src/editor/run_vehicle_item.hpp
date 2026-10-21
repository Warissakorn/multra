#pragma once
#include <QGraphicsPathItem>

namespace multra {
// A run vehicle (D199): the body fill plus a 1 px cosmetic casing whose tone is opposite the fill,
// so the body or its edge stands out on any background, photo or grid. The casing is drawn in
// paint() only: shape(), which a click tests, stays the item's own path and pen.
class RunVehicleItem : public QGraphicsPathItem {
public:
    RunVehicleItem(const QPainterPath& body, QColor fill, double pixelsPerMetre);
    // D283: a frame reuses the item; the zoom sets the casing's reach.
    void setPixelsPerMetre(double pixelsPerMetre);
    QRectF boundingRect() const override;
    void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override;
private:
    double margin_{}; // the casing's reach outside the path, in metres at the frame's zoom
};
}
