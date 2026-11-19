#include "t_junction_support.hpp"
#include "../src/core/following.hpp"
#include <map>
using namespace multra;
// LINE_APPROACH §9 (D596), rows LA1-LA4: a vehicle held at a conflict zone's waiting line rests with
// its front at the line, without the crawl the speed-dependent gap made, and goes once.
namespace {
struct Approach { std::vector<double> rests; double longestCrawl{}; int reversals{}, clamps{}; };
// The first minor vehicle in its queue, approaching its first give-way line, seeds 42..46.
Approach measure() {
    Approach a;
    const auto t = fixture::tJunction();
    const auto snapshot = compileDocument(t.document, test::root() / "data");
    for (std::uint32_t seed = 42; seed <= 46; ++seed) {
        auto s = createSimulation(snapshot.scenario, seed);
        struct Track { bool stopped{}, released{}; double releasedAt{}, crawl{}; };
        std::map<std::uint64_t, Track> track;
        const double dt = s.scenario->timeStep;
        while (s.tick < totalTicks(*s.scenario)) {
            s = stepSimulation(s);
            for (const auto& e : s.events) a.clamps += std::holds_alternative<SafetyClampEvent>(e);
            const auto refs = resolveRefs(*s.scenario, s.vehicles, *s.index);
            for (std::size_t v = 0; v < s.vehicles.size(); ++v) {
                const auto& vehicle = s.vehicles[v];
                double line = INFINITY;
                for (const auto& rz : s.index->routeZones[refs[v].route]) if (rz.role == ZoneRole::minor) line = std::min(line, rz.waitAt);
                if (!std::isfinite(line)) continue;
                const double toLine = line - vehicle.distance;
                bool first = true; // nobody of the same approach between it and its line
                for (std::size_t u = 0; u < s.vehicles.size(); ++u) {
                    if (u == v || s.scenario->routes[s.vehicles[u].routeIndex].segmentIds.front() !=
                                  s.scenario->routes[vehicle.routeIndex].segmentIds.front()) continue;
                    const double d = s.vehicles[u].distance - vehicle.distance;
                    if (d > 0 && d < toLine + 6) first = false;
                }
                if (!first) continue;
                auto& k = track[vehicle.id];
                if (toLine > 0 && toLine <= 15 && vehicle.speed > 0.01 && vehicle.speed < 1) {
                    k.crawl += dt; a.longestCrawl = std::max(a.longestCrawl, k.crawl);
                }
                if (toLine >= -0.05 && toLine <= 15 && vehicle.speed < 0.05 && !k.stopped) { k.stopped = true; a.rests.push_back(toLine); }
                if (k.stopped && !k.released && vehicle.acceleration > 0.3 && toLine > 0) { k.released = true; k.releasedAt = s.tick * dt; }
                if (k.released && toLine > 0 && vehicle.acceleration < -2 && s.tick * dt - k.releasedAt < 5) {
                    ++a.reversals; k.released = k.stopped = false;
                }
            }
        }
    }
    return a;
}
}
TEST(zonelinerest, la1_la4_held_vehicles_rest_at_the_line_and_go_once) {
    const auto a = measure();
    CHECK(a.rests.size() > 50); // the forcing: front vehicles really are held at their lines
    // LA1: at rest, the front is at the line. Before D596 the median was 2.65 m short (2.0 m standstill
    // distance plus a speed term that never quite reached zero).
    for (const double r : a.rests) CHECK(r >= -0.05 && r <= 0.05);
    // LA2: no crawl. Before: ~20 s at under 1 m/s for one vehicle.
    CHECK(a.longestCrawl < 3);
    // LA3: a released vehicle does not brake hard again before its line. Before: 15 in 10 seeds.
    CHECK(a.reversals == 0);
    // LA4: no clamp is bought with it.
    CHECK(a.clamps == 0);
}
TEST(zonelinerest, only_a_zone_line_loses_the_standstill_gap) {
    VehicleType car; car.length = 4.5; car.maxAcceleration = 2.5; car.comfortableDeceleration = 3; car.maxDeceleration = 8;
    DriverBehaviour b; b.standstillDistance = 2; b.additiveSafetyDistance = 2; b.multiplicativeSafetyDistance = 3;
    b.followingTime = 1.5; b.speedThreshold = 0.2;
    // Standing 1 m before a zone's line: it moves up to the line; behind a vehicle or another line it holds.
    CHECK(followingAcceleration(0, 13.9, 1, car, b, Leader{1, 0, 0, true}).acceleration > 0);
    CHECK(followingAcceleration(0, 13.9, 1, car, b, Leader{1, 0, 0, false}).acceleration <= 0);
    // At the line it stays.
    CHECK(followingAcceleration(0, 13.9, 1, car, b, Leader{0, 0, 0, true}).acceleration <= 0);
}
