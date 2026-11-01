#include "canvas.hpp"
#include "canvas_style.hpp"
#include "canvas_labels.hpp"
#include <QGuiApplication>
#include "../model/network/node_membership.hpp"
#include <QGraphicsPathItem>
#include <algorithm>
#include <limits>
#include <QGraphicsSimpleTextItem>
#include <QMouseEvent>
#include <QPainterPath>
#include <QPainterPathStroker>
#include <QPen>

// M8.6a (D161, docs/reference/ZONES_AND_PARKING.md §9): the Zone tool and the zones' marks. A zone is
// a translucent polygon under every Link with its name at the centroid; a zone connector is a dashed
// line from the centroid to its Link's start (origin) or end (destination).
namespace multra {
namespace {
Point centroid(const std::vector<Point>& polygon) {
    Point c{};
    for (const auto& p : polygon) { c.x += p.x; c.y += p.y; }
    if (!polygon.empty()) { c.x /= static_cast<double>(polygon.size()); c.y /= static_cast<double>(polygon.size()); }
    return c;
}
}
bool EditorCanvas::zonePress(QMouseEvent* e) {
    if (!polygonTool() || !document_ || !authoringPress(e, !zoneDraft_.empty()))
        return false; // D84/D306: Ctrl+right-click starts the draft; then plain clicks add corners
    zoneDraft_.push_back(world(e->pos(), false));
    redraw();
    return true;
}
void EditorCanvas::commitZoneDraft() {
    // A polygon needs three corners and a StreetSegment's centreline two points; short of that the draft stays.
    if (zoneDraft_.size() < (tool_ == Tool::road ? 2u : 3u)) return;
    auto polygon = zoneDraft_;
    zoneDraft_.clear(); draftPointer_.reset();
    redraw();
    const auto& committed = tool_ == Tool::node ? nodeCommitted : tool_ == Tool::road ? streetSegmentCommitted : zoneCommitted;
    if (committed && !committed(polygon)) { zoneDraft_ = std::move(polygon); redraw(); }
}
void EditorCanvas::drawZones() {
    if (!document_) return;
    const auto& layer = document_->zones;
    const auto outline = [&](const std::vector<Point>& polygon, bool draft) {
        QPainterPath shape;
        for (std::size_t i = 0; i < polygon.size(); ++i)
            i ? shape.lineTo(polygon[i].x, polygon[i].y) : shape.moveTo(polygon[i].x, polygon[i].y);
        // A saved zone keeps its dashed ink outline (it is not a Node); a draft is a preview,
        // solid on its casing (D306).
        if (draft) return addPreviewPath(shape, 200007, canvasStyle::previewPen(canvasStyle::active()));
        shape.closeSubpath();
        QPen pen(canvasStyle::ink(), 2, Qt::DashLine); pen.setCosmetic(true);
        QColor wash = canvasStyle::advisory(); wash.setAlpha(40);
        auto* item = scene_.addPath(shape, pen, QBrush(wash));
        item->setZValue(-150);
        return item;
    };
    for (const auto& zone : layer.zones) {
        if (zone.polygon.size() < 3) continue;
        auto* item = outline(zone.polygon, false);
        item->setData(0, QStringLiteral("zone")); item->setData(1, QString::fromStdString(zone.id));
        const auto c = centroid(zone.polygon);
        addLabel(QString::fromStdString(zone.name.empty() ? zone.id : zone.name), c, QGuiApplication::font(),
                 canvasStyle::ink(), -149, {}, {}, CanvasLabel::zone, false); // D424
        for (const auto& connector : layer.zoneConnectors) {
            if (connector.zoneId != zone.id) continue;
            const auto draw = [&](Point end, int level) {
                QPen pen(canvasStyle::advisory(), 1.5, Qt::DashLine); pen.setCosmetic(true);
                auto* line = scene_.addLine(c.x, c.y, end.x, end.y, pen);
                line->setZValue(level * 100. + 11.4);
                line->setData(0, QStringLiteral("zone-connector")); line->setData(1, QString::fromStdString(connector.id));
            };
            for (const auto& link : document_->network.links) {
                if (link.id != connector.linkId || link.geometry.empty()) continue;
                const auto end = connector.direction == ZoneDirection::origin ? link.geometry.front() : link.geometry.back();
                draw(end, link.level);
            }
            for (const auto& road : document_->network.streetSegments) {
                if (road.id != connector.streetSegmentId || road.centreline.empty()) continue;
                const bool atEnd = connector.streetSegmentForward == (connector.direction == ZoneDirection::destination);
                draw(atEnd ? road.centreline.back() : road.centreline.front(), road.level);
            }
        }
    }
    if (polygonTool() && !zoneDraft_.empty()) {
        outline(zoneDraft_, true)->setData(0, QStringLiteral("zone-draft"));
        // The edges still to come, fainter: last corner to the pointer to the first corner, or
        // with no pointer yet the closing edge alone.
        std::vector<Point> pending{zoneDraft_.back()};
        if (draftPointer_) pending.push_back(*draftPointer_);
        // A StreetSegment's centreline is open: only the edge to the pointer is still to come.
        if (tool_ != Tool::road && (zoneDraft_.size() > 1 || draftPointer_)) pending.push_back(zoneDraft_.front());
        if (pending.size() > 1) {
            QPainterPath edges(QPointF(pending[0].x, pending[0].y));
            for (std::size_t i = 1; i < pending.size(); ++i) edges.lineTo(pending[i].x, pending[i].y);
            auto* item = scene_.addPath(edges, canvasStyle::pendingPen()); item->setZValue(200006.9);
            item->setData(0, QStringLiteral("zone-draft-pending"));
        }
        const double r = 3 / viewScale();
        for (const auto& p : zoneDraft_)
            scene_.addEllipse(p.x - r, p.y - r, 2 * r, 2 * r, canvasStyle::previewPen(canvasStyle::active(), 1.5),
                              QBrush(editorDesign::role(QPalette::Base)))->setZValue(200008);
    }
}
// S0.1 (NODES §6): a Node is a solid outline over the Links with its name at the centroid.
void EditorCanvas::setHighlightedNode(std::string id) {
    if (highlightedNode_ == id) return;
    highlightedNode_ = std::move(id); redraw();
}
void EditorCanvas::drawNodes() {
    if (!document_) return;
    for (const auto& node : document_->network.nodes) {
        if (node.polygon.size() < 3) continue;
        QPainterPath shape;
        for (std::size_t i = 0; i < node.polygon.size(); ++i)
            i ? shape.lineTo(node.polygon[i].x, node.polygon[i].y) : shape.moveTo(node.polygon[i].x, node.polygon[i].y);
        shape.closeSubpath();
        // D423 (Q5 b): a Node's polygon is junction road -- the default asphalt with its kerb, on the
        // ground level, so no gap between its Connectors reads as a hole. Wireframe draws no surface.
        if (!wireframe_ && levelVisible(0)) {
            auto* surface = scene_.addPath(shape, QPen(Qt::NoPen),
                                           QBrush(QColor(QString::fromStdString(style("default").linkColor))));
            surface->setZValue(canvasStyle::surfaceLayer);
            surface->setData(0, QStringLiteral("node-surface")); surface->setData(1, QString::fromStdString(node.id));
            // A Node has no lanes of its own: its kerb shows when the narrowest lane's markings do.
            double narrowest = std::numeric_limits<double>::infinity();
            for (const auto& link : document_->network.links)
                for (const auto& lane : link.lanes) narrowest = std::min(narrowest, lane.width);
            addKerb(shape, 0, narrowest, node.id);
        }
        const bool lit = node.id == highlightedNode_;
        QPen pen(lit ? canvasStyle::selection() : canvasStyle::ink(), lit ? 2.5 : node.generated ? 1.5 : 2,
                 node.generated ? Qt::DotLine : Qt::SolidLine);
        pen.setCosmetic(true);
        // S5.2b (D328): a study-area Node is filled lightly under its usual outline.
        const auto& area = document_->zones.studyArea.nodes;
        const bool inArea = std::find(area.begin(), area.end(), node.id) != area.end();
        auto fill = canvasStyle::ok(); fill.setAlpha(40);
        auto* item = scene_.addPath(shape, pen, inArea ? QBrush(fill) : QBrush(Qt::NoBrush));
        item->setZValue(200005);
        item->setData(0, QStringLiteral("node")); item->setData(1, QString::fromStdString(node.id));
        if (inArea) item->setData(2, QStringLiteral("area"));
        const auto c = centroid(node.polygon);
        addLabel(QString::fromStdString(node.name.empty() ? node.id : node.name), c, QGuiApplication::font(),
                 canvasStyle::ink(), 200006, {}, {}, CanvasLabel::node, lit); // D424
        if (lit) drawNodeLegs(node);
    }
}
// S0.7 (NODES §16): the highlighted Node's legs, each Link's span outside the polygon up to its
// crossing, clipped to the Link's own surface. In-legs are solid, out-legs dashed. A Node with an
// issue has no legs (§3), so only its outline is lit.
void EditorCanvas::drawNodeLegs(const Node& node) {
    const auto& n = document_->network;
    const auto m = nodeMembership(n, node);
    const auto draw = [&](const NodeLeg& leg, bool in) {
        const auto link = std::find_if(n.links.begin(), n.links.end(), [&](const Link& l) { return l.id == leg.linkId; });
        if (link == n.links.end() || !levelVisible(link->level)) return;
        const auto span = in ? polylineSpan(link->geometry, 0, leg.station)
                             : polylineSpan(link->geometry, leg.station, polylineLength(link->geometry));
        if (span.size() < 2) return;
        QPainterPath line; line.moveTo(span[0].x, span[0].y);
        for (std::size_t i = 1; i < span.size(); ++i) line.lineTo(span[i].x, span[i].y);
        double width = 0;
        for (const auto& lane : link->lanes) width += lane.width;
        QPainterPathStroker corridor; corridor.setWidth(4 * width + 2); corridor.setCapStyle(Qt::FlatCap);
        const auto shape = objectShape(link->id).intersected(corridor.createStroke(line));
        if (shape.isEmpty()) return;
        auto tint = canvasStyle::selection(); tint.setAlpha(100);
        QPen pen(canvasStyle::selection(), 2, in ? Qt::SolidLine : Qt::DashLine); pen.setCosmetic(true);
        auto* item = scene_.addPath(shape, pen, in ? QBrush(tint) : QBrush(tint, Qt::BDiagPattern));
        item->setZValue(200004);
        item->setData(0, QStringLiteral("node-leg")); item->setData(1, QString::fromStdString(leg.linkId));
        item->setData(2, QString::fromLatin1(in ? "in" : "out"));
    };
    for (const auto& leg : m.inLegs) draw(leg, true);
    for (const auto& leg : m.outLegs) draw(leg, false);
}
}
