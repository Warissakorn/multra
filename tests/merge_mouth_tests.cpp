#include "t_junction_support.hpp"
#include "../src/model/network/conflict_display.hpp"
using namespace multra;
using namespace tjunction;
// M3.2.4e, rows MM1-MM6 of docs/reference/MERGE_MOUTH.md: an authored merge covers the measured
// overlap of the two surfaces, and each side waits 1 m short of it.
namespace {
struct Mouth { ConflictSide minor, major; StationInterval minorSpan, majorSpan; double minorLine{}, majorLine{}; };
double lineAt(const Network& n, const std::string& id) {
    for (const auto& w : n.rightOfWay.waitingLines) if (w.id == id) return w.point.station;
    throw std::runtime_error("no line " + id);
}
// The measured overlap on each side of one merge, independent of the extents the area stores.
Mouth mouthOf(const fixture::TJunction& t, const std::string& area) {
    const auto& n = t.document.network;
    for (const auto& a : n.rightOfWay.conflictAreas) {
        if (a.id != area) continue;
        const bool firstMinor = !a.first.path.connectorId.empty();
        const auto g = conflictAreaGeometry(n, a.kind, a.first, a.second);
        const auto span = [](const std::vector<ConflictSide>& s) {
            StationInterval r{s.front().entryStation, s.front().exitStation};
            for (const auto& x : s) { r.from = std::min(r.from, x.entryStation); r.to = std::max(r.to, x.exitStation); }
            return r;
        };
        if (g.first.empty() || g.second.empty()) throw std::runtime_error("no overlap measured");
        Mouth m{firstMinor ? a.first : a.second, firstMinor ? a.second : a.first,
                span(firstMinor ? g.first : g.second), span(firstMinor ? g.second : g.first)};
        m.minorLine = lineAt(n, m.minor.waitingLineId); m.majorLine = lineAt(n, m.major.waitingLineId);
        return m;
    }
    throw std::runtime_error("no area " + area);
}
// The join on the major Link: where the minor Connector ends on it.
double joinOn(const fixture::TJunction& t, const Mouth& m) {
    for (const auto& c : t.document.network.connectors)
        if (c.id == m.minor.path.connectorId) return attachmentStation(t.document.network, c.to, false);
    throw std::runtime_error("no connector");
}
fixture::TJunction variant(DrivingSide side) { fixture::TJunctionOptions o; o.side = side; return fixture::tJunction(o); }
}
TEST(mergemouth, mm1_mm2_each_side_waits_a_metre_short_of_the_measured_overlap) {
    for (const auto side : {DrivingSide::right, DrivingSide::left}) {
        const auto t = variant(side);
        for (const auto* area : {&t.crossingMerge, &t.nearMerge}) {
            const auto m = mouthOf(t, *area);
            CHECK(m.minorSpan.from > 5); // the forcing: the overlap starts well before the Connector's end
            CHECK(m.minorLine <= std::max(0.0, m.minorSpan.from - 1) + 1e-6);
            CHECK(m.majorLine <= m.majorSpan.from - 1 + 1e-6);
        }
    }
}
TEST(mergemouth, mm3_the_extents_contain_the_overlap_up_to_the_join) {
    for (const auto side : {DrivingSide::right, DrivingSide::left}) {
        const auto t = variant(side);
        for (const auto* area : {&t.crossingMerge, &t.nearMerge}) {
            const auto m = mouthOf(t, *area);
            CHECK(m.majorSpan.to > joinOn(t, m)); // the forcing: the measured mouth tapers past the join
            CHECK(m.minor.entryStation <= m.minorSpan.from + 1e-6 && m.minor.exitStation >= m.minorSpan.to - 1e-6);
            CHECK(m.major.entryStation <= m.majorSpan.from + 1e-6);
            CHECK(std::abs(m.major.exitStation - joinOn(t, m)) <= 1e-6); // clipped: past it one lane is shared
        }
    }
}
namespace {
// Route distances of a measured span on one route, or none when the route does not carry it.
std::optional<StationInterval> onRoute(const std::vector<RoutePart>& parts,
                                       const fixture::TJunction& t, const ConflictSide& side, StationInterval span) {
    const auto table = runtimeSections(t.document.network);
    const auto from = locateControlPoint(t.document.network, table, {side.path, span.from});
    const auto to = locateControlPoint(t.document.network, table, {side.path, span.to});
    if (!from || !to) throw std::runtime_error("span does not locate");
    std::optional<double> a, b;
    for (const auto& p : parts) {
        if (!a && p.segmentId == from->segment) a = p.start + from->position;
        if (a && p.segmentId == to->segment) b = p.start + to->position;
    }
    if (!a || !b) return std::nullopt;
    return StationInterval{*a, *b};
}
}
TEST(mergemouth, mm4_no_minor_and_major_body_share_a_measured_mouth) {
    const auto t = fixture::tJunction();
    const auto snapshot = compileDocument(t.document, test::root() / "data");
    auto s = createSimulation(snapshot.scenario, 42);
    struct Watch { std::vector<std::optional<StationInterval>> minor, major; };
    std::vector<Watch> watches;
    for (const auto* area : {&t.crossingMerge, &t.nearMerge}) {
        auto m = mouthOf(t, *area);
        m.majorSpan.to = std::min(m.majorSpan.to, joinOn(t, m)); // past the join, car-following
        Watch w;
        for (const auto& parts : s.index->parts) {
            w.minor.push_back(onRoute(parts, t, m.minor, m.minorSpan));
            w.major.push_back(onRoute(parts, t, m.major, m.majorSpan));
        }
        watches.push_back(std::move(w));
    }
    int shared = 0, minorInside = 0;
    while (s.tick < totalTicks(*s.scenario)) {
        s = stepSimulation(s);
        const auto refs = resolveRefs(*s.scenario, s.vehicles, *s.index);
        for (const auto& w : watches) {
            bool minor = false, major = false;
            for (std::size_t v = 0; v < s.vehicles.size(); ++v) {
                const auto& vehicle = s.vehicles[v];
                const double rear = vehicle.distance - s.scenario->vehicleTypes[refs[v].type].length;
                const auto over = [&](const std::optional<StationInterval>& span) {
                    return span && vehicle.distance > span->from + 1e-9 && rear < span->to - 1e-9; };
                minor = minor || over(w.minor[refs[v].route]);
                major = major || over(w.major[refs[v].route]);
            }
            minorInside += minor;
            shared += minor && major;
        }
    }
    CHECK(minorInside > 0); // the forcing: minor vehicles do pass through the mouths
    CHECK(shared == 0);
}
TEST(mergemouth, mm5_the_crossing_turn_waits_at_the_crossing_line_for_both) {
    const auto t = fixture::tJunction();
    const auto snapshot = compileDocument(t.document, test::root() / "data");
    const auto s = createSimulation(snapshot.scenario, 42);
    const auto crossing = zoneIndex(*s.scenario, "right-of-way/" + t.crossingArea);
    const auto merge = zoneIndex(*s.scenario, "right-of-way/" + t.crossingMerge);
    bool checked = false;
    for (const auto& zones : s.index->routeZones) {
        const RouteZone *c = nullptr, *m = nullptr;
        for (const auto& rz : zones) if (rz.role == ZoneRole::minor && rz.zoneIndex == crossing) c = &rz;
        for (const auto& rz : zones) if (rz.role == ZoneRole::minor && rz.zoneIndex == merge) m = &rz;
        if (!c || !m) continue;
        checked = true;
        CHECK(std::abs(c->waitAt - m->waitAt) <= 1e-9);
        CHECK(std::abs(c->clearAt - m->exitAt) <= 1e-9);
    }
    CHECK(checked);
}
TEST(mergemouth, mm6_every_movement_still_discharges) {
    const auto t = fixture::tJunction();
    const auto r = run(t);
    for (const auto* name : {"Minor approach → Major westbound", "Minor approach → Major eastbound",
                             "Major eastbound → Major eastbound", "Major westbound → Major westbound"})
        CHECK(movement(r.report, name).vehicles > 0);
    CHECK(!r.bothSidesInside);
}
