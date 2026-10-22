#include "canvas.hpp"
#include "canvas_style.hpp"
#include <QGraphicsPathItem>
#include <QGraphicsPolygonItem>
#include <QEvent>
#include <QPainter>
#include <QPainterPath>
#include <QTransform>
#include <cmath>

namespace multra {
bool EditorCanvas::movingBody() const {
    return (dragging_ && vertex_<0 && preview_!=original_) || groupDragging_ || copyDragging_;
}
bool EditorCanvas::movingGeometry() const {
    return movingBody() || (dragging_ && preview_!=original_) || (endpointDrag_ && endpointDraft_);
}
void EditorCanvas::clearSelectionState() {
    selection_.clear(); vertex_ = -1;
    clearHighlights();
}
void EditorCanvas::clearHighlights() {
    highlightedRoute_.clear(); highlightedConflict_.clear(); highlightedNode_.clear();
    if (selectionCleared) selectionCleared();
}
void EditorCanvas::clearSelection(bool cancelGesture) {
    if (cancelGesture) resetGesture();
    clearSelectionState();
    notifySelection();
}
void EditorCanvas::clearHover() {
    hoverObject_.clear(); hoverConflict_.clear(); hoverAutomatic_.clear(); hoverWaitingLine_.clear();
    hoverVertex_ = -1; hoverLaneKind_ = 0; hoverLaneLocation_ = -1;
    hoverSegment_.clear(); hoverHead_.reset(); connectorHover_.reset();
    if(!routeTracing_)routePreview_=routeDraft_;
    setCursor(idleCursor());
}
void EditorCanvas::updateHover(QPoint position) {
    std::string object, conflict, automatic, waiting;
    int vertex = -1, laneKind = 0, laneLocation = -1;
    auto cursor = idleCursor();
    const auto p = world(position, false);
    if (tool_ == Tool::select) {
        if (const auto h = laneHandleAt(position)) {
            laneKind = h->kind; laneLocation = h->location;
            const double x = std::abs(h->direction.x), y = std::abs(h->direction.y);
            cursor = QCursor(x > 2 * y ? Qt::SizeHorCursor : y > 2 * x ? Qt::SizeVerCursor :
                     h->direction.x * h->direction.y > 0 ? Qt::SizeBDiagCursor : Qt::SizeFDiagCursor);
        } else if ((vertex = vertexAt(position)) >= 0) cursor = QCursor(Qt::SizeAllCursor);
        else object = hit(p).first; // pointing stays an arrow; a move shows the hand (showMoveCursor)
    } else if (tool_ == Tool::split || tool_ == Tool::connect) {
        object = hit(p, false).first;
    } else if (tool_ == Tool::conflict) {
        waiting = waitingLineAt(p);
        if (waiting.empty()) {
            const auto areas = conflictsAt(p);
            if (!areas.empty()) conflict = areas.front();
            else automatic = automaticAt(p);
        }
        if (!waiting.empty()) cursor = QCursor(Qt::SizeAllCursor);
    } else if (tool_ == Tool::counter && counterLineAt(p)) object = hit(p).first;
    else if (tool_ == Tool::section && sectionLineAt(p)) object = hit(p).first;
    const bool changed = object != hoverObject_ || conflict != hoverConflict_ || automatic != hoverAutomatic_ ||
        waiting != hoverWaitingLine_ || vertex != hoverVertex_ || laneKind != hoverLaneKind_ || laneLocation != hoverLaneLocation_;
    hoverObject_ = std::move(object); hoverConflict_ = std::move(conflict);
    hoverAutomatic_ = std::move(automatic); hoverWaitingLine_ = std::move(waiting);
    hoverVertex_ = vertex; hoverLaneKind_ = laneKind; hoverLaneLocation_ = laneLocation;
    setCursor(cursor);
    if (changed) redraw();
}
void EditorCanvas::leaveEvent(QEvent* event) {
    clearHover(); redraw();
    QGraphicsView::leaveEvent(event);
}
void EditorCanvas::drawObjectFeedback(const std::string& id, const QPainterPath& shape, double z,
                                      const std::vector<std::vector<Point>>& edges) {
    const bool chosen = isSelected(id), hovered = id == hoverObject_;
    if ((!chosen && !hovered) || shape.isEmpty()) return;
    const bool primary = id == selected();
    const QColor colour = chosen ? canvasStyle::selection() : canvasStyle::hover();
    QPen pen(colour, chosen && primary ? 2 : 1.5); pen.setCosmetic(true); pen.setJoinStyle(Qt::RoundJoin);
    QColor fill = colour; fill.setAlpha(chosen ? 32 : 16);
    auto* item = scene_.addPath(shape, pen, QBrush(fill)); item->setZValue(z);
    item->setData(0, QStringLiteral("object-feedback")); item->setData(1, QString::fromStdString(id));
    item->setData(2, chosen ? QStringLiteral("selected") : QStringLiteral("hover"));
    if (!chosen) return;
    // The direction of travel rides the selection outline's long edges: a solid line with an
    // arrowhead every so many screen pixels, and one at the middle of an edge too short for two.
    const double scale = viewScale(), spacing = 72 / scale;
    for (const auto& edge : edges) {
        const double length = polylineLength(edge);
        if (!(length > 0) || !std::isfinite(length)) continue;
        const int count = std::max(1, static_cast<int>(length / spacing));
        const double step = length / count;
        for (int i = 0; i < count; ++i) {
            const double station = step * (i + .5);
            const auto at = pointAlong(edge, station);
            const auto ahead = directionAlong(edge, station, false);
            addArrowhead(at, std::atan2(ahead.y, ahead.x), 5, colour, z + .01, QStringLiteral("direction-arrow"));
        }
    }
}
QPainterPath EditorCanvas::centreStroke(const std::vector<Point>& line, double pixels) const {
    if (line.size() < 2) return {};
    QPainterPath centre; centre.moveTo(line.front().x, line.front().y);
    for (std::size_t i = 1; i < line.size(); ++i) centre.lineTo(line[i].x, line[i].y);
    // A region, not an open path: QPainterPath::contains would close an open one.
    QPainterPathStroker stroke; stroke.setWidth(pixels / viewScale());
    stroke.setCapStyle(Qt::FlatCap); stroke.setJoinStyle(Qt::RoundJoin);
    return stroke.createStroke(centre);
}
void EditorCanvas::drawCentreLine(const std::string& id, const std::vector<Point>& line, const QColor& colour, double z) {
    if (line.size() < 2) return;
    QPainterPath centre; centre.moveTo(line.front().x, line.front().y);
    for (std::size_t i = 1; i < line.size(); ++i) centre.lineTo(line[i].x, line[i].y);
    QPen pen(colour, 2); pen.setCosmetic(true); pen.setCapStyle(Qt::FlatCap); pen.setJoinStyle(Qt::RoundJoin);
    auto* item = scene_.addPath(centre, pen); item->setZValue(z);
    item->setData(0, QStringLiteral("centre-line")); item->setData(1, QString::fromStdString(id));
    // The highlight is a band a few pixels either side, so a selected line still reads as one.
    drawObjectFeedback(id, centreStroke(line, 8), z + .5, {line});
}
void EditorCanvas::addArrowhead(Point at, double angle, double pixels, QColor colour, double z, const QString& tag) {
    const double r = pixels / viewScale();
    QPolygonF arrow;
    for (double offset : {0.0, 2.5, -2.5}) arrow << QPointF(at.x + r*std::cos(angle+offset), at.y + r*std::sin(angle+offset));
    auto* item = scene_.addPolygon(arrow, QPen(Qt::NoPen), QBrush(colour));
    item->setZValue(z); item->setData(0, tag);
}
void EditorCanvas::drawGeometryHandles(const std::string& id, const std::vector<Point>& points, bool connector) {
    if (tool_ != Tool::select || selection_.size() != 1 || id != selected() || movingBody() || rotationPivot_) return;
    const double r = 4 / viewScale();
    for (std::size_t i = 0; i < points.size(); ++i) {
        const bool end = connector && (i == 0 || i + 1 == points.size());
        const bool held = end ? endpointDrag_ && *endpointDrag_ == (i == 0) : dragging_ && vertex_ == static_cast<int>(i);
        const bool hovered = hoverVertex_ == static_cast<int>(i);
        const QColor colour = held ? canvasStyle::active() : hovered ? canvasStyle::hover() : canvasStyle::selection();
        auto* item = scene_.addEllipse(points[i].x - r, points[i].y - r, 2*r, 2*r,
                                      QPen(Qt::NoPen), QBrush(colour));
        item->setZValue(200012);
        item->setData(0, end ? QStringLiteral("connector-end") : QStringLiteral("geometry-point"));
        item->setData(1, QString::fromStdString(id));
        item->setData(2, end ? QVariant(i == 0) : QVariant(static_cast<int>(i)));
    }
}
}
