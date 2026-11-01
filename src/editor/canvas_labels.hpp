#pragma once
#include <QGraphicsSimpleTextItem>
#include <QPointF>

class QGraphicsLineItem;
namespace multra {
// D424/D425 (canvas style slices B and B2): a canvas label is upright, fixed-size screen text.
// A halo label is text stroked in Base (Node and zone names); a box label is text in a white box
// with a grey frame and a leader line to its anchor (input rates and decision names, in the
// spirit of PTV Vistro's fixed-size callouts). The canvas places it in screen space
// (EditorCanvas::placeLabels); a label that finds no free place is hidden unless pinned.
class CanvasLabel : public QGraphicsSimpleTextItem {
public:
    static constexpr double halo = 3;              // logical px of Base around each glyph
    static constexpr double padX = 4, padY = 2;    // box padding
    static constexpr double radius = 3;            // box corner
    enum Priority { zone = 0, node = 1, decision = 2, input = 3 };
    enum class Style { halo, box };
    CanvasLabel(const QString& text, const QFont& font, QPointF anchor, int priority, bool pinned, Style style = Style::halo);
    QPointF anchor() const { return anchor_; }
    int priority() const { return priority_; }
    bool pinned() const { return pinned_; }
    Style style() const { return style_; }
    QGraphicsLineItem* leader() const { return leader_; }
    void setLeader(QGraphicsLineItem* line) { leader_ = line; }
    QRectF textRect() const { return QGraphicsSimpleTextItem::boundingRect(); }
    QRectF frame() const; // the box in item pixels (the text rect for a halo label)
    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override;
private:
    QPointF anchor_;
    int priority_{};
    bool pinned_{};
    Style style_{Style::halo};
    QGraphicsLineItem* leader_{};
};
}
