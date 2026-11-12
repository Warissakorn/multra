#include "test.hpp"
#include "../src/eval/priority_lines.hpp"
#include "../src/core/routes.hpp"
#include <algorithm>
#include <cmath>
#include <map>

using namespace multra;
// D528 (docs/reference/PRIORITY_LINES.md): the priority-line counter, rows PL1-PL5, on the
// hand-built crossing of stop_control_tests: the minor "north" crosses the major "east"; the area
// is 98..102 on both; the minor waiting line is at 90 on north, the major entry at 98 on east.
namespace {
Scenario crossing(ZoneControl control, double minorPerHour, double majorPerHour) {
    const auto source = test::demo().scenario;
    Scenario s;
    s.duration = 600; s.timeStep = 0.1;
    s.segments = {{"eastUp", 50, {"east"}}, {"east", 200, {}}, {"north", 200, {}}};
    s.routes = {{"majorRoute", {"eastUp", "east"}}, {"minorRoute", {"north"}}};
    s.vehicleTypes = source.vehicleTypes; s.behaviours = source.behaviours;
    s.conflictZones = {{"zone", {{"east"}, 98, 102}, {{"north"}, 98, 102}, 90, 3, 10, control}};
    s.inputs = {{"minorIn", "minorRoute", "car", minorPerHour, 0, 600}};
    if (majorPerHour > 0) s.inputs.push_back({"majorIn", "majorRoute", "car", majorPerHour, 0, 600});
    return s;
}
EvaluationSpec spec() { EvaluationSpec e; e.queue = {5 / 3.6, 10 / 3.6, 20}; e.warmup = 60; e.end = 600; return e; }
// What a test counts on its own: each passage's interpolated time, and for minor vehicles the time
// they first stood short of the line.
struct Passage { double time{}; std::optional<double> stood; };
struct Independent { std::vector<Passage> minor; std::vector<double> major; };
Independent runBoth(const Scenario& s, PriorityLineAccumulator& counter, std::uint32_t seed = 42) {
    Independent seen;
    auto state = createSimulation(s, seed);
    std::map<std::uint64_t, std::pair<std::size_t, double>> before;
    std::map<std::uint64_t, double> stood;
    const double begin = spec().queue.beginSpeed, dt = s.timeStep;
    const auto look = [&] {
        counter.observe(state);
        // Queued as the contract says: standing, within maxGap of the line or of the rear ahead.
        std::vector<const Vehicle*> minors;
        for (const auto& v : state.vehicles)
            if (state.scenario->routes[v.routeIndex].id == "minorRoute" && v.distance < 90) minors.push_back(&v);
        std::sort(minors.begin(), minors.end(), [](const auto* a, const auto* b) { return a->distance > b->distance || (a->distance == b->distance && a->id < b->id); });
        for (std::size_t k = 0; k < minors.size(); ++k) {
            const double gap = k == 0 ? 90 - minors[k]->distance
                                      : minors[k - 1]->distance - s.vehicleTypes[minors[k - 1]->typeIndex].length - minors[k]->distance;
            if (minors[k]->speed < begin && gap <= spec().queue.maxGap) stood.try_emplace(minors[k]->id, state.time);
        }
        std::map<std::uint64_t, std::pair<std::size_t, double>> now;
        for (const auto& v : state.vehicles) {
            const bool minor = state.scenario->routes[v.routeIndex].id == "minorRoute";
            const double line = minor ? 90 : 50 + 98; // the major route starts on eastUp (50 m)
            now[v.id] = {v.routeIndex, v.distance};
            const auto p = before.find(v.id);
            const double from = p != before.end() ? p->second.second : 0.0;
            if (p == before.end() && v.distance >= line) continue;
            if (from < line && v.distance >= line) {
                const double t = state.time - dt + dt * (line - from) / (v.distance - from);
                CHECK(t > state.time - dt - 1e-9); CHECK(t <= state.time + 1e-9); // PL1: inside the tick
                if (minor) seen.minor.push_back({t, stood.contains(v.id) ? std::optional{stood[v.id]} : std::nullopt});
                else seen.major.push_back(t);
            }
        }
        before = std::move(now);
    };
    look();
    while (state.tick < totalTicks(s)) { state = stepSimulation(std::move(state)); look(); }
    return seen;
}
bool inWindow(double t) { return t >= 60 && t <= 600; }
std::vector<double> followUps(const Independent& seen) {
    std::vector<double> gaps;
    for (std::size_t k = 1; k < seen.minor.size(); ++k) {
        const auto& l = seen.minor[k - 1]; const auto& f = seen.minor[k];
        if (!inWindow(l.time) || !inWindow(f.time) || !f.stood || *f.stood > l.time) continue;
        if (std::any_of(seen.major.begin(), seen.major.end(), [&](double m) { return m > l.time && m < f.time; })) continue;
        gaps.push_back(f.time - l.time);
    }
    return gaps;
}
std::uint64_t countIn(const std::vector<double>& times) { return std::count_if(times.begin(), times.end(), inWindow); }
}
TEST(prioritylines, pl1_counts_equal_the_passages_counted_independently) {
    PriorityLineAccumulator counter(spec());
    const auto seen = runBoth(crossing(ZoneControl::stop, 900, 400), counter);
    const auto r = counter.report();
    CHECK(r.rows.size() == 1); const auto& row = r.rows.front();
    std::vector<double> minorTimes; for (const auto& p : seen.minor) minorTimes.push_back(p.time);
    CHECK(row.minorCrossed == countIn(minorTimes)); CHECK(row.majorEntered == countIn(seen.major));
    CHECK(row.minorCrossed > 0 && row.majorEntered > 0); // the forcing: both sides moved
    test::near(row.window, 540.0); test::near(row.minorPerHour, row.minorCrossed * 3600.0 / 540);
    CHECK(row.excluded == 0); CHECK(row.control == ZoneControl::stop);
}
TEST(prioritylines, pl2_saturated_minor_without_major_traffic) {
    PriorityLineAccumulator counter(spec());
    const auto seen = runBoth(crossing(ZoneControl::stop, 1800, 0), counter);
    const auto row = counter.report().rows.front();
    const auto gaps = followUps(seen);
    CHECK(gaps.size() >= 10); CHECK(row.majorEntered == 0);
    CHECK(row.followUps == gaps.size());
    double sum = 0; for (const double g : gaps) sum += g;
    test::near(*row.followUpMean, sum / static_cast<double>(gaps.size()));
    CHECK(row.followUpSd.has_value());
}
TEST(prioritylines, pl3_a_major_entry_between_two_departures_breaks_the_pair) {
    PriorityLineAccumulator counter(spec());
    const auto seen = runBoth(crossing(ZoneControl::stop, 1800, 600), counter);
    const auto row = counter.report().rows.front();
    std::size_t queuedPairs = 0; // consecutive in-window pairs whose follower stood in time, majors ignored
    for (std::size_t k = 1; k < seen.minor.size(); ++k) {
        const auto& l = seen.minor[k - 1]; const auto& f = seen.minor[k];
        queuedPairs += inWindow(l.time) && inWindow(f.time) && f.stood && *f.stood <= l.time;
    }
    CHECK(row.followUps == followUps(seen).size());
    CHECK(row.followUps < queuedPairs); // the forcing: some queued pairs straddle a major entry
}
TEST(prioritylines, pl4_released_at_rest_is_not_queued_and_yield_is_named) {
    PriorityLineAccumulator counter(spec());
    const auto seen = runBoth(crossing(ZoneControl::yield, 60, 0), counter);
    const auto row = counter.report().rows.front();
    CHECK(row.control == ZoneControl::yield);
    // Released at rest at the source, a vehicle is not queued; only a pair the arrivals bunched
    // behind each other within maxGap follows up, as counted independently.
    std::size_t releasedAtRest = 0;
    for (const auto& p : seen.minor) releasedAtRest += inWindow(p.time);
    CHECK(row.minorCrossed == releasedAtRest && row.minorCrossed > 0);
    CHECK(row.followUps == followUps(seen).size());
    CHECK(row.followUps * 2 < row.minorCrossed);
}
TEST(prioritylines, pl5_the_same_seed_gives_the_same_report) {
    PriorityLineAccumulator a(spec()), b(spec());
    runBoth(crossing(ZoneControl::stop, 900, 400), a, 7);
    runBoth(crossing(ZoneControl::stop, 900, 400), b, 7);
    CHECK(a.report().rows == b.report().rows);
}
