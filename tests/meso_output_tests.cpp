// M11.4 row ME10 (docs/reference/MESO_NETWORK.md §7, §13, D300): a Meso run of the whole Micro run and
// its outputs by slice, JSON and CSV. Each check first confirms its forcing.
#include "test.hpp"
#include "../src/project/input_manifest.hpp"
#include "../src/project/meso_output.hpp"
#include <fstream>
using namespace multra;
namespace {
ProjectDocument fixture(const char* name) {
    std::ifstream file(test::root() / "tests" / name); Json j; file >> j;
    return parseDocument(j);
}
std::vector<std::string> lines(const std::string& text) {
    std::vector<std::string> out; std::string line;
    for (const char c : text) { if (c == '\n') { out.push_back(line); line.clear(); } else line += c; }
    return out;
}
MesoLink link(const std::string& id, std::size_t from, std::size_t to, double metres, double lanes, double capacityPerLane, double kmh, double jamPerLane) {
    return {id, 0, metres, from, to, metres, lanes, capacityPerLane, kmh / 3.6, jamPerLane, {}};
}
// A hand-built run: links in a chain joined by turns, one path over them at `vph` until `until`.
MesoRun chain(std::vector<MesoLink> links, double vph, double until, double step, double slice, double duration) {
    MesoRun r;
    r.settings = {step, slice}; r.duration = duration;
    r.graph.links = std::move(links);
    r.graph.nodes.resize(r.graph.links.size() + 1);
    MesoPath p{"p", {}, {}, {{0, until, vph}}};
    for (std::size_t i = 0; i < r.graph.links.size(); ++i) {
        p.links.push_back(i);
        if (i + 1 < r.graph.links.size()) { p.turns.push_back(r.graph.turns.size()); r.graph.turns.push_back({i + 1, i, i + 1, "t", 0, {}}); }
    }
    r.classes = {{"type:car", 1, {p}}};
    r.loading = loadMeso(r.graph, r.classes, step, duration);
    return r;
}
}
TEST(mesooutput, me10_a_run_and_its_outputs) { // ME10
    const auto d = fixture("meso-two-route.traffic.json");
    std::vector<MesoStage> stages;
    const MesoObserver observer{[&](MesoStage s) { stages.push_back(s); }, {}};
    InputManifest inputs;
    const auto r = runMeso(d, test::root() / "data", &inputs, &observer);
    CHECK(stages == (std::vector<MesoStage>{MesoStage::graph, MesoStage::demand, MesoStage::loading}));
    CHECK(!r.classes.empty() && r.duration == 3600); // forcing: the matrix loads over the whole run
    const auto j = mesoJson(r);
    for (const char* key : {"validated", "measure", "revision", "settings", "duration", "cancelled", "advisories", "classes", "totals", "links", "turns", "inputManifest"})
        CHECK(j.contains(key));
    CHECK(j["validated"] == false && j["cancelled"] == false && !j["inputManifest"]["files"].empty());
    for (const char* key : {"vkt", "vht", "delay", "storedAtEnd", "inNodesAtEnd"}) CHECK(j["totals"].contains(key));
    const auto csv = lines(mesoCsv(r));
    CHECK(csv.at(0).rfind("# MULTRA - not yet validated.", 0) == 0);
    const auto slices = static_cast<std::size_t>(3600 / r.settings.slice);
    CHECK(csv.size() == 3 + r.graph.links.size() * slices);
    double demanded = 0, entered = 0, arrived = 0, waiting = 0;
    for (const auto& c : j["classes"]) {
        demanded += c["demanded"].template get<double>(); entered += c["entered"].template get<double>(); arrived += c["arrived"].template get<double>(); waiting += c["waiting"].template get<double>();
    }
    CHECK(demanded > 1000); // forcing: the matrix's 1200 trips
    test::near(demanded, entered + waiting, 1e-6);
    // Per link: the slices' inflows sum to its entrance count; PCU entered = arrived + stored on links
    // + inside nodes (D301).
    double storedPcu = 0, insidePcu = 0, enteredPcu = 0, arrivedPcu = 0;
    for (std::size_t l = 0; l < r.graph.links.size(); ++l) {
        double sum = 0;
        for (const auto& x : mesoLinkSlices(r, l)) sum += x.inflow * (x.end - x.start) / 3600;
        test::near(sum, r.loading.up[l].at.back(), 1e-6);
        storedPcu += r.loading.up[l].at.back() - r.loading.down[l].at.back();
        insidePcu += r.loading.admitted[l].at.back() - r.loading.up[l].at.back();
    }
    CHECK(std::any_of(r.graph.turns.begin(), r.graph.turns.end(), [](const auto& t) { return t.freeTime > 0; })); // forcing: timed turns
    for (const auto& x : r.loading.commodities) {
        enteredPcu += x.entered.back() * r.classes[x.cls].pcu; arrivedPcu += x.arrived.back() * r.classes[x.cls].pcu;
    }
    test::near(enteredPcu, arrivedPcu + storedPcu + insidePcu, 1e-6);
    test::near(j["totals"]["storedAtEnd"].template get<double>(), storedPcu, 1e-6);
    test::near(j["totals"]["inNodesAtEnd"].template get<double>(), insidePcu, 1e-6);
}
TEST(mesooutput, me10_slices_flag_spill_back_and_time_travel) { // ME10
    // Yperman's Example 4.5 as a run, 10-minute slices: R = 0 from the step at 40 min.
    auto blocked = chain({link("a", 0, 1, 10000, 1, 3600, 120, 225), link("x", 1, 2, 1000, 1, 0, 120, 225)}, 3600, 7200, 300, 600, 7200);
    const auto s = mesoLinkSlices(blocked, 0);
    CHECK(s.size() == 12 && blocked.loading.receiving[0][7] > 0 && blocked.loading.receiving[0][8] == 0); // forcing
    for (std::size_t i = 0; i < s.size(); ++i) CHECK(s[i].spillBack == (i >= 4)); // slice 4 = [40, 50) min
    CHECK(!s[0].travelTime); // nothing leaves: no travel time
    // A free link (ME3): 50 s in every full slice, no delay; a bottleneck (ME4): delay.
    auto free = chain({link("a", 0, 1, 1000, 1, 1800, 72, 150)}, 900, 600, 1, 300, 900);
    const auto f = mesoLinkSlices(free, 0);
    CHECK(f.size() == 3 && f[0].inflow == 900); // forcing
    test::near(*f[0].travelTime, 50, 1e-6); test::near(*f[1].travelTime, 50, 1e-6);
    CHECK(f[0].queue < 1e-9);
    test::near(mesoJson(free)["totals"]["delay"].template get<double>(), 0, 1e-9);
    // A free chain whose turn takes 20 s: no delay, and VHT is the free time with the turn's.
    auto timed = chain({link("a", 0, 1, 1000, 1, 1800, 72, 150), link("b", 1, 2, 1000, 1, 1800, 72, 150)}, 900, 600, 1, 300, 900);
    timed.graph.turns[0].freeTime = 20;
    timed.loading = loadMeso(timed.graph, timed.classes, 1, 900);
    const auto tj = mesoJson(timed);
    CHECK(tj["turns"][0]["freeTime"] == 20 && timed.loading.commodities[0].arrived.back() > 149); // forcing
    test::near(tj["totals"]["delay"].template get<double>(), 0, 1e-6);
    test::near(tj["totals"]["vht"].template get<double>(), 150 * (50 + 20 + 50) / 3600.0, 1e-6);
    auto drop = chain({link("a", 0, 1, 3000, 2, 1800, 72, 150), link("b", 1, 2, 1000, 1, 1800, 72, 150)}, 2700, 1200, 1, 300, 3600);
    const auto q = mesoLinkSlices(drop, 0);
    CHECK(q[3].queue > 100 && *q[3].travelTime > 150 + 60); // the queue at the lane drop
    CHECK(mesoJson(drop)["totals"]["delay"].template get<double>() > 10);
}
TEST(mesooutput, me10_cancel_refusal_and_advisories) { // ME10
    const auto d = fixture("meso-two-route.traffic.json");
    const MesoObserver stop{{}, [](std::size_t k, std::size_t) { return k < 10; }};
    const auto r = runMeso(d, test::root() / "data", nullptr, &stop);
    CHECK(r.loading.cancelled && r.loading.up.front().at.size() == 11);
    // Without a jam density the graph refuses.
    auto bare = fixture("macro-two-route.traffic.json");
    MesoStage last{};
    const MesoObserver seen{[&](MesoStage s) { last = s; }, {}};
    bool refused = false;
    try { runMeso(bare, test::root() / "data", nullptr, &seen); } catch (const ValidationError& e) {
        refused = std::any_of(e.issues.begin(), e.issues.end(), [](const auto& i) { return i.code == "MESO_JAM_DENSITY_MISSING"; });
    }
    CHECK(refused && last == MesoStage::graph);
    // A refused path is advised.
    auto run = runMeso(d, test::root() / "data");
    CHECK(run.loading.refused.empty()); // forcing: none refused on the fixture
    run.classes.front().pcu = 0;
    run.loading = loadMeso(run.graph, run.classes, run.settings.step, run.duration);
    CHECK(!run.loading.refused.empty());
}
