#pragma once
#include "../model/network/network.hpp"
#include <QGraphicsLineItem>
#include <QColor>
#include <optional>

namespace multra {
// travel: unit direction of the road at the bar; pole: where a signal head's mast stands (D460),
// just outside the road's kerb-side edge, level with the head.
struct RoadCrossbar { Point first, second; int level{}; Point travel{}; std::optional<Point> pole{}; };
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
// D435: a signal head -- its stop line plus a head just upstream. D460 (owner review): the head is
// one square box of one colour with sharp corners, in real metres, hung on a mast arm that meets
// its kerb-side edge from a pole beside the road; dark while editing, the state's colour in a run,
// where the stop line also keeps D199's shape cue (full, thinned middle, open) so it reads without hue.
inline constexpr double headSize=0.90,headGap=0.35,poleRadius=0.22,poleSetback=0.50;
class SignalHeadItem : public RoadCrossbarItem {
public:
    SignalHeadItem(const RoadCrossbar&, QColor line, double scale, bool preview = false, SignalCue cue = SignalCue::none);
    QPainterPath shape() const override;
    QRectF boundingRect() const override;
    void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override;
private:
    struct HeadFrame { QPointF centre, across, back; double size{}; };
    std::optional<HeadFrame> frame() const; // the housing's placement; none for a degenerate bar
    QPainterPath housing() const; // the head's square: also the hit region beside the stop line
    QPainterPath square(QPointF centre,double side) const; // a square aligned with the bar
    std::optional<QPointF> poleAt() const; // the pole's centre, when the bar knows its kerb
    Point travel_{}; double scale_{}; SignalCue lamp_{}; std::optional<Point> pole_; // lamp_: the run state
};
}
