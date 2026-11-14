// M12 R1 rows SC1-SC6 (docs/reference/SUBAREA_CONSISTENCY.md §11, D553). Each row first checks its forcing.
#include "study_area_fixture.hpp"
#include "../src/project/subarea_consistency.hpp"
#include "../src/project/meso_output.hpp"
#include "../src/core/simulation.hpp"
#include <iostream>
namespace {
std::filesystem::path data() { return test::root() / "data"; }
// The Meso corridor, 100 trips west to east in period p (0-600 s), area {A} fed by the Macro cut.
Corridor area() {
    auto c = mesoCorridor(100);
    setStudyAreaNodes(c.d, {c.a});
    c.d.zones.studyArea.flowSource = "macro";
    return c;
}
const ConsistencyRow& rowOf(const ConsistencyPeriod& p, const std::string& id) {
    const auto it = std::find_if(p.rows.begin(), p.rows.end(), [&](const auto& r) { return r.id == id; });
    CHECK(it != p.rows.end());
    return *it;
}
}
TEST(consistency, sc1_byte_identity) { // SC1
    auto c = area();
    auto plain = c.d; plain.zones.studyArea = {};
    CHECK(!c.d.zones.studyArea.nodes.empty() && plain.zones.studyArea.nodes.empty()); // forcing: one with, one without
    // runMeso ignores the area: whole-network output equal with and without it.
    CHECK(mesoJson(runMeso(c.d, data())).dump() == mesoJson(runMeso(plain, data())).dump());
    CHECK(has(refusal([&] { runMesoArea(plain, data()); }), "CONSISTENCY_NO_STUDY_AREA", "studyArea"));
}
TEST(consistency, sc2_meso_area_run) { // SC2
    auto c = area();
    StudyAreaProvenance provenance;
    const auto run = runMesoArea(c.d, data(), nullptr, &provenance);
    const auto cut = studyAreaCut(c.d.network, {c.a});
    const std::set<std::string> kept(cut.links.begin(), cut.links.end());
    CHECK(!run.graph.links.empty() && !run.loading.watched.empty()); // forcing
    for (const auto& l : run.graph.links) CHECK(kept.contains(l.source));
    CHECK(provenance.flowSource == "macro" && provenance.projectSha256 == projectSha256(c.d));
    double entered = 0, demanded = 0;
    for (const auto& w : run.loading.watched) if (w.position == 0) entered += w.entrance.at.back() / run.classes[w.cls].pcu;
    for (const auto& o : run.loading.commodities) demanded += o.demand.back();
    std::cerr << "SC2: entered " << entered << " of " << demanded << " demanded\n";
    test::near(demanded, 100, 1e-9);
    test::near(entered, demanded, 1e-9);
    auto self = c.d; self.zones.studyArea.flowSource = "meso";
    CHECK(has(refusal([&] { runMesoArea(self, data()); }), "MESO_AREA_SOURCE_SELF", "studyArea.flowSource"));
}
TEST(consistency, sc3_to_sc5_columns) { // SC3, SC4, SC5
    auto c = area();
    const std::vector<std::uint32_t> seeds{42, 43};
    const auto report = consistencyReport(c.d, data(), {}, seeds, "corridor.traffic.json");
    CHECK(report.periods.size() == 1 && !report.periods[0].rows.empty()); // forcing: one period, rows
    const auto& p = report.periods[0];
    // SC3: Macro, read by hand from the assignment: each kept Link's first `link` arc.
    const auto macro = runMacro(c.d, data(), "p");
    for (const auto& r : p.rows) {
        if (r.kind != ConsistencyKind::link) continue;
        std::optional<std::size_t> a;
        for (std::size_t i = 0; i < macro.graph.links.size(); ++i) {
            const auto& x = macro.graph.links[i];
            if (x.kind == MacroArc::link && x.source == r.id && (!a || x.fromStation < macro.graph.links[*a].fromStation)) a = i;
        }
        double v = 0; if (a) for (const auto& cls : macro.assignment.classFlows) v += cls[*a];
        test::near(*r.macro, v, 1e-9);
    }
    const auto outbound = road(c.d, c.m3).links[1];
    const auto& exit = rowOf(p, exitGateName(outbound));
    test::near(*exit.macro, 600, 1e-6);
    test::near(*rowOf(p, outbound).macro, 600, 1e-6);
    // SC4: Meso from the aggregate entrance curve (PCU 1), and within q T_ff / (e - s) of Macro.
    const auto meso = runMesoArea(c.d, data());
    double tff = 0;
    for (const auto& path : meso.classes[0].paths) for (const auto l : path.links) tff += meso.graph.links[l].length / meso.graph.links[l].freeSpeed;
    CHECK(tff > 0);
    for (const auto& r : p.rows) {
        if (r.kind != ConsistencyKind::link) continue;
        std::optional<std::size_t> first;
        for (std::size_t l = 0; l < meso.graph.links.size(); ++l)
            if (meso.graph.links[l].source == r.id && (!first || meso.graph.links[l].fromStation < meso.graph.links[*first].fromStation)) first = l;
        const double v = first ? (meso.loading.up[*first].value(600) - meso.loading.up[*first].value(0)) * 6 : 0.0;
        test::near(*r.meso, v, 1e-9);
        CHECK(std::abs(*r.meso - *r.macro) <= 600 * tff / 600 + 1e-6);
    }
    std::cerr << "SC4: outbound Macro " << *rowOf(p, outbound).macro << ", Meso " << *rowOf(p, outbound).meso << ", bound " << tff << '\n';
    // SC5: Micro recounted from the raw events with an independent lane-to-Link map.
    const auto snapshot = compileDocument(c.d, data());
    std::map<std::string, std::string> linkOfLane;
    for (const auto& l : snapshot.network.links) for (const auto& lane : l.lanes) linkOfLane[lane.id] = l.id;
    const auto linkOf = [&](const std::string& section) {
        const auto lane = section.substr(0, section.find("/sec-"));
        const auto it = linkOfLane.find(lane);
        return it == linkOfLane.end() ? std::string{} : it->second;
    };
    std::map<std::string, std::vector<std::optional<double>>> perSeed;
    for (const auto seed : seeds) {
        std::map<std::uint64_t, std::vector<std::pair<std::string, double>>> visits; // per vehicle, Link and time, consecutive repeats dropped
        std::map<std::string, double> counted;
        auto state = createSimulation(snapshot.scenario, seed);
        while (state.tick < totalTicks(snapshot.scenario)) {
            state = stepSimulation(std::move(state));
            for (const auto& e : state.events) {
                std::string link; std::uint64_t v{}; double t{};
                if (const auto* d = std::get_if<DepartedEvent>(&e)) {
                    for (const auto& r : snapshot.scenario.routes) if (r.id == d->routeId) link = linkOf(r.segmentIds.front());
                    v = d->vehicleId; t = d->time;
                    visits[v].push_back({link, t});
                } else if (const auto* m = std::get_if<SegmentEnteredEvent>(&e)) {
                    visits[m->vehicleId].push_back({linkOf(m->segmentId), m->time});
                } else if (const auto* a = std::get_if<ArrivedEvent>(&e); a && a->time < 600) {
                    for (const auto& r : snapshot.scenario.routes) if (r.id == a->routeId) counted[exitGateName(linkOf(r.segmentIds.back()))] += 6;
                }
            }
        }
        for (const auto& [v, list] : visits)
            for (std::size_t i = 0; i < list.size(); ++i)
                if (!list[i].first.empty() && (i == 0 || list[i].first != list[i - 1].first) && list[i].second < 600) counted[list[i].first] += 6;
        CHECK(counted[outbound] > 0); // forcing: vehicles reached A's outbound leg
        for (const auto& r : p.rows) {
            const auto id = r.kind == ConsistencyKind::entryGate ? r.id.substr(5, r.id.size() - 8) : r.id; // gate:<link>:in -> <link>
            perSeed[r.id].push_back(counted.contains(id) ? counted[id] : 0.0);
        }
    }
    for (const auto& r : p.rows) {
        const auto e = estimate(perSeed[r.id]);
        CHECK(r.micro.n == e.n);
        test::near(*r.micro.mean, *e.mean, 1e-9);
        test::near(*r.micro.halfWidth95, *e.halfWidth95, 1e-9);
    }
    std::cerr << "SC5: outbound Micro mean " << *rowOf(p, outbound).micro.mean << '\n';
    // The report twice gives the same bytes.
    CHECK(consistencyJson(report).dump() == consistencyJson(consistencyReport(c.d, data(), {}, seeds, "corridor.traffic.json")).dump());
    CHECK(consistencyCsv(report).rfind("# MULTRA - not yet validated.", 0) == 0);
}
TEST(consistency, sc6_arithmetic) { // SC6
    const auto c = compareVolumes(400.0, 360.0);
    CHECK(c.delta && c.percent && c.geh); // forcing
    test::near(*c.delta, -40, 1e-12); test::near(*c.percent, -10, 1e-12); test::near(*c.geh, 2.0519567041703082, 1e-12);
    const auto zero = compareVolumes(0.0, 5.0);
    CHECK(!zero.percent && zero.geh && *zero.delta == 5);
    const auto none = compareVolumes(0.0, 0.0);
    CHECK(!none.percent && !none.geh && *none.delta == 0);
    CHECK(!compareVolumes(std::nullopt, 3.0).delta);
}
TEST(consistency, sc11_gate_counts_inside_zones) { // SC11 (§15, D555)
    auto c = area();
    const auto cut = studyAreaCut(c.d.network, {c.a});
    const auto west = road(c.d, c.aw).links[0];
    CHECK(std::find(cut.entryGates.begin(), cut.entryGates.end(), west) != cut.entryGates.end()); // forcing: zone west's Link is a gate
    const std::vector<std::uint32_t> seeds{42};
    const auto report = consistencyReport(c.d, data(), {}, seeds, "");
    const auto& gate = rowOf(report.periods[0], entryGateName(west));
    const auto& link = rowOf(report.periods[0], west);
    test::near(*gate.macro, 600, 1e-6);
    test::near(*gate.macro, *link.macro, 1e-9);
    test::near(*gate.comparisons[0].geh, 0, 1e-9);
    test::near(*rowOf(report.periods[0], exitGateName(road(c.d, c.m3).links[1])).macro, 600, 1e-6); // no inside zone: unchanged
    // A second origin connector for west on another entry-gate Link, shares 3 : 1: 600 splits 450 and 150 (by hand).
    std::string north;
    for (const auto& g : cut.entryGates) if (g != west) { north = g; break; }
    CHECK(!north.empty());
    for (auto& zc : c.d.zones.zoneConnectors) if (zc.id == "zw") zc.share = 3;
    putZoneConnector(c.d, {"zw2", "west", north, ZoneDirection::origin, 1});
    const auto split = consistencyReport(c.d, data(), {}, seeds, "");
    test::near(*rowOf(split.periods[0], entryGateName(west)).macro, 450, 1e-6);
    test::near(*rowOf(split.periods[0], entryGateName(north)).macro, 150, 1e-6);
}
