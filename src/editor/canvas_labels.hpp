#pragma once
#include <QGraphicsSimpleTextItem>
namespace trafficsim {
// Logical pixels, vector text, with no device/bitmap cache. Upright at every camera scale.
class CanvasLabelItem final : public QGraphicsSimpleTextItem {
public:
    explicit CanvasLabelItem(QString text,QFont font);
    QRectF boundingRect() const override;
    void paint(QPainter*,const QStyleOptionGraphicsItem*,QWidget*) override;
};
}
