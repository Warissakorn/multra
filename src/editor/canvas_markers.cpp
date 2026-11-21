#include "canvas.hpp"
#include "canvas_style.hpp"
#include "../model/network/right_of_way.hpp"
#include <QGraphicsLineItem>
#include <QGraphicsPathItem>
#include <QPainterPath>
#include <QPen>
#include <algorithm>
#include <cmath>

// PRIORITY_RULES slice 2 (D616): a priority rule's conflict markers under the Conflict area and Stop line
// tools, as short green bars across their lanes (Vissim's green marker), picked like a stop line;
// D617: each marker's headway as a faint band upstream, and a dashed link to its line while picked.
namespace multra {
namespace {
int markerLevel(const Network& n, const ControlPathRef& ref) {
    for (const auto& l : n.links) if (l.id == ref.linkId) return l.level;
    for (const auto& c : n.connectors) if (c.id == ref.connectorId) return c.level;
    return 0;
}
double distanceTo(Point p, Point a, Point b) {
    const double dx = b.x - a.x, dy = b.y - a.y, n = dx * dx + dy * dy;
    const double t = n <= 0 ? 0 : std::clamp(((p.x - a.x) * dx + (p.y - a.y) * dy) / n, 0.0, 1.0);
    return std::hypot(p.x - a.x - t * dx, p.y - a.y - t * dy);
}
}
std::optional<EditorCanvas::MarkerRef> EditorCanvas::priorityMarkerAt(Point p) const {
    if (!document_) return std::nullopt;
    const auto& n = document_->network;
    double best = 4 / viewScale(); // four pixels either side, as a stop line is hit
    std::optional<MarkerRef> found;
    for (const auto& r : n.rightOfWay.priorityRules)
        for (std::size_t k = 0; k < r.markers.size(); ++k) {
            if (!levelVisible(markerLevel(n, r.markers[k].point.path))) continue;
            const auto bar = waitingLineBar(n, r.markers[k].point);
            if (!bar) continue;
            if (const double d = distanceTo(p, bar->first, bar->second); d <= best) { best = d; found = MarkerRef{r.id, k}; }
        }
    return found;
}
void EditorCanvas::drawPriorityMarkers() {
    const auto& n = document_->network;
    const auto mid = [](const std::pair<Point, Point>& bar) { return Point{(bar.first.x + bar.second.x) / 2, (bar.first.y + bar.second.y) / 2}; };
    for (const auto& r : n.rightOfWay.priorityRules) {
        // D617: a rule is picked through its line, one of its markers, or its table row.
        const bool picked = r.markerForm() && ((!selectedStopLine_.empty() && selectedStopLine_ == r.waitingLineId) ||
            (selectedMarker_ && selectedMarker_->first == r.id) || highlightedConflict_ == r.id);
        std::optional<std::pair<Point, Point>> lineBar;
        if (picked) for (const auto& w : n.rightOfWay.waitingLines) if (w.id == r.waitingLineId) lineBar = waitingLineBar(n, w.point);
        for (std::size_t k = 0; k < r.markers.size(); ++k) {
            const auto& marker = r.markers[k];
            if (!levelVisible(markerLevel(n, marker.point.path))) continue;
            // The headway as a faint band upstream on the major lane (§5): a standing vehicle in it holds the line.
            if (marker.headway > 0) {
                const auto polyline = controlPathPolyline(n, marker.point.path);
                if (polyline.size() >= 2) {
                    const auto band = polylineSpan(polyline, std::max(0.0, marker.point.station - marker.headway), marker.point.station);
                    if (band.size() >= 2) {
                        QPainterPath path(QPointF(band.front().x, band.front().y));
                        for (std::size_t i = 1; i < band.size(); ++i) path.lineTo(band[i].x, band[i].y);
                        QColor green = canvasStyle::conflictPriority(); green.setAlpha(70);
                        QPen pen(green, 6); pen.setCosmetic(true); pen.setCapStyle(Qt::FlatCap);
                        auto* item = scene_.addPath(path, pen);
                        item->setZValue(markerLevel(n, marker.point.path) * 100. + 6);
                        item->setData(0, QStringLiteral("priority-headway")); item->setData(1, QString::fromStdString(r.id));
                    }
                }
            }
            if (lineBar) if (const auto bar = waitingLineBar(n, marker.point)) {
                const auto a = mid(*lineBar), b = mid(*bar);
                QPen pen(canvasStyle::conflictPriority(), 1, Qt::DashLine); pen.setCosmetic(true);
                auto* item = scene_.addLine(a.x, a.y, b.x, b.y, pen);
                item->setZValue(markerLevel(n, marker.point.path) * 100. + 8);
                item->setData(0, QStringLiteral("priority-link")); item->setData(1, QString::fromStdString(r.id));
            }
        }
        for (std::size_t k = 0; k < r.markers.size(); ++k) {
            auto point = r.markers[k].point;
            const int level = markerLevel(n, point.path);
            if (!levelVisible(level)) continue;
            const bool dragged = lineDrag_ && lineDrag_->marker == k && lineDrag_->id == r.id && lineDrag_->moved;
            if (dragged) point.station = lineDrag_->station;
            const auto bar = waitingLineBar(n, point);
            if (!bar) continue;
            const bool chosen = tool_ == Tool::stopLine && selectedMarker_ == MarkerRef{r.id, k};
            QPen pen(dragged || chosen ? canvasStyle::active() : canvasStyle::conflictPriority(), dragged ? 3 : 2);
            pen.setCosmetic(true); pen.setCapStyle(Qt::FlatCap);
            auto* item = scene_.addLine(bar->first.x, bar->first.y, bar->second.x, bar->second.y, pen);
            item->setZValue(level * 100. + 7);
            item->setData(0, QStringLiteral("priority-marker")); item->setData(1, QString::fromStdString(r.id));
            item->setData(2, static_cast<int>(k));
        }
    }
}
}
