#pragma once
#include "../model/network/network.hpp"
#include <QGraphicsLineItem>
#include <QColor>

namespace trafficsim {
struct RoadCrossbar { Point first, second; int level{}; };
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
    RoadCrossbarItem(const RoadCrossbar&, QColor, double scale, bool dashed = false, SignalCue cue = SignalCue::none);
    QPainterPath shape() const override;
    QRectF boundingRect() const override;
    void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override;
private:
    double hitWidth_{};
    SignalCue cue_{};
};
}
