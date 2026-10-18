#include "canvas.hpp"
#include "canvas_style.hpp"
#include "../model/network/right_of_way.hpp"
#include <QGraphicsLineItem>
#include <QGraphicsPathItem>
#include <QPainterPath>
#include <algorithm>
#include <QMouseEvent>
#include <QPen>

// M5.4b (D133): the Section tool and the sections' marks. Ctrl+right-click on a Link sets the
// start line, a second one the end line and hands both over; Esc drops a half-placed section. A
// line is drawn across every lane of its Link, where evaluation measures it.
namespace trafficsim {
std::optional<SectionLine> EditorCanvas::sectionLineAt(Point p) const {
    if (!document_) return std::nullopt;
    // The same pick a queue counter's explicit point uses, widened from its lane to the Link.
    const auto placed = headAt(p);
    if (!placed || !placed->slot.connectorId.empty()) return std::nullopt;
    const auto point = laneControlPoint(document_->network, placed->slot.lane, placed->station);
    if (!point) return std::nullopt;
    return SectionLine{point->path.linkId, point->station};
}
bool EditorCanvas::sectionPress(QMouseEvent* e) {
    if (tool_ != Tool::section || !document_ || e->button() != Qt::RightButton || !(e->modifiers() & Qt::ControlModifier))
        return false; // D84: Ctrl+right-click authors; a left click only selects
    const auto p = world(e->pos(), false);
    lastPick_ = p;
    const auto line = sectionLineAt(p);
    if (!line) { clearSelection(false); return true; }
    if (!sectionStart_) { sectionStart_ = *line; redraw(); return true; }
    const auto start = *sectionStart_;
    sectionStart_.reset();
    redraw();
    if (sectionCommitted) sectionCommitted(start, *line);
    return true;
}
void EditorCanvas::drawSections() {
    if (!document_) return;
    const auto& n = document_->network;
    const auto mark = [&](const SectionLine& line, const std::string& id, const char* end, bool draft) {
        for (const auto& link : n.links) {
            if (link.id != line.linkId || !levelVisible(link.level)) continue;
            for (const auto& lane : link.lanes) {
                const auto bar = waitingLineBar(n, {{link.id, lane.id, {}, {}, {}}, line.station});
                if (!bar) continue; // past this lane's end: evaluation skips it too
                QPen pen(canvasStyle::advisory(), draft ? 4 : 3, draft ? Qt::DashLine : Qt::SolidLine);
                pen.setCosmetic(true); pen.setCapStyle(Qt::FlatCap);
                auto* item = scene_.addLine(bar->first.x, bar->first.y, bar->second.x, bar->second.y, pen);
                item->setZValue(link.level * 100. + 11.6);
                item->setData(0, QStringLiteral("travel-time-section")); item->setData(1, QString::fromStdString(id));
                item->setData(2, QString::fromLatin1(end));
            }
        }
    };
    for (const auto& s : n.travelTimeSections) { mark(s.start, s.id, "start", false); mark(s.end, s.id, "end", false); }
    // B-6 (D223): each desired-speed decision across every lane of its Link, dotted to tell it apart.
    if (document_->definition)
        for (const auto& d : document_->definition->speedDecisions)
            for (const auto& link : n.links) {
                if (link.id != d.linkId || !levelVisible(link.level)) continue;
                for (const auto& lane : link.lanes) {
                    const auto bar = waitingLineBar(n, {{link.id, lane.id, {}, {}, {}}, d.station});
                    if (!bar) continue;
                    QPen pen(canvasStyle::advisory(), 3, Qt::DotLine); pen.setCosmetic(true); pen.setCapStyle(Qt::FlatCap);
                    auto* item = scene_.addLine(bar->first.x, bar->first.y, bar->second.x, bar->second.y, pen);
                    item->setZValue(link.level * 100. + 11.6);
                    item->setData(0, QStringLiteral("speed-decision")); item->setData(1, QString::fromStdString(d.id));
                }
            }
    // B-6 §7 (D225): each reduced-speed area shaded along each lane it covers, over its stretch.
    if (document_->definition)
        for (const auto& a : document_->definition->reducedSpeedAreas)
            for (const auto& link : n.links) {
                if (link.id != a.linkId || !levelVisible(link.level)) continue;
                for (const auto& lane : link.lanes) {
                    if (!a.laneIds.empty() && std::find(a.laneIds.begin(), a.laneIds.end(), lane.id) == a.laneIds.end()) continue;
                    const auto centre = laneGeometry(link, lane.id, n.drivingSide);
                    const auto span = polylineSpan(centre, matchedStation(link.geometry, centre, a.from), matchedStation(link.geometry, centre, a.to));
                    if (span.size() < 2) continue;
                    QPainterPath path(QPointF(span.front().x, span.front().y));
                    for (std::size_t i = 1; i < span.size(); ++i) path.lineTo(span[i].x, span[i].y);
                    auto colour = canvasStyle::advisory(); colour.setAlpha(70);
                    QPen pen(colour, lane.width * 0.8); pen.setCapStyle(Qt::FlatCap); pen.setJoinStyle(Qt::RoundJoin);
                    auto* item = scene_.addPath(path, pen);
                    item->setZValue(link.level * 100. + 11.5);
                    item->setData(0, QStringLiteral("speed-area")); item->setData(1, QString::fromStdString(a.id));
                }
            }
    if (tool_ == Tool::section && sectionStart_) mark(*sectionStart_, {}, "start", true);
}
}
