#pragma once
#include <QGraphicsSimpleTextItem>
#include <QPointF>

namespace multra {
// D424 (canvas style slice B): a canvas label is upright screen text with a halo in the Base
// colour, so it reads over asphalt, a map or the grid. The canvas places it beside its anchor in
// screen space (EditorCanvas::placeLabels); a label that finds no free side is hidden unless pinned.
class CanvasLabel : public QGraphicsSimpleTextItem {
public:
    static constexpr double halo = 3; // logical px of Base around each glyph
    enum Priority { zone = 0, node = 1, decision = 2, input = 3 };
    CanvasLabel(const QString& text, const QFont& font, QPointF anchor, int priority, bool pinned);
    QPointF anchor() const { return anchor_; }
    int priority() const { return priority_; }
    bool pinned() const { return pinned_; }
    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override;
private:
    QPointF anchor_;
    int priority_{};
    bool pinned_{};
};
}
