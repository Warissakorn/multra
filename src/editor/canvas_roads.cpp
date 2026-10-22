#include "canvas.hpp"
#include "canvas_style.hpp"
#include <QGraphicsPathItem>
#include <QPainterPath>
#include <algorithm>
#include <cmath>

// S3.3b (D307, docs/reference/ROADS.md §16): a Road's marks. Under the Road tool every centreline is a
// thin ink line, so the author sees what is already drawn; the Road the Roads table selects is lit:
// its centreline with a direction arrow and its generated Links washed in the selection colour, all
// in the warning colour while part of it is detached.
namespace multra {
void EditorCanvas::setHighlightedRoad(std::string id, bool detached) {
    if (highlightedRoad_ == id && highlightedRoadDetached_ == detached) return;
    highlightedRoad_ = std::move(id); highlightedRoadDetached_ = detached; redraw();
}
void EditorCanvas::drawRoads() {
    if (!document_) return;
    const auto& n = document_->network;
    const auto line = [](const std::vector<Point>& points) {
        QPainterPath path;
        for (std::size_t i = 0; i < points.size(); ++i) i ? path.lineTo(points[i].x, points[i].y) : path.moveTo(points[i].x, points[i].y);
        return path;
    };
    for (const auto& road : n.roads) {
        const bool lit = road.id == highlightedRoad_;
        if ((!lit && tool_ != Tool::road) || !levelVisible(road.level) || road.centreline.size() < 2) continue;
        const QColor colour = !lit ? canvasStyle::ink() : highlightedRoadDetached_ ? canvasStyle::warning() : canvasStyle::selection();
        QPen pen(colour, lit ? 2 : 1.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin); pen.setCosmetic(true);
        auto* item = scene_.addPath(line(road.centreline), pen);
        item->setZValue(200003); item->setData(0, QStringLiteral("road")); item->setData(1, QString::fromStdString(road.id));
        item->setData(2, lit ? (highlightedRoadDetached_ ? QStringLiteral("detached") : QStringLiteral("lit")) : QStringLiteral("plain"));
        if (!lit) continue;
        // The digitised direction is the forward Link's: one arrowhead at the centreline's middle.
        const double length = polylineLength(road.centreline);
        if (std::isfinite(length) && length > 0) {
            const auto ahead = directionAlong(road.centreline, length / 2, false);
            addArrowhead(pointAlong(road.centreline, length / 2), std::atan2(ahead.y, ahead.x), 6, colour, 200003.1, QStringLiteral("road-direction"));
        }
        QColor wash = colour; wash.setAlpha(60);
        for (const auto& id : road.links) {
            const auto link = std::find_if(n.links.begin(), n.links.end(), [&](const Link& l) { return l.id == id; });
            if (link == n.links.end() || !levelVisible(link->level)) continue;
            auto* surface = scene_.addPath(objectShape(id), QPen(Qt::NoPen), QBrush(wash));
            surface->setZValue(200002); surface->setData(0, QStringLiteral("road-link")); surface->setData(1, QString::fromStdString(id));
        }
    }
}
}
