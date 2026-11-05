#pragma once
#include <QGraphicsSimpleTextItem>
#include <QColor>
#include <QPointF>
#include <optional>

class QGraphicsLineItem;
namespace multra {
// D424/D425 (canvas style slices B and B2): a canvas label is upright, fixed-size screen text.
// A halo label is text stroked in Base (Node and zone names); a box label is text in a white box
// with a grey frame and a leader line to its anchor (input rates and decision names, in the
// spirit of PTV Vistro's fixed-size callouts). The canvas places it in screen space
// (EditorCanvas::placeLabels); a label that finds no free place is hidden unless pinned.
//
// D460: an input rate may instead lie on its own lanes, along the road, while its text fits inside
// them; zoomed out past that it hides (a pinned one falls back to its box). Box heights follow the
// text's ink, not the font's line height. A tag label (a selected Node's name) is a tight framed box
// centred on its anchor, with no halo and no leader.
class CanvasLabel : public QGraphicsSimpleTextItem {
public:
    // Where an on-lane label may go: the input's bar runs across lanes firstLane.. of one Link
    // (`group`) from `from` to `to` (scene), the road leaving along unit `direction` and straight
    // for `reach` metres. A label takes the middle of one of those lanes; `surface` is the road's colour.
    struct LaneSlot { QPointF from, to, direction; double reach{}; int firstLane{}, laneCount{1}; QString group; QColor surface; };
    static constexpr double halo = 3;              // logical px of Base around each glyph
    static constexpr double padX = 4, padY = 2;    // box padding
    static constexpr double radius = 3;            // box corner
    enum Priority { zone = 0, node = 1, decision = 2, input = 3 };
    enum class Style { halo, box, tag }; // tag (D460): a tight frame centred on its anchor, never moved
    CanvasLabel(const QString& text, const QFont& font, QPointF anchor, int priority, bool pinned, Style style = Style::halo);
    QPointF anchor() const { return anchor_; }
    int priority() const { return priority_; }
    bool pinned() const { return pinned_; }
    // D464: an exported image pins every rate so none hides, without the accent frame of a selection.
    void setAccent(bool on) { accent_ = on; update(); }
    Style style() const { return style_; }
    QGraphicsLineItem* leader() const { return leader_; }
    void setLeader(QGraphicsLineItem* line) { leader_ = line; }
    const std::optional<LaneSlot>& laneSlot() const { return lane_; }
    void setLaneSlot(const LaneSlot& slot) { lane_ = slot; }
    bool onLane() const { return onLane_; }
    void setOnLane(bool on) { prepareGeometryChange(); onLane_ = on; }
    QRectF textRect() const { return QGraphicsSimpleTextItem::boundingRect(); }
    QRectF frame() const; // the box in item pixels (the text rect for a halo label)
    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override;
private:
    QPointF anchor_;
    int priority_{};
    bool pinned_{};
    bool accent_{};
    Style style_{Style::halo};
    QGraphicsLineItem* leader_{};
    std::optional<LaneSlot> lane_;
    bool onLane_{};
};
}
