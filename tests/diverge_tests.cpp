#include "test.hpp"
#include "../tools/four_leg_network.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/model/network/right_of_way.hpp"
#include "../src/model/network/connector_surface.hpp"
#include "../src/core/routes.hpp"
#include "../src/project/run.hpp"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <limits>
#include <map>
#include <numbers>
#include <set>
using namespace multra;
// M3.2's diverge acceptance fixture, rows A62-A66 (M3_ACCEPTANCE §1, "Diverge acceptance fixture"):
// one lane A splits by two Connectors into B and C, with an authored route and input to each. Branching
// is original order (M3_CONTRACT, D109): no zone, no rule, vehicles keep their order on the shared lane.
namespace {
const auto data = [] { return test::root() / "data"; };
struct Fork { ProjectDocument d; std::string a, b, c, toB, toC; };
Fork splitFixture(double perBranch = 450) {
    Fork f; f.d.definition = AuthoringDefinition{};
    f.a = addLink(f.d, {{0, 0}, {200, 0}}, 1, 3.5);
    f.b = addLink(f.d, {{230, 5}, {400, 5}}, 1, 3.5);
    f.c = addLink(f.d, {{230, -5}, {400, -5}}, 1, 3.5);
    const auto lane = [&](const std::string& l) { return fixture::detail::lane(f.d, l, 0); };
    f.toB = addConnector(f.d, {f.a, lane(f.a)}, {f.b, lane(f.b)});
    f.toC = addConnector(f.d, {f.a, lane(f.a)}, {f.c, lane(f.c)});
    const auto rb = putRoute(f.d, {"to-b", {f.a, f.toB, f.b}});
    const auto rc = putRoute(f.d, {"to-c", {f.a, f.toC, f.c}});
    putInput(f.d, {"in-b", rb, "car", perBranch, 0, 300});
    putInput(f.d, {"in-c", rc, "car", perBranch, 0, 300});
    changeRunSettings(f.d, 420, 0.1);
    return f;
}
// The compiled geometry the rows measure in: route metres, the shared lane first on both routes.
struct Shape { std::size_t routeB{}, routeC{}; double shared{}; };
Shape shape(const Scenario& s) {
    Shape r{SIZE_MAX, SIZE_MAX, 0};
    for (std::size_t i = 0; i < s.routes.size(); ++i) (s.routes[i].id.find("to-b") != std::string::npos ? r.routeB : r.routeC) = i;
    CHECK(s.routes.size() == 2 && r.routeB != SIZE_MAX && r.routeC != SIZE_MAX);
    const auto& first = s.routes[r.routeB].segmentIds.front();
    CHECK(first == s.routes[r.routeC].segmentIds.front()); // the forcing: one shared first segment
    CHECK(s.routes[r.routeB].segmentIds[1] != s.routes[r.routeC].segmentIds[1]);
    for (const auto& seg : s.segments) if (seg.id == first) r.shared = seg.length;
    CHECK(r.shared > 0);
    return r;
}
double lengthOf(const SimState& st, const Vehicle& v) { return st.scenario->vehicleTypes[v.typeIndex].length; }
double widthOf(const SimState& st, const Vehicle& v) { return st.scenario->vehicleTypes[v.typeIndex].width; }
// World geometry of each route, for the bodies' footprints: the Links' centre lines and each
// Connector's centre between its two painted rails (the runtime lane centre, D114). A compiled metre
// maps to the drawn centre line in proportion within each part.
struct Path { std::vector<Point> points; std::vector<double> at; }; // `at`: route metres at each point
struct Pose { Point centre; double heading{}; };
Path pathOf(const Fork& f, const Scenario& s, std::size_t route, const std::string& connector, const std::string& exit) {
    const auto& net = f.d.network;
    const auto link = [&](const std::string& id) { return std::find_if(net.links.begin(), net.links.end(), [&](auto& l) { return l.id == id; })->geometry; };
    const auto& c = *std::find_if(net.connectors.begin(), net.connectors.end(), [&](auto& x) { return x.id == connector; });
    const auto rails = connectorSurface(net, c).boundaries;
    CHECK(rails.size() == 2 && rails[0].size() == rails[1].size());
    std::vector<Point> mid;
    for (std::size_t i = 0; i < rails[0].size(); ++i) mid.push_back({(rails[0][i].x + rails[1][i].x) / 2, (rails[0][i].y + rails[1][i].y) / 2});
    Path p; double start = 0;
    const auto& ids = s.routes[route].segmentIds;
    const std::vector<std::vector<Point>> parts{link(f.a), mid, link(exit)};
    for (std::size_t k = 0; k < parts.size(); ++k) {
        double drawn = 0;
        for (std::size_t i = 1; i < parts[k].size(); ++i) drawn += std::hypot(parts[k][i].x - parts[k][i - 1].x, parts[k][i].y - parts[k][i - 1].y);
        const double compiled = std::find_if(s.segments.begin(), s.segments.end(), [&](auto& g) { return g.id == ids[k]; })->length;
        double along = 0;
        for (std::size_t i = 0; i < parts[k].size(); ++i) {
            if (i) along += std::hypot(parts[k][i].x - parts[k][i - 1].x, parts[k][i].y - parts[k][i - 1].y);
            if (k && !i) continue; // the joint is the previous part's last point
            p.points.push_back(parts[k][i]); p.at.push_back(start + along / drawn * compiled);
        }
        start += compiled;
    }
    return p;
}
Pose poseAt(const Path& p, double d) {
    std::size_t i = 1;
    while (i + 1 < p.at.size() && p.at[i] < d) ++i;
    const auto a = p.points[i - 1], b = p.points[i];
    const double t = std::clamp((d - p.at[i - 1]) / (p.at[i] - p.at[i - 1]), 0.0, 1.0);
    return {{a.x + t * (b.x - a.x), a.y + t * (b.y - a.y)}, std::atan2(b.y - a.y, b.x - a.x)};
}
// How deep two footprints (centre, heading, length, width) overlap: the least overlap over the four
// separating axes, 0 when they are apart.
double penetration(Pose p, double pl, double pw, Pose q, double ql, double qw) {
    double least = std::numeric_limits<double>::infinity();
    for (const double h : {p.heading, p.heading + std::numbers::pi / 2, q.heading, q.heading + std::numbers::pi / 2}) {
        const double ax = std::cos(h), ay = std::sin(h);
        const auto half = [&](const Pose& r, double l, double w) {
            return l / 2 * std::abs(std::cos(r.heading) * ax + std::sin(r.heading) * ay) + w / 2 * std::abs(-std::sin(r.heading) * ax + std::cos(r.heading) * ay);
        };
        const double apart = std::abs((q.centre.x - p.centre.x) * ax + (q.centre.y - p.centre.y) * ay);
        least = std::min(least, half(p, pl, pw) + half(q, ql, qw) - apart);
        if (least <= 0) return 0;
    }
    return least;
}
struct Watch {
    std::vector<std::uint64_t> entered, leftShared; // departure order; the order fronts passed the shared lane's end
    std::size_t clamps{}, overlaps{}, mouthOverlaps{};
    bool spilled{}; // a vehicle to B stood with its rear inside the split (`spilledWithin` metres past it)
    double worst{}; // the deepest footprint overlap between the two branches (m)
};
// Runs to the end, every tick checking the bodies: along the route on the shared lane and on one
// branch, and as footprints in the plane between vehicles on the two branches.
Watch watch(SimState st, const Shape& sh, const std::array<Path, 2>& paths, double spilledWithin = 12) {
    Watch w; std::set<std::uint64_t> passed;
    while (st.tick < totalTicks(*st.scenario)) {
        st = stepSimulation(std::move(st));
        for (const auto& e : st.events) {
            if (std::holds_alternative<SafetyClampEvent>(e)) ++w.clamps;
            if (const auto* d = std::get_if<DepartedEvent>(&e)) w.entered.push_back(d->vehicleId);
        }
        std::vector<const Vehicle*> by(st.vehicles.size());
        for (std::size_t i = 0; i < by.size(); ++i) by[i] = &st.vehicles[i];
        std::sort(by.begin(), by.end(), [](auto* x, auto* y) { return x->distance < y->distance; });
        for (const auto* v : by) {
            if (v->distance > sh.shared && passed.insert(v->id).second) w.leftShared.push_back(v->id);
            const double rear = v->distance - lengthOf(st, *v) - sh.shared;
            if (v->routeIndex == sh.routeB && v->speed < 0.1 && rear > 0 && rear < spilledWithin) w.spilled = true;
        }
        for (std::size_t i = 0; i + 1 < by.size(); ++i)
            for (std::size_t j = i + 1; j < by.size(); ++j) {
                const auto *back = by[i], *ahead = by[j];
                const double rear = ahead->distance - lengthOf(st, *ahead);
                if (rear > sh.shared + 40) break; // well past the split: the branches are apart
                const bool sameRoute = back->routeIndex == ahead->routeIndex;
                const bool onShared = rear < sh.shared - 1e-9 || back->distance < sh.shared - 1e-9;
                if (sameRoute || onShared) { if (back->distance - rear > 1e-9) ++w.overlaps; continue; }
                const auto body = [&](const Vehicle* v) {
                    return poseAt(paths[v->routeIndex == sh.routeB ? 0 : 1], v->distance - lengthOf(st, *v) / 2);
                };
                const double depth = penetration(body(back), lengthOf(st, *back), widthOf(st, *back), body(ahead), lengthOf(st, *ahead), widthOf(st, *ahead));
                if (depth > 1e-6) { ++w.mouthOverlaps; w.worst = std::max(w.worst, depth); }
            }
    }
    return w;
}
std::array<Path, 2> paths(const Fork& f, const Scenario& s, const Shape& sh) {
    return {pathOf(f, s, sh.routeB, f.toB, f.b), pathOf(f, s, sh.routeC, f.toC, f.c)};
}
}
TEST(diverge, a62_a_plain_diverge_is_branching_with_no_zone_and_no_rule) {
    const auto f = splitFixture();
    const auto all = automaticConflicts(f.d.network);
    std::size_t pair = 0;
    for (const auto& a : all) {
        CHECK(a.kind == ConflictKind::branching); // no crossing and no merge at a diverge
        const std::set<std::string> owners{a.first.path.connectorId, a.second.path.connectorId};
        if (owners == std::set<std::string>{f.toB, f.toC}) ++pair; // the two branches against each other
        else CHECK(a.first.path.linkId == f.a || a.second.path.linkId == f.a); // a branch against the lane it leaves
    }
    CHECK(pair == 1);
    CHECK(f.d.network.rightOfWay.conflictAreas.empty()); // derived, never stored
    validateDocument(f.d);
    const auto s = compileDocument(f.d, data()).scenario;
    CHECK(s.conflictZones.empty()); CHECK(s.priorityRules.empty()); CHECK(s.laneChanges.empty());
    shape(s);
}
TEST(diverge, a63_vehicles_leave_the_shared_lane_in_the_order_they_entered_it) {
    const auto f = splitFixture();
    const auto s = compileDocument(f.d, data()).scenario;
    const auto sh = shape(s);
    for (std::uint64_t seed = 42; seed <= 51; ++seed) {
        auto st = createSimulation(s, seed);
        const auto w = watch(st, sh, paths(f, s, sh));
        if (std::getenv("MULTRA_DIVERGE_REPORT"))
            std::printf("free seed %llu footprint-ticks %zu deepest %.2f m\n", static_cast<unsigned long long>(seed), w.mouthOverlaps, w.worst);
        CHECK(w.leftShared.size() > 50); // the forcing: both branches carried traffic through the split
        CHECK(std::equal(w.leftShared.begin(), w.leftShared.end(), w.entered.begin()));
        CHECK(w.clamps == 0); CHECK(w.overlaps == 0);
        CHECK(w.mouthOverlaps == 0); // free flow: no footprints touch across the split either
    }
}
namespace {
// A red head `at` metres along segment `k` of the route to B, red for `red` seconds then green to the end.
Scenario heldOnB(Scenario s, const Shape& sh, std::size_t k, double at, double red) {
    s.signalPrograms.push_back({"hold", 0, {{red, SignalColor::red}, {10000, SignalColor::green}}});
    s.signalHeads.push_back({"hold-b", s.routes[sh.routeB].segmentIds[k], at, "hold"});
    return s;
}
}
// A64 (D623): a body inside the branching area also occupies the sibling branch, so a follower to C keeps
// behind a car to B until its rear has left the area. `MULTRA_DIVERGE_REPORT=1` prints the per-seed
// footprints (evidence/diverge-acceptance.md had 8 of 10 seeds overlapping before D623).
TEST(diverge, a64_a_queue_spilling_back_over_the_split_keeps_each_path_in_order) {
    const auto f = splitFixture();
    const auto base = compileDocument(f.d, data()).scenario;
    const auto sh = shape(base);
    const auto s = heldOnB(base, sh, 2, 30, 120); // 30 m into B: its queue reaches back over the split
    std::size_t spilled = 0;
    for (std::uint64_t seed = 42; seed <= 51; ++seed) {
        const auto w = watch(createSimulation(s, seed), sh, paths(f, s, sh));
        spilled += w.spilled;
        if (std::getenv("MULTRA_DIVERGE_REPORT"))
            std::printf("seed %llu clamps %zu path-overlaps %zu footprint-ticks %zu deepest %.2f m spilled %d\n",
                        static_cast<unsigned long long>(seed), w.clamps, w.overlaps, w.mouthOverlaps, w.worst, int(w.spilled));
        CHECK(w.clamps == 0); CHECK(w.overlaps == 0); CHECK(w.mouthOverlaps == 0);
        CHECK(std::equal(w.leftShared.begin(), w.leftShared.end(), w.entered.begin()));
    }
    CHECK(spilled >= 9); // the forcing: a vehicle to B stood with its rear inside the split (seed 42 stops short)
}
TEST(diverge, a65_a_branch_blocked_at_the_split_holds_the_shared_lane_behind_it) {
    const auto f = splitFixture();
    const auto base = compileDocument(f.d, data()).scenario;
    const auto sh = shape(base);
    const auto s = heldOnB(base, sh, 1, 0.5, 90); // on the Connector to B, just past the split
    auto st = createSimulation(s, 42);
    std::optional<std::uint64_t> held; std::size_t heldTicks = 0, passedHeld = 0, cWhileHeld = 0;
    std::map<std::uint64_t, std::size_t> order; std::size_t departures = 0;
    while (st.tick < totalTicks(*st.scenario)) {
        st = stepSimulation(std::move(st));
        for (const auto& e : st.events) if (const auto* d = std::get_if<DepartedEvent>(&e)) order[d->vehicleId] = departures++;
        if (st.time > 90) break;
        const Vehicle* first = nullptr; // a vehicle to B standing (below 0.1 m/s) within 3 m of the line
        for (const auto& v : st.vehicles)
            if (v.routeIndex == sh.routeB && v.speed < 0.1 && v.distance <= sh.shared + 0.5 && v.distance > sh.shared - 2.5) first = &v;
        if (!first) continue;
        held = first->id; ++heldTicks;
        for (const auto& v : st.vehicles) {
            if (order[v.id] > order[first->id] && v.distance > first->distance) ++passedHeld;
            if (order[v.id] > order[first->id] && v.routeIndex == sh.routeC && v.distance > sh.shared) ++cWhileHeld;
        }
    }
    CHECK(held.has_value()); CHECK(heldTicks > 300); // the forcing: a vehicle to B stood at the split for over 30 s
    CHECK(passedHeld == 0); CHECK(cWhileHeld == 0); // nobody behind it reached C: the shared lane is first-in, first-out
    // After the hold, everything drains with no clamp and in order.
    const auto w = watch(createSimulation(s, 42), sh, paths(f, s, sh));
    CHECK(w.clamps == 0); CHECK(w.overlaps == 0);
    CHECK(std::equal(w.leftShared.begin(), w.leftShared.end(), w.entered.begin()));
}
TEST(diverge, a66_a_copied_state_through_the_split_replays_exactly) {
    const auto f = splitFixture();
    const auto base = compileDocument(f.d, data()).scenario;
    const auto s = heldOnB(base, shape(base), 2, 30, 120);
    auto st = createSimulation(s, 42);
    while (st.tick < 1500) st = stepSimulation(std::move(st));
    CHECK(st.vehicles.size() > 10); // the forcing: copied while a queue stands across the split
    auto a = st, b = st;
    std::size_t compared = 0;
    while (a.tick < totalTicks(*a.scenario)) {
        a = stepSimulation(std::move(a)); b = stepSimulation(std::move(b));
        CHECK(a.events == b.events); compared += a.events.size();
    }
    CHECK(compared > 0); CHECK(a.vehicles.empty() && b.vehicles.empty()); // drained, no vehicle lost
}
namespace {
// One car to B placed standing with its front `front` metres past the split, held there by a red head
// just ahead of it; the inputs to B are dropped, so only cars to C come up behind it.
SimState standingOnB(const Fork& f, const Shape& sh, double front) {
    auto s = heldOnB(compileDocument(f.d, data()).scenario, sh, 1, front + 2.5, 10000);
    std::erase_if(s.inputs, [](const auto& i) { return i.id.find("in-b") != std::string::npos; });
    auto st = createSimulation(s, 42);
    Vehicle v;
    v.id = st.nextVehicleId++; v.distance = sh.shared + front; v.speed = 0;
    for (std::uint32_t r = 0; r < st.scenario->routes.size(); ++r) if (st.scenario->routes[r].id.find("to-b") != std::string::npos) v.routeIndex = r;
    for (std::uint32_t t = 0; t < st.scenario->vehicleTypes.size(); ++t) if (st.scenario->vehicleTypes[t].id == "car") v.typeIndex = t;
    v.desiredSpeed = 15; v.driverFactor = 0.5;
    st.vehicles.push_back(v);
    return st;
}
double touchOnB(const Scenario& s, const Shape& sh) {
    for (const auto& b : s.branchings) {
        if (b.firstSegmentId == s.routes[sh.routeB].segmentIds[1]) return b.firstLength;
        if (b.secondSegmentId == s.routes[sh.routeB].segmentIds[1]) return b.secondLength;
    }
    return 0;
}
}
TEST(diverge, a67_each_branching_pair_compiles_once_with_the_editors_reach) {
    const auto f = splitFixture();
    const auto s = compileDocument(f.d, data()).scenario;
    const auto sh = shape(s);
    CHECK(s.branchings.size() == 1); // the pair; the two mouth pieces against A give none
    const auto& b = s.branchings.front();
    const std::set<std::string> segments{b.firstSegmentId, b.secondSegmentId};
    CHECK(segments == (std::set<std::string>{s.routes[sh.routeB].segmentIds[1], s.routes[sh.routeC].segmentIds[1]}));
    double exit = 0; // the editor's own reach, in Connector stations
    for (const auto& a : automaticConflicts(f.d.network))
        if (!a.first.path.connectorId.empty() && !a.second.path.connectorId.empty()) exit = a.first.exitStation;
    CHECK(exit > 5);
    // Located on the runtime Connector, whose metres run a little longer than the drawn stations.
    for (const double length : {b.firstLength, b.secondLength}) { CHECK(length >= exit - 1e-9); CHECK(length < exit * 1.05); }
    CHECK(validateScenario(s).empty());
    // Two Connectors from two different lanes that never overlap: no branching, nothing compiled.
    Fork g; g.d.definition = AuthoringDefinition{};
    g.a = addLink(g.d, {{0, 0}, {200, 0}}, 2, 3.5);
    g.b = addLink(g.d, {{230, 8}, {400, 8}}, 1, 3.5);
    g.c = addLink(g.d, {{230, -8}, {400, -8}}, 1, 3.5);
    const auto lane = [&](const std::string& l, int k) { return fixture::detail::lane(g.d, l, k); };
    const auto left = addConnector(g.d, {g.a, lane(g.a, 1)}, {g.b, lane(g.b, 0)});
    const auto right = addConnector(g.d, {g.a, lane(g.a, 0)}, {g.c, lane(g.c, 0)});
    putInput(g.d, {"in-b", putRoute(g.d, {"to-b", {g.a, left, g.b}}), "car", 300, 0, 60});
    putInput(g.d, {"in-c", putRoute(g.d, {"to-c", {g.a, right, g.c}}), "car", 300, 0, 60});
    changeRunSettings(g.d, 60, 0.1);
    std::size_t drawn = 0;
    for (const auto& a : automaticConflicts(g.d.network))
        drawn += a.kind == ConflictKind::branching && !a.first.path.connectorId.empty() && !a.second.path.connectorId.empty();
    CHECK(compileDocument(g.d, data()).scenario.branchings.size() == drawn); // exactly what the editor draws
}
TEST(diverge, a68_a_car_standing_past_the_area_holds_nobody_on_the_other_branch) {
    const auto f = splitFixture();
    const auto base = compileDocument(f.d, data()).scenario;
    const auto sh = shape(base);
    const double touch = touchOnB(base, sh);
    CHECK(touch > 5);
    // Past the area (rear beyond its reach) cars to C go by; inside it, they wait behind it.
    for (const auto& [front, past] : {std::pair{touch + 4.5 + 3, true}, std::pair{touch, false}}) {
        auto st = standingOnB(f, sh, front);
        const auto standing = st.vehicles.front().id;
        const double length = st.scenario->vehicleTypes[st.vehicles.front().typeIndex].length;
        CHECK((st.vehicles.front().distance - length - sh.shared > touch) == past); // the forcing: where its rear is
        std::size_t reachedC = 0;
        while (st.time < 120) {
            st = stepSimulation(std::move(st));
            const auto it = std::find_if(st.vehicles.begin(), st.vehicles.end(), [&](const auto& v) { return v.id == standing; });
            CHECK(it != st.vehicles.end() && it->speed < 0.1 && std::abs(it->distance - (sh.shared + front)) < 0.5); // it stands
            for (const auto& v : st.vehicles) if (v.routeIndex == sh.routeC && v.distance > sh.shared + touch + 4.5) ++reachedC;
        }
        CHECK((reachedC > 0) == past);
    }
}
TEST(diverge, a69_a_scenario_without_a_diverge_compiles_no_branching) {
    CHECK(test::demo().scenario.branchings.empty()); // M0's crossing.json, and so the TS baselines' runs
    ProjectDocument d; d.definition = AuthoringDefinition{};
    const auto link = addLink(d, {{0, 0}, {300, 0}}, 1, 3.5);
    putInput(d, {"in", putRoute(d, {"r", {link}}), "car", 600, 0, 60}); changeRunSettings(d, 60, 0.1);
    const auto s = compileDocument(d, data()).scenario;
    CHECK(s.branchings.empty()); CHECK(!buildScenarioIndex(s).branchings);
}
TEST(diverge, a70_a_state_copied_with_a_body_on_the_sibling_replays_exactly) {
    const auto f = splitFixture();
    const auto base = compileDocument(f.d, data()).scenario;
    const auto sh = shape(base);
    auto st = createSimulation(heldOnB(base, sh, 2, 30, 120), 42);
    const auto index = buildScenarioIndex(*st.scenario);
    const auto ghosts = [&](const SimState& x) {
        const auto spans = occupiedSpans(*x.scenario, x.vehicles, index);
        return std::count_if(spans.begin(), spans.end(), [](const auto& span) { return !span.own; });
    };
    while (ghosts(st) == 0 && st.tick < totalTicks(*st.scenario)) st = stepSimulation(std::move(st));
    CHECK(ghosts(st) > 0); // the forcing: copied with a body occupying the sibling branch
    auto a = st, b = st;
    while (a.tick < totalTicks(*a.scenario)) {
        a = stepSimulation(std::move(a)); b = stepSimulation(std::move(b));
        CHECK(a.events == b.events);
    }
    CHECK(a.vehicles.empty() && b.vehicles.empty());
}
