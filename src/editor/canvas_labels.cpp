#include "canvas.hpp"
#include "canvas_labels.hpp"
#include "canvas_style.hpp"
#include <QGraphicsLineItem>
#include <QPainter>
#include <QPainterPath>
#include <algorithm>
#include <cmath>
#include <vector>

namespace multra {
CanvasLabel::CanvasLabel(const QString& text, const QFont& font, QPointF anchor, int priority, bool pinned, Style style)
    : QGraphicsSimpleTextItem(text), anchor_(anchor), priority_(priority), pinned_(pinned), style_(style) {
    setFont(font);
    setFlag(QGraphicsItem::ItemIgnoresTransformations);
    setPos(anchor);
}
QRectF CanvasLabel::frame() const {
    const auto text = textRect();
    return style_ == Style::box ? text.adjusted(-padX, -padY, padX, padY) : text;
}
QRectF CanvasLabel::boundingRect() const {
    return style_ == Style::box ? frame().adjusted(-1, -1, 1, 1) : textRect().adjusted(-halo, -halo, halo, halo);
}
QPainterPath CanvasLabel::shape() const { QPainterPath path; path.addRect(boundingRect()); return path; }
void CanvasLabel::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) {
    QPainterPath glyphs;
    const QFontMetricsF metrics(font());
    glyphs.addText(0, metrics.ascent(), font(), text());
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    if (style_ == Style::box) {
        // D425: a white box, grey frame (accent and heavier when pinned), then the text.
        QPen pen(pinned_ ? canvasStyle::selection() : editorDesign::role(QPalette::Dark), pinned_ ? 1.5 : 1);
        painter->setPen(pen); painter->setBrush(editorDesign::role(QPalette::Base));
        painter->drawRoundedRect(frame().adjusted(.5, .5, -.5, -.5), radius, radius);
    } else {
        // D424: the glyph outlines, stroked wide in Base, then filled in the label's brush.
        painter->strokePath(glyphs, QPen(editorDesign::role(QPalette::Base), 2 * halo, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    }
    painter->fillPath(glyphs, brush());
    painter->restore();
}
CanvasLabel* EditorCanvas::addLabel(const QString& text, Point anchor, const QFont& font, const QBrush& brush,
                                    double z, const QString& tag, const std::string& id, int priority, bool pinned,
                                    CanvasLabel::Style style) {
    auto* label = new CanvasLabel(text, font, QPointF(anchor.x, anchor.y), priority, pinned, style);
    label->setBrush(brush); label->setZValue(z);
    if (!tag.isEmpty()) label->setData(0, tag);
    if (!id.empty()) label->setData(1, QString::fromStdString(id));
    scene_.addItem(label); labels_.push_back(label);
    if (style == CanvasLabel::Style::box) {
        // D425: the leader ties the box to what it describes; it is drawn, never hit.
        QPen pen(editorDesign::role(QPalette::Dark), 1); pen.setCosmetic(true);
        auto* leader = scene_.addLine(QLineF(label->anchor(), label->anchor()), pen);
        leader->setZValue(z - .1); leader->setData(0, QStringLiteral("label-leader"));
        if (!id.empty()) leader->setData(1, QString::fromStdString(id));
        leader->setAcceptedMouseButtons(Qt::NoButton);
        label->setLeader(leader);
    }
    return label;
}
namespace {
// The point of a rectangle nearest to p (p itself when inside).
QPointF nearestOn(const QRectF& r, QPointF p) {
    return {std::clamp(p.x(), r.left(), r.right()), std::clamp(p.y(), r.top(), r.bottom())};
}
}
// D424/D425: pinned labels first, then by priority, then in drawing order. A halo label tries right,
// left, above and below its anchor; a box tries eight directions at 12 px, then at 32 px. Each takes
// the first screen rectangle that meets no label already placed, else hides (with its leader); a
// pinned label always takes its first place. Labels stay upright under any view rotation or flip,
// so the rectangles are axis-aligned and the test is exact.
void EditorCanvas::placeLabels() {
    std::vector<CanvasLabel*> order(labels_.begin(), labels_.end());
    std::stable_sort(order.begin(), order.end(), [](const CanvasLabel* a, const CanvasLabel* b) {
        if (a->pinned() != b->pinned()) return a->pinned();
        return a->priority() > b->priority();
    });
    std::vector<QRectF> placed;
    const double gap = editorDesign::space2, margin = 2;
    for (auto* label : order) {
        const auto box = label->frame(), text = label->textRect();
        const QPointF at = mapFromScene(label->anchor());
        // Candidate positions of the frame's top-left corner, in screen pixels from the anchor.
        std::vector<QPointF> corners;
        if (label->style() == CanvasLabel::Style::box) {
            for (const double reach : {12.0, 32.0})
                for (const auto [dx, dy] : {std::pair{1, 0}, {-1, 0}, {0, -1}, {0, 1}, {1, -1}, {-1, -1}, {1, 1}, {-1, 1}}) {
                    const double x = dx > 0 ? reach : dx < 0 ? -reach - box.width() : -box.width() / 2;
                    const double y = dy > 0 ? reach : dy < 0 ? -reach - box.height() : -box.height() / 2;
                    corners.push_back({x, y});
                }
        } else {
            corners = {QPointF(gap, -text.height() / 2), QPointF(-gap - text.width(), -text.height() / 2),
                       QPointF(-text.width() / 2, -gap - text.height()), QPointF(-text.width() / 2, gap)};
        }
        bool shown = false;
        for (std::size_t i = 0; i < corners.size() && !shown; ++i) {
            const QRectF rect = QRectF(at + corners[i], box.size()).adjusted(-margin, -margin, margin, margin);
            const bool free = std::none_of(placed.begin(), placed.end(), [&](const QRectF& r) { return r.intersects(rect); });
            if (!free && !(label->pinned() && i == 0)) continue;
            const QPointF shift = corners[i] - box.topLeft();
            label->setTransform(QTransform::fromTranslate(shift.x(), shift.y()));
            placed.push_back(rect); shown = true;
            if (auto* leader = label->leader()) {
                const QPointF end = nearestOn(QRectF(at + corners[i], box.size()), at);
                leader->setLine(QLineF(label->anchor(), viewportTransform().inverted().map(end)));
            }
        }
        label->setVisible(shown);
        if (auto* leader = label->leader()) leader->setVisible(shown);
        label->setData(4, shown ? QStringLiteral("shown") : QStringLiteral("hidden"));
    }
}
}
