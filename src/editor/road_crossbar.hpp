#pragma once
#include "../model/network/network.hpp"
#include <QGraphicsLineItem>
#include <QColor>

namespace multra {
struct RoadCrossbar { Point first, second; int level{}; Point travel{}; }; // travel: unit direction of the road at the bar
// Presentation only: intersect the road normal with its real rails at this station.
std::optional<RoadCrossbar> roadCrossbar(const std::vector<Point>& centre,
    const std::vector<Point>& firstRail, const std::vector<Point>& secondRail,
    double station, int level,
    const std::vector<Point>& firstContinuation = {}, const std::vector<Point>& secondContinuation = {});
std::optional<RoadCrossbar> objectCrossbar(const Network&, const std::string& id, bool end, std::optional<double> station = {});
std::optional<RoadCrossbar> signalCrossbar(const Network&, const NetworkSignalHead&);
// A run signal state's shape (D199), so it reads without hue: stop is the full bar, caution thins
// its middle third, go opens it. none is the plain stroke every edit-mode bar uses.
enum class SignalCue { none, stop, caution, go };
// The hit region is wider than the visible cosmetic stroke, at every zoom; a cue changes only paint.
class RoadCrossbarItem : public QGraphicsLineItem {
public:
    // `preview` marks a bar not yet placed (a head under the pointer, a route's moving end): solid,
    // on a translucent Base casing (D306), never dashed.
    RoadCrossbarItem(const RoadCrossbar&, QColor, double scale, bool preview = false, SignalCue cue = SignalCue::none);
    QPainterPath shape() const override;
    QRectF boundingRect() const override;
    void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override;
private:
    double hitWidth_{};
    SignalCue cue_{};
    bool preview_{};
};
// D435: a signal head as Synchro draws one -- its stop line plus a dark three-lamp head just upstream,
// lamps across the lane in red, amber, green order. Edit mode shows every lamp dim; a run lights the
// state's lamp only, so the state reads by position as well as hue (D199). Constant size on screen.
class SignalHeadItem : public RoadCrossbarItem {
public:
    SignalHeadItem(const RoadCrossbar&, QColor line, double scale, bool preview = false, SignalCue cue = SignalCue::none);
    QPainterPath shape() const override;
    QRectF boundingRect() const override;
    void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override;
private:
    struct HeadFrame { QPointF centre, across, back; double length{}, depth{}; };
    std::optional<HeadFrame> frame() const; // the housing's placement; none for a degenerate bar
    QPainterPath housing() const;
    Point travel_{}; double scale_{}; SignalCue lamp_{};
};
}
