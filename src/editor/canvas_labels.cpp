#include "canvas.hpp"
#include "canvas_labels.hpp"
#include "canvas_style.hpp"
#include <QPainter>
#include <QPainterPath>
#include <algorithm>
#include <array>

namespace multra {
CanvasLabel::CanvasLabel(const QString& text, const QFont& font, QPointF anchor, int priority, bool pinned)
    : QGraphicsSimpleTextItem(text), anchor_(anchor), priority_(priority), pinned_(pinned) {
    setFont(font);
    setFlag(QGraphicsItem::ItemIgnoresTransformations);
    setPos(anchor);
}
QRectF CanvasLabel::boundingRect() const {
    return QGraphicsSimpleTextItem::boundingRect().adjusted(-halo, -halo, halo, halo);
}
QPainterPath CanvasLabel::shape() const { QPainterPath path; path.addRect(boundingRect()); return path; }
void CanvasLabel::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) {
    // The glyph outlines, stroked wide in Base, then filled in the label's brush.
    QPainterPath glyphs;
    const QFontMetricsF metrics(font());
    glyphs.addText(0, metrics.ascent(), font(), text());
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->strokePath(glyphs, QPen(editorDesign::role(QPalette::Base), 2 * halo, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter->fillPath(glyphs, brush());
    painter->restore();
}
CanvasLabel* EditorCanvas::addLabel(const QString& text, Point anchor, const QFont& font, const QBrush& brush,
                                    double z, const QString& tag, const std::string& id, int priority, bool pinned) {
    auto* label = new CanvasLabel(text, font, QPointF(anchor.x, anchor.y), priority, pinned);
    label->setBrush(brush); label->setZValue(z);
    if (!tag.isEmpty()) label->setData(0, tag);
    if (!id.empty()) label->setData(1, QString::fromStdString(id));
    scene_.addItem(label); labels_.push_back(label);
    return label;
}
// D424: pinned labels first, then by priority, then in drawing order. Each takes the first of
// right, left, above and below its anchor whose screen rectangle meets no label already placed;
// a pinned label always takes the right. The view's rotation leaves labels upright, so the
// rectangles are axis-aligned and the test is exact at any heading.
void EditorCanvas::placeLabels() {
    std::vector<CanvasLabel*> order(labels_.begin(), labels_.end());
    std::stable_sort(order.begin(), order.end(), [](const CanvasLabel* a, const CanvasLabel* b) {
        if (a->pinned() != b->pinned()) return a->pinned();
        return a->priority() > b->priority();
    });
    std::vector<QRectF> placed;
    const double gap = editorDesign::space2, margin = 2;
    for (auto* label : order) {
        const auto text = label->QGraphicsSimpleTextItem::boundingRect();
        const QPointF at = mapFromScene(label->anchor());
        const std::array<QPointF, 4> offsets{QPointF(gap, -text.height() / 2), QPointF(-gap - text.width(), -text.height() / 2),
                                             QPointF(-text.width() / 2, -gap - text.height()), QPointF(-text.width() / 2, gap)};
        bool shown = false;
        for (std::size_t i = 0; i < offsets.size() && !shown; ++i) {
            const QRectF rect = QRectF(at + offsets[i], text.size()).adjusted(-margin, -margin, margin, margin);
            const bool free = std::none_of(placed.begin(), placed.end(), [&](const QRectF& r) { return r.intersects(rect); });
            if (!free && !(label->pinned() && i == 0)) continue;
            label->setTransform(QTransform::fromTranslate(offsets[i].x(), offsets[i].y()));
            placed.push_back(rect); shown = true;
        }
        label->setVisible(shown);
        label->setData(4, shown ? QStringLiteral("shown") : QStringLiteral("hidden"));
    }
}
}
