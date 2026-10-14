// D166 (docs/reference/BEHAVIOUR_LAB.md §2): the Urban circuit. Left-hand traffic; every one-way
// street's reference line lies h = lanes x width / 2 to the left of its centreline, so two loops on
// one street read as a two-way road. No loop shares a segment with another (no merge, no rule), so
// each intersection is two loops crossing under one split-phase controller.
//
//            C2 (north leg)
//   C3 -> --X3======= north street (main <-, C3 ->) ======X2
//           | main turns left                            | main turns left; C2 runs south
//   west    |                                            | beside it, then turns away east
//   street  |                                            |
//           `== SW curve == X1a ==== X1b == SE curve ==='  south street (main ->)
//                            C1 crosses the south street at X1a (north) and X1b (south)
#include "lab_build.hpp"
#include "../commands/demand_commands.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace trafficsim::lab::build {
LabNetwork urban(const LabSettings& settings) {
    const auto& road = settings.road;
    const double A = road.straightLength, B = road.blockDepth, R = road.curveRadius;
    const double h = road.lanes * road.laneWidth / 2;
    const double e = 2 * h + 3;           // half an intersection box: the other street and a margin
    const double a1 = 0.35 * A, a2 = 0.65 * A; // the two straight-through crossings
    const double pi = std::numbers::pi;
    const double rc1 = std::min(R, (a2 - a1) / 3), top1 = 0.45 * B, bottom1 = 0.45 * B;
    const double W2 = 0.3 * A, L2 = 0.3 * B, y2 = 0.35 * B, rc2 = std::min(R, W2 / 3);
    const double W3 = 0.25 * A, L3 = 0.3 * B, x3 = 0.4 * A, rc3 = std::min(R, W3 / 3);
    if (!(a1 > h + R + e + 5) || !(B > h + R + e + 20) || !(top1 + h - rc1 > e + 5) ||
        !(y2 + h + rc2 < B - e - 5) || !(x3 - h - rc3 > e + 5))
        throw std::invalid_argument("LAB_ROAD_INVALID: the block is too small for its curves and intersections");
    LabNetwork result;
    auto& d = result.document;
    d.network.id = "behaviour-lab-urban";
    d.network.drivingSide = road.drivingSide;
    d.definition = AuthoringDefinition{};
    std::vector<std::pair<double, double>> gaps;
    const auto gap = [&](Path& p, Point from, Point to) {
        p.line(from); const double start = p.at(); p.line(to); gaps.push_back({start, p.at()});
    };

    // The tuned vehicle's loop, anticlockwise round the block, h inside the street centrelines.
    Path main({h + R, h});
    gap(main, {a1 - e, h}, {a1 + e, h});
    gap(main, {a2 - e, h}, {a2 + e, h});
    main.line({A - h - R, h});
    main.arc({A - h - R, h + R}, R, -pi / 2, 0);                  // SE: a sweeping corner
    main.line({A - h, B - e});
    { const double s = main.at(); main.arc({A - e, B - e}, e - h, 0, pi / 2); gaps.push_back({s, main.at()}); } // X2: left
    main.line({e, B - h});
    { const double s = main.at(); main.arc({e, B - e}, e - h, pi / 2, pi); gaps.push_back({s, main.at()}); }    // X3: left
    main.line({h, h + R});
    main.arc({h + R, h + R}, R, pi, 3 * pi / 2);                  // SW: a sweeping corner
    const auto mainLoop = addLoop(d, main.points(), gaps, road.lanes, road.laneWidth, "Main");
    result.mainRoute = putRoute(d, {"", mainLoop.objects});

    // C1: clockwise, crossing the south street northbound at X1a and southbound at X1b.
    gaps.clear();
    Path c1({a1 - h, -bottom1 - h + rc1});
    gap(c1, {a1 - h, -e}, {a1 - h, e});
    c1.line({a1 - h, top1 + h - rc1});
    c1.arc({a1 - h + rc1, top1 + h - rc1}, rc1, pi, pi / 2);
    c1.line({a2 + h - rc1, top1 + h});
    c1.arc({a2 + h - rc1, top1 + h - rc1}, rc1, pi / 2, 0);
    gap(c1, {a2 + h, e}, {a2 + h, -e});
    c1.line({a2 + h, -bottom1 - h + rc1});
    c1.arc({a2 + h - rc1, -bottom1 - h + rc1}, rc1, 0, -pi / 2);
    c1.line({a1 - h + rc1, -bottom1 - h});
    c1.arc({a1 - h + rc1, -bottom1 - h + rc1}, rc1, -pi / 2, -pi);
    const auto cross1 = addLoop(d, c1.points(), gaps, road.lanes, road.laneWidth, "Cross 1");

    // C2: anticlockwise east of the block; runs south through X2 beside the main loop's east street.
    gaps.clear();
    Path c2({A + h, B + L2 - h - rc2});
    gap(c2, {A + h, B + e}, {A + h, B - e});
    c2.line({A + h, y2 + h + rc2});
    c2.arc({A + h + rc2, y2 + h + rc2}, rc2, pi, 3 * pi / 2);
    c2.line({A + W2 - h - rc2, y2 + h});
    c2.arc({A + W2 - h - rc2, y2 + h + rc2}, rc2, -pi / 2, 0);
    c2.line({A + W2 - h, B + L2 - h - rc2});
    c2.arc({A + W2 - h - rc2, B + L2 - h - rc2}, rc2, 0, pi / 2);
    c2.line({A + h + rc2, B + L2 - h});
    c2.arc({A + h + rc2, B + L2 - h - rc2}, rc2, pi / 2, pi);
    const auto cross2 = addLoop(d, c2.points(), gaps, road.lanes, road.laneWidth, "Cross 2");

    // C3: anticlockwise north-west of the block; runs east through X3 beside the main loop's north street.
    gaps.clear();
    Path c3({-W3 + h + rc3, B + h});
    gap(c3, {-e, B + h}, {e, B + h});
    c3.line({x3 - h - rc3, B + h});
    c3.arc({x3 - h - rc3, B + h + rc3}, rc3, -pi / 2, 0);
    c3.line({x3 - h, B + L3 - h - rc3});
    c3.arc({x3 - h - rc3, B + L3 - h - rc3}, rc3, 0, pi / 2);
    c3.line({-W3 + h + rc3, B + L3 - h});
    c3.arc({-W3 + h + rc3, B + L3 - h - rc3}, rc3, pi / 2, pi);
    c3.line({-W3 + h, B + h + rc3});
    c3.arc({-W3 + h + rc3, B + h + rc3}, rc3, pi, 3 * pi / 2);
    const auto cross3 = addLoop(d, c3.points(), gaps, road.lanes, road.laneWidth, "Cross 3");

    for (const auto* loop : {&cross1, &cross2, &cross3}) result.crossRoutes.push_back(putRoute(d, {"", loop->objects}));
    // Offsets a quarter cycle apart, so a lap meets each intersection in a different phase.
    const double quarter = std::round((road.green + road.amber + road.red) / 4); // on the time grid
    signalise(d, "X1a", 0, road, {mainLoop.into[0]}, {cross1.into[0]});
    signalise(d, "X1b", quarter, road, {mainLoop.into[1]}, {cross1.into[1]});
    signalise(d, "X2", 2 * quarter, road, {mainLoop.into[2]}, {cross2.into[0]});
    signalise(d, "X3", 3 * quarter, road, {mainLoop.into[3]}, {cross3.into[0]});
    return result;
}
}
