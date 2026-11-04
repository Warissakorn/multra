#include "test.hpp"
#include "../tools/weaving_network.hpp"
#include "../tools/weaving_sweep.hpp"
#include <fstream>
#include <map>
#include <set>
using namespace multra;
// D449: the weaving lab (tools/weaving_network.hpp; docs/reference/WEAVING_FIXTURE.md, rows
// BA179-BA183). Three Type A weaves, the weave the entry Link, the origin the entry lane.
namespace {
const auto kFile = "data/projects/weaving-lab.traffic.json";
Json committed() { std::ifstream f(test::root() / kFile); Json j; f >> j; return j; }
// Structure and strings exactly, numbers to 1e-9 m: Connector curves are computed geometry (as
// `lanelab` compares its file).
bool same(const Json& a, const Json& b) {
    if (a.is_number() && b.is_number()) return std::abs(a.get<double>() - b.get<double>()) <= 1e-9;
    if (a.type() != b.type() || a.size() != b.size()) return false;
    if (a.is_object()) {
        for (auto it = a.begin(); it != a.end(); ++it) if (!b.contains(it.key()) || !same(it.value(), b.at(it.key()))) return false;
        return true;
    }
    if (a.is_array()) { for (std::size_t i = 0; i < a.size(); ++i) if (!same(a[i], b[i])) return false; return true; }
    return a == b;
}
const auto kData = test::root() / "data";
// The 300 m scene alone, 450 s: about a third of one full run, so the rows stay quick.
sweep::WeavingRun middle(double factor, bool lateral, std::uint32_t seed = 42) {
    const auto lab = fixture::weavingLab();
    const auto& sc = lab.scenes[1];
    return sweep::runWeaving(lab.document, kData, seed, {factor, lateral, {sc.toMain, sc.toExit}, 450.0});
}
std::uint64_t served(const sweep::WeavingRun& r) { std::uint64_t n = 0; for (const auto& m : r.movements.movements) n += m.vehicles; return n; }
std::uint64_t unfinished(const sweep::WeavingRun& r) { std::uint64_t n = 0; for (const auto& m : r.movements.movements) n += m.unfinished; return n; }
std::uint64_t changes(const sweep::WeavingRun& r) { std::uint64_t n = 0; for (const auto& c : r.changes.rows) n += c.changes; return n; }
}
TEST(weavinglab, committed_file_is_the_builder_output) {
    const auto built = documentJson(fixture::weavingLab().document);
    CHECK(same(committed(), built));
    CHECK(documentJson(parseDocument(committed())) == committed()); // and the file reopens exactly
}
TEST(weavinglab, ba179_each_weave_compiles_to_one_crossing_per_weaving_movement) {
    const auto lab = fixture::weavingLab();
    const auto s = compileDocument(lab.document, kData).scenario;
    CHECK(validateScenario(s).empty());
    CHECK(s.conflictZones.empty()); // a weave is lane changing alone: nothing crosses or merges
    for (const auto& sc : lab.scenes) {
        // The forcing: lane 0 reaches only the exit, lanes 1-2 only the mainline.
        std::map<std::string, double> deadEnd;
        for (const auto& d : s.routeDeadEnds) deadEnd[d.routeId] = d.at;
        const auto main = [&](int k) { return sc.toMain + "/lane-" + std::to_string(k); };
        const auto exit = [&](int k) { return sc.toExit + "/lane-" + std::to_string(k); };
        CHECK(deadEnd.count(main(1)) && !deadEnd.count(main(2)) && !deadEnd.count(main(3)));
        CHECK(!deadEnd.count(exit(1)) && deadEnd.count(exit(2)) && deadEnd.count(exit(3)));
        // Every stub's dead end is the weave's end, so every change is made inside the weave.
        for (const auto& id : {main(1), exit(2), exit(3)}) CHECK(std::abs(deadEnd.at(id) - sc.length) < 1e-6);
        // Mandatory spans only from the stubs, each to a neighbouring lane; toward the full chain,
        // each stub has exactly one.
        std::map<std::string, std::set<std::string>> to;
        for (const auto& c : s.laneChanges) if (deadEnd.count(c.fromRouteId)) to[c.fromRouteId].insert(c.toRouteId);
        CHECK(to.at(main(1)) == std::set<std::string>{main(2)});
        CHECK(to.at(exit(2)).count(exit(1)) == 1);
        CHECK(to.at(exit(3)) == std::set<std::string>{exit(2)});
        // The origin is the entry lane: ramp traffic only on lane 0, mainline only on lanes 1-2.
        std::map<std::string, double> volume;
        for (const auto& i : s.inputs) volume[i.routeId] += i.vehiclesPerHour;
        CHECK(std::abs(volume[main(1)] - fixture::kRampMain) < 1e-6);
        CHECK(std::abs(volume[exit(1)] - fixture::kRampExit) < 1e-6);
        CHECK(std::abs(volume[main(2)] + volume[main(3)] - fixture::kMainMain) < 1e-6);
        CHECK(std::abs(volume[exit(2)] + volume[exit(3)] - fixture::kMainExit) < 1e-6);
    }
}
TEST(weavinglab, ba179_a_weave_with_two_entries_does_not_run) {
    // The limitation the lab's shape works around (M3_8_CONTRACT §2): a change is compiled only on
    // a route's entry Link, so the two crossing movements have no lane path. A fix shows up here.
    const auto two = fixture::weavingTwoEntries();
    const auto issues = routeRuntimeIssues(two.network, *two.definition);
    std::vector<std::string> paths;
    for (const auto& i : issues) if (i.code == "UNSUPPORTED_ROUTE_TOPOLOGY") paths.push_back(i.path);
    CHECK(paths == std::vector<std::string>({"routes[1]", "routes[2]"})); // main-exit and ramp-main
}
TEST(weavinglab, ba180_switched_off_below_breakdown_every_vehicle_is_served_or_counted) {
    const auto r = middle(0.6, false);
    // The forcing: weaving vehicles change lanes.
    CHECK(changes(r) > 0);
    CHECK(r.removed == 0);   // nobody leaves without arriving (M3 §4: no diffusion)
    CHECK(!r.overlap);
    CHECK(r.movements.safetyClamps == 0);
    CHECK(served(r) > 0);
    // What is left at 450 s is only what was still on its way: each vehicle served or counted.
    CHECK(unfinished(r) < served(r) / 5);
}
TEST(weavinglab, ba181_switched_on_below_breakdown_the_same_holds) {
    const auto r = middle(0.6, true);
    CHECK(changes(r) > 0);
    CHECK(r.removed == 0);
    CHECK(!r.overlap);
    CHECK(served(r) > 0);
    CHECK(unfinished(r) < served(r) / 5);
}
TEST(weavinglab, ba182_a_run_replays_exactly) {
    for (const bool lateral : {false, true}) {
        const auto a = middle(0.6, lateral, 43), b = middle(0.6, lateral, 43);
        CHECK(a.movements == b.movements);
        CHECK(a.changes.rows == b.changes.rows);
        CHECK(a.lastArrival == b.lastArrival);
    }
}
TEST(weavinglab, ba183_at_full_demand_the_weave_end_keeps_serving) {
    // D450 (was pinned open by D449): at the fixture's demand the two lane heads at the dead end
    // stood short of the gap each needs in the other's lane, nobody could yield once both stood, and
    // with no removal the weave stopped serving for good. A yielding partner now keeps its standstill
    // distance, and a change never lands in a waiting partner's place.
    const auto lab = fixture::weavingLab();
    const auto r = middle(1.0, false);
    const auto& sc = lab.scenes[1];
    double last = 0;
    for (const auto& id : {sc.toMain, sc.toExit}) if (r.lastArrival.count(id)) last = std::max(last, r.lastArrival.at(id));
    CHECK(r.removed == 0 && !r.overlap);
    CHECK(last >= 450 - 60); // still arriving in the last minute
}
