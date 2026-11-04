#include "test.hpp"
#include "../tools/lateral_lab_network.hpp"
#include "../tools/weaving_sweep.hpp"
#include <fstream>
using namespace multra;
// D457: the lateral lab (tools/lateral_lab_network.hpp; docs/reference/LATERAL_LAB.md, rows
// BA198-BA203). Four scenes, the one shipped project whose behaviours carry the lateral keys.
namespace {
const auto kFile = "data/projects/lateral-lab.traffic.json";
const auto kData = test::root() / "data";
Json committed() { std::ifstream f(test::root() / kFile); Json j; f >> j; return j; }
// Structure and strings exactly, numbers to 1e-9 m (as `weavinglab` compares its file).
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
// One scene alone (its authored routes' inputs), for `seconds`, from the committed file.
struct SceneRun {
    std::uint64_t departed{}, arrived{}, clamps{}, changes{}, lateralChanges{}, removed{};
    bool overlap{}, straddled{};
    double lastArrival{};
    std::map<std::string, std::pair<double, double>> kerbTime; // type id -> (kerb-lane, all) vehicle-seconds
};
SceneRun run(ProjectDocument document, const std::vector<std::string>& routes, double seconds, std::uint32_t seed = 42,
             bool bias = true) {
    if (!bias)
        for (auto& b : document.definition->behaviours) if (b.id == fixture::kKeepLeftBehaviour) b.kerbLaneBias.reset();
    auto snapshot = compileDocument(document, kData);
    auto& inputs = snapshot.scenario.inputs;
    std::erase_if(inputs, [&](const auto& i) { return std::find(routes.begin(), routes.end(), sweep::authoredRoute(i.routeId)) == routes.end(); });
    snapshot.scenario.duration = seconds;
    for (auto& i : inputs) i.endTime = std::min(i.endTime, seconds);
    SceneRun r;
    auto s = createSimulation(snapshot.scenario, seed);
    const double dt = snapshot.scenario.timeStep;
    while (s.tick < totalTicks(snapshot.scenario)) {
        const auto before = s.vehicles.size();
        s = stepSimulation(std::move(s));
        std::uint64_t in = 0, out = 0;
        for (const auto& e : s.events) {
            if (std::holds_alternative<DepartedEvent>(e)) ++in;
            if (const auto* a = std::get_if<ArrivedEvent>(&e)) { ++out; r.lastArrival = a->time; }
            if (std::holds_alternative<SafetyClampEvent>(e)) ++r.clamps;
            if (std::holds_alternative<LaneChangeEvent>(e)) ++r.changes;
        }
        r.departed += in; r.arrived += out;
        if (s.vehicles.size() + out != before + in) ++r.removed;
        r.overlap = r.overlap || sweep::overlaps2d(s);
        const auto refs = resolveRefs(*s.scenario, s.vehicles, *s.index);
        for (const auto& span : occupiedSpans(*s.scenario, s.vehicles, *s.index, refs)) r.straddled = r.straddled || !span.own;
        for (const auto& v : s.vehicles) {
            if (v.lateralTarget) ++r.lateralChanges; // vehicle-ticks with a crossing under way
            const auto& type = s.scenario->vehicleTypes[v.typeIndex].id;
            const auto& route = s.scenario->routes[v.routeIndex].id;
            auto& [kerb, all] = r.kerbTime[type];
            all += dt;
            if (route.ends_with("/lane-1")) kerb += dt; // lane 0, the kerb lane, is the compiled lane-1 route
        }
    }
    return r;
}
}
TEST(laterallab, committed_file_is_the_builder_output) { // BA198
    const auto built = documentJson(fixture::lateralLab(kData).document);
    CHECK(same(committed(), built));
    CHECK(documentJson(parseDocument(committed())) == committed()); // and the file reopens exactly
}
TEST(laterallab, only_the_lab_is_switched_on) { // BA199
    CHECK(compileDocument(parseDocument(committed()), kData).scenario.behaviours.front().lateral == fixture::labLateralKeys());
    const auto on = [](const char* file) {
        std::ifstream f(test::root() / "data/projects" / file); Json j; f >> j;
        const auto snapshot = compileDocument(parseDocument(j), kData);
        return createSimulation(snapshot.scenario, 42).index->lateral.on;
    };
    CHECK(on("lateral-lab.traffic.json"));
    for (const char* p : {"four-leg-signalised.traffic.json", "t-junction-priority.traffic.json", "m2.6-study-template.traffic.json",
                          "lane-change-lab.traffic.json", "weaving-lab.traffic.json"})
        CHECK(!on(p));
}
TEST(laterallab, a_timed_change_straddles_and_never_overlaps) { // BA200
    const auto lab = fixture::lateralLab(kData);
    const auto r = run(lab.document, {lab.exitARoute, lab.exitBRoute}, 300);
    CHECK(r.changes > 0 && r.lateralChanges > 0 && r.straddled);
    CHECK(!r.overlap && r.removed == 0 && r.clamps == 0);
}
TEST(laterallab, the_lane_drop_serves_without_a_lock) { // BA201
    const auto lab = fixture::lateralLab(kData);
    const auto r = run(lab.document, {lab.dropRoute}, 300);
    CHECK(r.changes > 0 && r.arrived > 0 && r.lastArrival > 240); // arrivals into the last minute
    CHECK(!r.overlap && r.removed == 0 && r.clamps == 0);
}
TEST(laterallab, the_short_weave_serves_without_a_lock) { // BA202
    const auto lab = fixture::lateralLab(kData);
    const auto r = run(lab.document, {lab.weaveMain, lab.weaveExit}, 300);
    CHECK(r.changes > 0 && r.arrived > 0 && r.lastArrival > 240);
    CHECK(!r.overlap && r.removed == 0 && r.clamps == 0);
}
TEST(laterallab, keep_left_holds_the_kerb_lane_more_with_the_bias) { // BA203
    const auto lab = fixture::lateralLab(kData);
    const auto with = run(lab.document, {lab.keepLeftRoute}, 300);
    const auto without = run(lab.document, {lab.keepLeftRoute}, 300, 42, false);
    const auto share = [](const SceneRun& r, const char* type) {
        const auto& [kerb, all] = r.kerbTime.at(type);
        return kerb / all;
    };
    for (const char* type : {"car", "heavy-vehicle", "motorcycle"}) CHECK(share(with, type) > share(without, type));
    CHECK(!with.overlap && with.removed == 0 && with.clamps == 0);
}
