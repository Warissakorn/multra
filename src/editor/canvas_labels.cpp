#include "canvas.hpp"
#include "canvas_labels.hpp"
#include "canvas_style.hpp"
#include "canvas_hud.hpp"
#include <QLabel>
#include <QGraphicsLineItem>
#include <QPainter>
#include <QPainterPath>
#include <algorithm>
#include <cmath>
#include <cstdint>
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
// Labels that are placed together: one halo label, or the box labels whose anchors chain within
// clusterReach screen px of each other (D426), stacked as one column.
struct LabelUnit {
    std::vector<CanvasLabel*> members;
    bool pinned{};
    int priority{};
    std::size_t first{}; // drawing order of its earliest member
};
constexpr double clusterReach = 48, stackGap = 2, margin = 2;
}
// D424-D426: labels are placed once per redraw, in screen space, at their exact device points.
// Box labels whose anchors lie within 48 px of each other (single linkage) form one unit: a column
// of boxes 2 px apart -- pinned, then by priority, then top to bottom by anchor -- aligned on the
// side that faces the anchors, each box on its own leader. A unit tries E, W, N, S and the four
// diagonals of its anchors' bounding box at 12, 32 and 64 px (a halo label: right, left, above,
// below). It takes the first place that meets no placed label and no other drawn object; failing
// that the first that meets no placed label (covering an object only when it must); failing that
// a pinned unit may overlap another label, but every unit stays inside the viewport and away
// from HUD/map overlays; a unit without a usable place hides whole, leaders too. Labels stay
// upright under any rotation or flip, so the rectangles are axis-aligned and the tests exact.
void EditorCanvas::placeLabels() {
    const QTransform toScreen = viewportTransform(), toScene = toScreen.inverted();
    const bool exporting=property("editorImageRendering").toBool();
    const QRectF viewportBounds=QRectF(viewport()->rect()).adjusted(4,4,-4,-4);
    std::vector<QRectF> overlays;
    for(auto* widget:{static_cast<QWidget*>(hud_),static_cast<QWidget*>(mapCredit_),static_cast<QWidget*>(mapStatus_)})
        if(!exporting&&widget&&widget->isVisibleTo(viewport()))overlays.push_back(QRectF(widget->geometry()).adjusted(-4,-4,4,4));
    std::vector<QPointF> at(labels_.size());
    for (std::size_t i = 0; i < labels_.size(); ++i) at[i] = toScreen.map(labels_[i]->anchor());
    // Units: union-find over box anchors.
    std::vector<std::size_t> parent(labels_.size());
    for (std::size_t i = 0; i < parent.size(); ++i) parent[i] = i;
    const auto root = [&](std::size_t i) { while (parent[i] != i) i = parent[i] = parent[parent[i]]; return i; };
    for (std::size_t i = 0; i < labels_.size(); ++i)
        for (std::size_t j = i + 1; j < labels_.size(); ++j)
            if (labels_[i]->style() == CanvasLabel::Style::box && labels_[j]->style() == CanvasLabel::Style::box &&
                QLineF(at[i], at[j]).length() <= clusterReach)
                parent[root(j)] = root(i);
    std::vector<LabelUnit> units;
    std::vector<std::size_t> unitOf(labels_.size(), SIZE_MAX);
    for (std::size_t i = 0; i < labels_.size(); ++i) {
        auto& slot = unitOf[root(i)];
        if (slot == SIZE_MAX) { slot = units.size(); units.push_back({{}, false, 0, i}); }
        auto& unit = units[slot];
        unit.members.push_back(labels_[i]);
        unit.pinned = unit.pinned || labels_[i]->pinned();
        unit.priority = std::max(unit.priority, labels_[i]->priority());
    }
    std::stable_sort(units.begin(), units.end(), [](const LabelUnit& a, const LabelUnit& b) {
        if (a.pinned != b.pinned) return a.pinned;
        if (a.priority != b.priority) return a.priority > b.priority;
        return a.first < b.first;
    });
    const auto screenOf = [&](const CanvasLabel* l) {
        const auto found = std::find(labels_.begin(), labels_.end(), l);
        return at[static_cast<std::size_t>(found - labels_.begin())];
    };
    // Another drawn object under a screen rectangle: anything visible that is not a label, a leader,
    // the background, a zone wash, feedback, the run, or one of this unit's own anchors.
    const auto coversObject = [&](const QRectF& rect, const LabelUnit& unit) {
        const auto blocks=[&](const QGraphicsItem* item) {
            if(dynamic_cast<const CanvasLabel*>(item)||item->zValue() < -100)return false;
            const auto tag=item->data(0).toString();
            if(tag==QStringLiteral("label-leader")||tag==QStringLiteral("object-feedback")||tag==QStringLiteral("zone")||
               tag==QStringLiteral("demand-hover")||tag.startsWith(QStringLiteral("run")))return false;
            const auto id=item->data(1).toString();
            return id.isEmpty()||std::none_of(unit.members.begin(),unit.members.end(),[&](const CanvasLabel* l){return l->data(1).toString()==id;});
        };
        const auto polygon=toScene.map(QPolygonF(rect));
        for(const auto* item:scene_.items(polygon,Qt::IntersectsItemShape))if(blocks(item))return true;
        // Baked roads are hidden scene items, but labels must avoid the same geometry with either
        // renderer. QGraphicsScene's spatial query omits them, so inspect their exact shapes too.
        QPainterPath probe;probe.addPolygon(polygon);
        for(const auto* item:runBaked_)if(blocks(item)&&item->sceneBoundingRect().intersects(probe.boundingRect())&&
            item->shape().intersects(item->mapFromScene(probe)))return true;
        return false;
    };
    std::vector<QRectF> placed;
    for (auto& unit : units) {
        auto& members = unit.members;
        std::stable_sort(members.begin(), members.end(), [&](const CanvasLabel* a, const CanvasLabel* b) {
            if (a->pinned() != b->pinned()) return a->pinned();
            if (a->priority() != b->priority()) return a->priority() > b->priority();
            const auto pa = screenOf(a), pb = screenOf(b);
            return pa.y() != pb.y() ? pa.y() < pb.y() : pa.x() < pb.x();
        });
        // Candidate layouts: each a frame rectangle per member, in screen pixels.
        std::vector<std::vector<QRectF>> layouts;
        if (members.size() == 1 && members.front()->style() == CanvasLabel::Style::halo) {
            const auto text = members.front()->textRect();
            const QPointF p = screenOf(members.front());
            const double gap = editorDesign::space2;
            for (const QPointF corner : {QPointF(gap, -text.height() / 2), QPointF(-gap - text.width(), -text.height() / 2),
                                         QPointF(-text.width() / 2, -gap - text.height()), QPointF(-text.width() / 2, gap)})
                layouts.push_back({QRectF(p + corner, text.size())});
        } else {
            QRectF anchors(screenOf(members.front()), QSizeF(0, 0));
            double width = 0, height = -stackGap;
            for (const auto* l : members) {
                const auto p = screenOf(l);
                anchors = anchors.united(QRectF(p, QSizeF(0, 0)));
                width = std::max(width, l->frame().width()); height += l->frame().height() + stackGap;
            }
            for (const double reach : {12.0, 32.0, 64.0})
                for (const auto [dx, dy] : {std::pair{1, 0}, {-1, 0}, {0, -1}, {0, 1}, {1, -1}, {-1, -1}, {1, 1}, {-1, 1}}) {
                    const double x = dx > 0 ? anchors.right() + reach : dx < 0 ? anchors.left() - reach - width : anchors.center().x() - width / 2;
                    double y = dy > 0 ? anchors.bottom() + reach : dy < 0 ? anchors.top() - reach - height : anchors.center().y() - height / 2;
                    std::vector<QRectF> layout;
                    for (const auto* l : members) {
                        const auto size = l->frame().size();
                        const double left = dx > 0 ? x : dx < 0 ? x + width - size.width() : x + (width - size.width()) / 2;
                        layout.push_back(QRectF(QPointF(left, y), size));
                        y += size.height() + stackGap;
                    }
                    layouts.push_back(std::move(layout));
                }
        }
        const auto meetsLabel = [&](const std::vector<QRectF>& layout) {
            return std::any_of(layout.begin(), layout.end(), [&](const QRectF& r) {
                const auto padded = r.adjusted(-margin, -margin, margin, margin);
                return std::any_of(placed.begin(), placed.end(), [&](const QRectF& p) { return p.intersects(padded); });
            });
        };
        const auto meetsObject = [&](const std::vector<QRectF>& layout) {
            return std::any_of(layout.begin(), layout.end(), [&](const QRectF& r) { return coversObject(r, unit); });
        };
        const auto fitsView=[&](const std::vector<QRectF>& layout){
            return std::all_of(layout.begin(),layout.end(),[&](const QRectF& r){
                return (exporting||viewportBounds.contains(r.adjusted(-1,-1,1,1)))&&
                    std::none_of(overlays.begin(),overlays.end(),[&](const QRectF& overlay){return overlay.intersects(r);});
            });
        };
        const std::vector<QRectF>* chosen = nullptr;
        for (const auto& layout : layouts) if (fitsView(layout) && !meetsLabel(layout) && !meetsObject(layout)) { chosen = &layout; break; }
        if (!chosen) for (const auto& layout : layouts) if (fitsView(layout) && !meetsLabel(layout)) { chosen = &layout; break; }
        if(!chosen&&unit.pinned)for(const auto& layout:layouts)if(fitsView(layout)){chosen=&layout;break;}
        for (std::size_t i = 0; i < members.size(); ++i) {
            auto* label = members[i];
            const bool shown = chosen != nullptr;
            if (shown) {
                const auto& rect = (*chosen)[i];
                const QPointF p = screenOf(label);
                const QPointF shift = rect.topLeft() - p - label->frame().topLeft();
                label->setTransform(QTransform::fromTranslate(shift.x(), shift.y()));
                placed.push_back(rect.adjusted(-margin, -margin, margin, margin));
                if (auto* leader = label->leader()) leader->setLine(QLineF(label->anchor(), toScene.map(nearestOn(rect, p))));
            }
            label->setVisible(shown);
            if (auto* leader = label->leader()) leader->setVisible(shown);
            label->setData(4, shown ? QStringLiteral("shown") : QStringLiteral("hidden"));
        }
    }
}
}
