#pragma once
// The bar's geometry only, so canvas.hpp needs no Qt item or signal-head drawing constant: editing
// the head's look (9 of 11 road_crossbar.hpp edits from 2026-10-28) then rebuilds its few users,
// not every includer of canvas.hpp. Items and constants stay in road_crossbar.hpp.
#include "../model/network/network.hpp"
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
}
