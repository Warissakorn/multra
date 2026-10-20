#include "canvas.hpp"
#include "canvas_style.hpp"
#include <QGraphicsPathItem>
#include <QGraphicsSimpleTextItem>
#include <QMouseEvent>
#include <QPainterPath>
#include <QPen>

// M8.6a (D161, docs/reference/ZONES_AND_PARKING.md §9): the Zone tool and the zones' marks. A zone is
// a translucent polygon under every Link with its name at the centroid; a zone connector is a dashed
// line from the centroid to its Link's start (origin) or end (destination).
namespace trafficsim {
namespace {
Point centroid(const std::vector<Point>& polygon) {
    Point c{};
    for (const auto& p : polygon) { c.x += p.x; c.y += p.y; }
    if (!polygon.empty()) { c.x /= static_cast<double>(polygon.size()); c.y /= static_cast<double>(polygon.size()); }
    return c;
}
}
bool EditorCanvas::zonePress(QMouseEvent* e) {
    if ((tool_ != Tool::zone && tool_ != Tool::node) || !document_ || e->button() != Qt::RightButton || !(e->modifiers() & Qt::ControlModifier))
        return false; // D84: Ctrl+right-click authors; a left click only selects
    zoneDraft_.push_back(world(e->pos(), false));
    redraw();
    return true;
}
void EditorCanvas::commitZoneDraft() {
    if (zoneDraft_.size() < 3) return; // a polygon needs three corners; the draft stays
    auto polygon = zoneDraft_;
    zoneDraft_.clear();
    redraw();
    const auto& committed = tool_ == Tool::node ? nodeCommitted : zoneCommitted;
    if (committed && !committed(polygon)) { zoneDraft_ = std::move(polygon); redraw(); }
}
void EditorCanvas::drawZones() {
    if (!document_) return;
    const auto& layer = document_->zones;
    const auto outline = [&](const std::vector<Point>& polygon, bool draft) {
        QPainterPath shape;
        for (std::size_t i = 0; i < polygon.size(); ++i)
            i ? shape.lineTo(polygon[i].x, polygon[i].y) : shape.moveTo(polygon[i].x, polygon[i].y);
        if (!draft) shape.closeSubpath();
        QPen pen(draft ? canvasStyle::active() : canvasStyle::ink(), 2, Qt::DashLine);
        pen.setCosmetic(true);
        QColor wash = canvasStyle::advisory(); wash.setAlpha(draft ? 20 : 40);
        auto* item = scene_.addPath(shape, pen, draft ? QBrush(Qt::NoBrush) : QBrush(wash));
        item->setZValue(draft ? 200007 : -150);
        return item;
    };
    for (const auto& zone : layer.zones) {
        if (zone.polygon.size() < 3) continue;
        auto* item = outline(zone.polygon, false);
        item->setData(0, QStringLiteral("zone")); item->setData(1, QString::fromStdString(zone.id));
        const auto c = centroid(zone.polygon);
        auto* label = scene_.addSimpleText(QString::fromStdString(zone.name.empty() ? zone.id : zone.name));
        label->setFlag(QGraphicsItem::ItemIgnoresTransformations);
        label->setBrush(canvasStyle::ink()); label->setPos(c.x, c.y); label->setZValue(-149);
        for (const auto& connector : layer.zoneConnectors) {
            if (connector.zoneId != zone.id) continue;
            for (const auto& link : document_->network.links) {
                if (link.id != connector.linkId || link.geometry.empty()) continue;
                const auto end = connector.direction == ZoneDirection::origin ? link.geometry.front() : link.geometry.back();
                QPen pen(canvasStyle::advisory(), 1.5, Qt::DashLine); pen.setCosmetic(true);
                auto* line = scene_.addLine(c.x, c.y, end.x, end.y, pen);
                line->setZValue(link.level * 100. + 11.4);
                line->setData(0, QStringLiteral("zone-connector")); line->setData(1, QString::fromStdString(connector.id));
            }
        }
    }
    if ((tool_ == Tool::zone || tool_ == Tool::node) && !zoneDraft_.empty()) {
        outline(zoneDraft_, true);
        const double r = 3 / viewScale();
        QPen pen(canvasStyle::active(), 2); pen.setCosmetic(true);
        for (const auto& p : zoneDraft_) scene_.addEllipse(p.x - r, p.y - r, 2 * r, 2 * r, pen)->setZValue(200008);
    }
}
// S0.1 (NODES §6): a Node is a solid outline over the Links with its name at the centroid.
void EditorCanvas::drawNodes() {
    if (!document_) return;
    for (const auto& node : document_->network.nodes) {
        if (node.polygon.size() < 3) continue;
        QPainterPath shape;
        for (std::size_t i = 0; i < node.polygon.size(); ++i)
            i ? shape.lineTo(node.polygon[i].x, node.polygon[i].y) : shape.moveTo(node.polygon[i].x, node.polygon[i].y);
        shape.closeSubpath();
        QPen pen(canvasStyle::ink(), node.generated ? 1.5 : 2, node.generated ? Qt::DotLine : Qt::SolidLine);
        pen.setCosmetic(true);
        auto* item = scene_.addPath(shape, pen, QBrush(Qt::NoBrush));
        item->setZValue(200005);
        item->setData(0, QStringLiteral("node")); item->setData(1, QString::fromStdString(node.id));
        const auto c = centroid(node.polygon);
        auto* label = scene_.addSimpleText(QString::fromStdString(node.name.empty() ? node.id : node.name));
        label->setFlag(QGraphicsItem::ItemIgnoresTransformations);
        label->setBrush(canvasStyle::ink()); label->setPos(c.x, c.y); label->setZValue(200006);
    }
}
}
