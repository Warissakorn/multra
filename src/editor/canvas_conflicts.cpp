#include "canvas.hpp"
#include "canvas_style.hpp"
#include "../model/network/right_of_way.hpp"
#include "../model/network/conflict_display.hpp"
#include "../model/network/conflict_surface.hpp"
#include <QApplication>
#include <QGraphicsPathItem>
#include <QGraphicsPolygonItem>
#include <QMouseEvent>
#include <QPen>
#include <algorithm>
#include <cmath>

// M3.2.4: authored conflict areas and waiting lines on the canvas, drawn from the same lane
// painted strips. Runtime coverage uses those same rails; grouping shares controls, not outlines.
namespace multra {
namespace {
QPainterPath outlinePath(const std::vector<Point>& points) {
    QPainterPath shape(QPointF(points.front().x, points.front().y));
    for (std::size_t i = 1; i < points.size(); ++i) shape.lineTo(points[i].x, points[i].y);
    shape.closeSubpath();
    return shape;
}
// Offset laterally inside the painted rails, retaining the measured entry/exit cuts.
// Cap the inset at 20% of local width so narrow/tapered lanes and short mouths remain visible.
QPainterPath bandPath(const Network& n,const std::vector<ConflictSide>& spans,const std::vector<ConflictMouthBand>& mouths) {
    QPainterPath shape;
    const auto add=[&](const ConflictSide& side) {
        const auto outline=conflictBandOutline(n,side),full=conflictSideOutline(n,side);
        if(outline.size()>=3 && full.size()>=3)shape=shape.united(outlinePath(outline).intersected(outlinePath(full)));
    };
    for(const auto& side:spans)add(side);
    // The physical cap determines the Link span, while the directional band follows its lane.
    for(const auto& mouth:mouths)add(mouth.lane);
    return shape;
}
QPainterPath displayPath(const Network& n,const ConflictAreaGeometry& geometry) {
    return bandPath(n,geometry.first,geometry.firstMouth).united(bandPath(n,geometry.second,geometry.secondMouth));
}
int levelOf(const Network& n, const ControlPathRef& ref) {
    for (const auto& l : n.links) if (l.id == ref.linkId) return l.level;
    for (const auto& c : n.connectors) if (c.id == ref.connectorId) return c.level;
    return 0;
}
double segmentDistance(Point p, Point a, Point b) {
    const double dx = b.x - a.x, dy = b.y - a.y, n = dx * dx + dy * dy;
    const double t = n <= 0 ? 0 : std::clamp(((p.x - a.x) * dx + (p.y - a.y) * dy) / n, 0.0, 1.0);
    return std::hypot(p.x - a.x - t * dx, p.y - a.y - t * dy);
}
}
void EditorCanvas::setHighlightedConflict(std::string id) {
    if (highlightedConflict_ == id) return;
    highlightedConflict_ = std::move(id); redraw();
}
std::vector<std::string> EditorCanvas::conflictsAt(Point p) const {
    std::vector<std::string> found;
    if (!document_) return found;
    const auto& n = document_->network;
    for (const auto& area : n.rightOfWay.conflictAreas) {
        if(!levelVisible(levelOf(n,area.first.path)) || !levelVisible(levelOf(n,area.second.path)))continue;
        const auto shape=displayPath(n,conflictAreaGeometry(n,area.kind,area.first,area.second));
        if(shape.contains(QPointF(p.x,p.y))) {
            std::string key=area.id;
            for(const auto& g:conflictGroups_)if(conflictGroupContains(g,area.id)){key=g.key;break;}
            found.push_back(key);
        }
    }
    std::sort(found.begin(), found.end());
    found.erase(std::unique(found.begin(),found.end()),found.end());
    return found;
}
std::string EditorCanvas::waitingLineAt(Point p) const {
    if (!document_) return {};
    const auto& n = document_->network;
    // Four pixels either side at any zoom, as a stop line is hit (headShape).
    double best = 4 / viewScale();
    std::string found;
    for (const auto& line : n.rightOfWay.waitingLines) {
        if (!levelVisible(levelOf(n, line.point.path))) continue;
        const auto bar = waitingLineBar(n, line.point);
        if (!bar) continue;
        const double d = segmentDistance(p, bar->first, bar->second);
        if (d <= best) { best = d; found = line.id; }
    }
    return found;
}
void EditorCanvas::selectStopLine(std::string id) {
    if (selectedStopLine_ == id) return;
    selectedStopLine_ = std::move(id); redraw();
}
// M3.2.12 slice 3 (STOP_LINES §5). D84: Ctrl+right-click on a lane places a stop line there; a left
// click picks a placed line and starts its drag along its own path.
bool EditorCanvas::stopLinePress(QMouseEvent* e) {
    if (tool_ != Tool::stopLine || !document_) return false;
    const bool place = e->button() == Qt::RightButton && (e->modifiers() & Qt::ControlModifier);
    if (!place && e->button() != Qt::LeftButton) return false;
    const auto p = world(e->pos(), false);
    lastPick_ = p;
    if (const auto m = place ? std::nullopt : priorityMarkerAt(p)) { // D616: a marker is the smaller target
        selectedStopLine_.clear(); selectedMarker_ = m;
        for (const auto& r : document_->network.rightOfWay.priorityRules) if (r.id == m->first) {
            const auto& point = r.markers[m->second].point;
            auto polyline = controlPathPolyline(document_->network, point.path);
            if (polyline.size() >= 2) { lineDrag_ = LineDrag{r.id, std::move(polyline), point.station, point.station, false, {}, m->second}; dragPress_ = e->pos(); }
        }
        redraw(); return true;
    }
    if (!place) selectedMarker_.reset();
    if (const auto id = waitingLineAt(p); !place && !id.empty()) {
        selectStopLine(id);
        if (stopLinePicked) stopLinePicked(id);
        for (const auto& line : document_->network.rightOfWay.waitingLines) if (line.id == id) {
            auto polyline = controlPathPolyline(document_->network, line.point.path);
            if (polyline.size() < 2) break;
            lineDrag_ = LineDrag{id, std::move(polyline), line.point.station, line.point.station, false};
            dragPress_ = e->pos();
        }
        return true;
    }
    if (!place) { selectStopLine({}); return true; }
    const auto at = headAt(p);
    const auto point = at ? slotControlPoint(document_->network, *at) : std::nullopt;
    if (!point) { if (creationRejected) creationRejected(); return true; }
    // D616: with a line picked, another lane gets a marker of its rule; its own lane a new line.
    const auto& lines = document_->network.rightOfWay.waitingLines;
    const auto picked = std::find_if(lines.begin(), lines.end(), [&](const auto& w) { return w.id == selectedStopLine_; });
    if (picked != lines.end() && picked->point.path != point->path) { if (priorityMarkerAdded) priorityMarkerAdded(picked->id, *point); return true; }
    if (stopLinePlaced) stopLinePlaced(*point);
    return true;
}
bool EditorCanvas::conflictPress(QMouseEvent* e) {
    if (tool_ != Tool::conflict || !document_) return false;
    const bool change = e->button() == Qt::RightButton && (e->modifiers() & Qt::ControlModifier);
    if (!change && e->button() != Qt::LeftButton) return false;
    const auto p = world(e->pos(), false);
    lastPick_ = p;
    // D84: a left click selects an area and drags a waiting line; Ctrl+right-click authors a
    // passive area or sets the next priority.
    // A line wins over an area: it is the smaller target, and it always stands just outside one.
    if (const auto id = waitingLineAt(p); !change && !id.empty()) {
        for (const auto& line : document_->network.rightOfWay.waitingLines) if (line.id == id) {
            auto polyline = controlPathPolyline(document_->network, line.point.path);
            if (polyline.size() < 2) break;
            lineDrag_ = LineDrag{id, std::move(polyline), line.point.station, line.point.station, false};
            dragPress_ = e->pos();
            return true;
        }
    }
    const auto areas = conflictsAt(p);
    if (areas.empty()) {
        // No authored area here: an automatic one is authored by the click (D68), as Vissim's
        // first click sets a passive area's priority.
        if(const auto key=automaticAt(p);!key.empty()) {
            if(conflictPicked)conflictPicked(key);
            const auto group=std::find_if(conflictGroups_.begin(),conflictGroups_.end(),[&](const auto& g){return g.key==key;});
            const bool branching=group!=conflictGroups_.end() && group->geometryKind==ConflictGeometryKind::branching;
            if(change && !branching && conflictAuthored)conflictAuthored(key);
            return true;
        }
        clearSelection(false); return true;
    }
    const bool current = std::find(areas.begin(), areas.end(), highlightedConflict_) != areas.end();
    const auto area = current ? highlightedConflict_ : areas.front();
    if (!current && conflictPicked) conflictPicked(area);
    if (change && conflictCycled) conflictCycled(area);
    return true;
}
void EditorCanvas::updateLineDrag(QPoint position) {
    if (!lineDrag_) return;
    if ((position - dragPress_).manhattanLength() < QApplication::startDragDistance() && !lineDrag_->moved) return;
    lineDrag_->moved = true; showMoveCursor();
    // Along its own path only, as a stop line drags: another lane is another line.
    const double length = polylineLength(lineDrag_->polyline);
    lineDrag_->station = std::clamp(stationOfClosestPoint(lineDrag_->polyline, world(position, false)), 0.0, length);
    redraw();
}
void EditorCanvas::finishLineDrag(QPoint position) {
    if (!lineDrag_) return;
    updateLineDrag(position); // the release is authoritative: the last move may be coalesced
    const auto drag = *lineDrag_; lineDrag_.reset();
    redraw();
    if (!drag.moved || std::abs(drag.station - drag.original) <= 1e-9) return;
    if (drag.marker) { if (priorityMarkerMoved) priorityMarkerMoved(drag.id, *drag.marker, drag.station); } // D616
    else if (waitingLineMoved) waitingLineMoved(drag.id, drag.station);
}
void EditorCanvas::setAutomaticConflicts(std::vector<AutomaticConflict> automatic) {
    if (automatic_ == automatic) return;
    automatic_ = std::move(automatic);
    if (tool_ == Tool::conflict) redraw();
}
std::string EditorCanvas::automaticAt(Point p) const {
    if (!document_ || tool_ != Tool::conflict) return {};
    const auto& n = document_->network;
    for(const auto& a:automatic_) {
        if(!levelVisible(levelOf(n,a.first.path)) || !levelVisible(levelOf(n,a.second.path)))continue;
        if(displayPath(n,conflictAreaGeometry(n,a.kind,a.first,a.second)).contains(QPointF(p.x,p.y))) {
            for(const auto& g:conflictGroups_)if(conflictGroupContains(g,a.key))return g.key;
            return a.key;
        }
    }
    return {};
}
void EditorCanvas::updateConflictRelated() {
    // D611 (owner): selecting a conflict area shows the Links and Connectors it runs on as hovered.
    conflictRelated_.clear();
    if (!document_ || tool_ != Tool::conflict || highlightedConflict_.empty()) return;
    const auto& n = document_->network;
    const auto groups = conflictGroups(n, automatic_);
    const auto group = std::find_if(groups.begin(), groups.end(), [&](const auto& g) { return g.key == highlightedConflict_; });
    const auto member = [&](const std::string& id) { return id == highlightedConflict_ || (group != groups.end() && conflictGroupContains(*group, id)); };
    for (const auto& a : n.rightOfWay.conflictAreas) if (member(a.id))
        for (const auto* side : {&a.first, &a.second}) conflictRelated_.insert(conflictOwner(side->path));
    for (const auto& a : automatic_) if (member(a.key))
        for (const auto* side : {&a.first, &a.second}) conflictRelated_.insert(conflictOwner(side->path));
}
void EditorCanvas::drawConflicts() {
    if (!document_) return;
    const auto& n = document_->network;
    conflictGroups_=conflictGroups(n,automatic_);
    // Separate bands follow each participant's driving direction; grouping uses measured polygons.
    const auto draw=[&](const std::string& id,ConflictKind kind,ConflictPriority priority,const ConflictSide& first,const ConflictSide& second,bool automatic,ConflictGeometryKind geometryKind) {
        std::string key=id;
        for(const auto& g:conflictGroups_)if(conflictGroupContains(g,id)){key=g.key;break;}
        if(!levelVisible(levelOf(n,first.path)) || !levelVisible(levelOf(n,second.path)))return;
        const auto geometry=conflictAreaGeometry(n,kind,first,second);
        const bool branching=geometryKind==ConflictGeometryKind::branching;
        for(const auto* side:{&first,&second}) {
            const auto shape=bandPath(n,side==&first?geometry.first:geometry.second,side==&first?geometry.firstMouth:geometry.secondMouth);if(shape.isEmpty())continue;
            const int level=levelOf(n,side->path);
            const bool undecided=priority==ConflictPriority::undetermined;
            const bool yields=(side==&first)==(priority==ConflictPriority::firstYields);
            const bool lit=key==highlightedConflict_,hover=key==hoverConflict_ || key==hoverAutomatic_;
            // D610 (owner): right of way green, yields red, undecided olive; a refused branching stays the dark error ink.
            QColor tint=branching?canvasStyle::error():undecided?(automatic?canvasStyle::ink():canvasStyle::conflictPassive())
                :yields?canvasStyle::conflictYield():canvasStyle::conflictPriority();
            // D435 as amended by D592 (owner, 2026-11-19): a flat fill with no hatching or outline at
            // rest, fully opaque so the area reads on any road; the waiting lines stay above it (z 7).
            // Only an automatic candidate nobody has decided stays a faint wash. Hover and the
            // highlighted conflict get a clean solid edge.
            tint.setAlpha(automatic && undecided?40:255);
            const bool hatched=!branching && !undecided && yields; // tag only: z order, not paint
            QPen pen(Qt::NoPen);
            if(lit || hover) {
                pen=QPen(lit?canvasStyle::selection():canvasStyle::hover(),lit?2:1.5,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin);
                pen.setCosmetic(true);
            }
            auto* item=scene_.addPath(shape,pen,QBrush(tint));
            item->setZValue(level*100.+(automatic?(hatched?5.6:5.5):hatched?6.5:6));
            item->setData(0,QStringLiteral("conflict-area"));
            if(automatic && key.rfind("auto/",0)==0)item->setData(0,QStringLiteral("auto-conflict"));
            item->setData(1,QString::fromStdString(key));
            if(automatic)item->setData(2,QString::fromLatin1(branching?"branching":undecided?"passive":"merge"));
            item->setData(4,QString::fromLatin1(branching?"branching":geometryKind==ConflictGeometryKind::merge?"merge":"crossing"));
            item->setData(5,QString::fromStdString(conflictOwner(side->path))); // directional participant
            item->setData(3,QString::fromStdString(id)); // lane-pair identity, independent of group selection
            // D610 (b), owner: each conflicting path carries a dark centre line, arrowhead at its exit end.
            if(automatic && undecided)continue; // a faint candidate stays faint
            QColor mark=tint.darker(170);mark.setAlpha(255);
            QPen centre(mark,1.5);centre.setCosmetic(true);centre.setCapStyle(Qt::FlatCap);
            const auto spans=side==&first?geometry.first:geometry.second;
            const auto& mouths=side==&first?geometry.firstMouth:geometry.secondMouth;
            std::vector<ConflictSide> lanes(spans.begin(),spans.end());for(const auto& m:mouths)lanes.push_back(m.lane);
            for(const auto& span:lanes) {
                std::vector<Point> line;
                try{line=polylineSpan(controlPathPolyline(n,span.path),std::min(span.entryStation,span.exitStation),std::max(span.entryStation,span.exitStation));}
                catch(const std::exception&){continue;}
                const double length=line.size()<2?0:polylineLength(line);
                if(!(length>=.5) || !std::isfinite(length))continue;
                const double z=item->zValue()+.05;
                QPainterPath stroke(QPointF(line.front().x,line.front().y));for(std::size_t i=1;i<line.size();++i)stroke.lineTo(line[i].x,line[i].y);
                auto* mid=scene_.addPath(stroke,centre);mid->setZValue(z);
                const double inset=std::min(length/2,6/viewScale());
                const auto ahead=directionAlong(line,length-inset,false);
                auto* head=addArrowhead(pointAlong(line,length-inset),std::atan2(ahead.y,ahead.x),6,mark,z+.01,QStringLiteral("conflict-arrow"));
                for(auto* part:{static_cast<QGraphicsItem*>(mid),static_cast<QGraphicsItem*>(head)}) {
                    part->setData(0,QStringLiteral("conflict-arrow"));part->setData(1,QString::fromStdString(key));part->setData(3,QString::fromStdString(id));
                }
            }
        }
    };
    // D590 (owner): conflict areas show only under the Conflict area tool. D603 (owner, amends D590):
    // waiting lines, plain or carrying Stop or Yield, show only under the Conflict area, Stop line and
    // Queue counter tools (the counter snaps to them).
    // D594 slice 3: the Stop line tool shows the areas its lines cover, and every placed line.
    const bool stopTool=tool_==Tool::stopLine,conflictTool=tool_==Tool::conflict,lineTool=conflictTool||stopTool||tool_==Tool::counter;
    if(conflictTool||stopTool)for(const auto& a:n.rightOfWay.conflictAreas)draw(a.id,a.kind,a.priority,a.first,a.second,false,
        a.kind==ConflictKind::crossing?ConflictGeometryKind::crossing:ConflictGeometryKind::merge);
    if(conflictTool)for(const auto& a:automatic_)draw(a.key,a.kind,a.priority,a.first,a.second,true,a.geometryKind);
    // SL7 (STOP_LINES §5): under the Conflict area tool a hovered or selected area shows where its
    // sides without a line wait -- faint, dashed, ink -- and nothing else does.
    if(conflictTool)for(const auto& a:n.rightOfWay.conflictAreas) {
        std::string key=a.id;
        for(const auto& g:conflictGroups_)if(conflictGroupContains(g,a.id)){key=g.key;break;}
        if(key!=highlightedConflict_ && key!=hoverConflict_)continue;
        for(const auto* side:{&a.first,&a.second}) {
            if(!side->waitingLineId.empty() || !levelVisible(levelOf(n,side->path)))continue;
            std::optional<std::pair<Point,Point>> bar;
            try{bar=waitingLineBar(n,{side->path,derivedWaitingStation(n,*side)});}catch(const std::exception&){}
            if(!bar)continue;
            QColor ink=canvasStyle::ink();ink.setAlpha(150);
            QPen pen(ink,1.5,Qt::DashLine);pen.setCosmetic(true);pen.setCapStyle(Qt::FlatCap);
            auto* item=scene_.addLine(bar->first.x,bar->first.y,bar->second.x,bar->second.y,pen);
            item->setZValue(levelOf(n,side->path)*100.+7);
            item->setData(0,QStringLiteral("derived-line"));item->setData(1,QString::fromStdString(a.id));
        }
    }
    for (const auto& line : n.rightOfWay.waitingLines) {
        const int level = levelOf(n, line.point.path);
        if (!levelVisible(level)) continue;
        auto point = line.point;
        const bool dragged = lineDrag_ && lineDrag_->id == line.id && lineDrag_->moved;
        if (dragged) point.station = lineDrag_->station;
        const auto bar = waitingLineBar(n, point);
        if (!bar) continue;
        // M3.2.5b: the line's control shows in the bar -- dashed for none, solid amber for Yield,
        // solid red and heavier for Stop. No text: the canvas has no locale.
        const auto control = std::find_if(n.rightOfWay.stopControls.begin(), n.rightOfWay.stopControls.end(),
                                          [&](const auto& c) { return c.waitingLineId == line.id; });
        const bool controlled = control != n.rightOfWay.stopControls.end();
        if (!lineTool) continue; // D603
        const bool stop = controlled && control->mode == StopMode::stop;
        const bool picked = stopTool && line.id == selectedStopLine_;
        QPen pen(dragged || picked ? canvasStyle::active() : line.id==hoverWaitingLine_ ? canvasStyle::hover() : stop ? canvasStyle::error() : canvasStyle::warning(), dragged || stop ? 3 : 2,
                 controlled ? Qt::SolidLine : Qt::DashLine);
        pen.setCosmetic(true); pen.setCapStyle(Qt::FlatCap);
        auto* item = scene_.addLine(bar->first.x, bar->first.y, bar->second.x, bar->second.y, pen);
        item->setZValue(level * 100. + 7);
        item->setData(0, QStringLiteral("waiting-line")); item->setData(1, QString::fromStdString(line.id));
        item->setData(2, QString::fromLatin1(!controlled ? "" : stop ? "stop" : "yield"));
    }
    if(conflictTool||stopTool)drawPriorityMarkers(); // D616
}
}
