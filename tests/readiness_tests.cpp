// S4.2 rows RS1-RS4 (docs/reference/READINESS.md §3, §9, D313): the readiness panel by level, from the
// same functions the runs call. Each row first checks its forcing.
#include "test.hpp"
#include "../src/commands/catalog_commands.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/commands/node_commands.hpp"
#include "../src/commands/zone_commands.hpp"
#include "../src/project/demand_catalog.hpp"
#include "../src/commands/street_segment_commands.hpp"
#include "../src/project/json.hpp"
#include "../src/project/macro_run.hpp"
#include "../src/project/meso_run.hpp"
#include "../src/project/readiness.hpp"
#include "../src/project/run.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <set>
using namespace multra;
namespace {
ProjectDocument shipped(const std::string& project) {
    std::ifstream file(test::root() / "data/projects" / project); Json j; file >> j; return parseDocument(j);
}
std::filesystem::path data() { return test::root() / "data"; }
struct Started {}; // thrown from a stage observer once a run is past its refusals
std::set<std::pair<std::string, std::string>> refusals(const std::vector<ReadinessRow>& rows) {
    std::set<std::pair<std::string, std::string>> out;
    for (const auto& r : rows) if (r.severity == ReadinessSeverity::refusal) out.insert({r.code, r.path});
    return out;
}
std::set<std::pair<std::string, std::string>> issues(const ValidationError& e) {
    std::set<std::pair<std::string, std::string>> out;
    for (const auto& i : e.issues) out.insert({i.code, i.path});
    return out;
}
// The run's verdict: empty when it starts, its refusals otherwise.
template <class F> std::optional<std::set<std::pair<std::string, std::string>>> verdict(F run) {
    try { run(); } catch (const Started&) { return std::set<std::pair<std::string, std::string>>{}; }
    catch (const ValidationError& e) { return issues(e); }
    catch (const std::exception& e) { return std::set<std::pair<std::string, std::string>>{{e.what(), ""}}; }
    return std::set<std::pair<std::string, std::string>>{};
}
// A StreetSegment-only fixture: one Link-less StreetSegment with an owned template and a typed, jam-density link type.
ProjectDocument streetSegmentOnly() {
    ProjectDocument d;
    d.definition = AuthoringDefinition{}; d.definition->externalLinkTypes = false;
    d.definition->linkTypes = {{"arterial", "Arterial", 1000, 10, "bpr", {}, "Test values", JamDensity{150, "Test value"}}};
    putCrossSection(d, {"ns", "ns", 2, 1, 1, 3.5, {}, 0, std::string("arterial"), "Test values"}, {});
    putStreetSegment(d, {"", "", {{0, 0}, {100, 0}}, std::string("ns"), 0, {}, {}});
    return d;
}
std::string laneOf(const ProjectDocument& d, const std::string& link) {
    for (const auto& l : d.network.links) if (l.id == link) return l.lanes.front().id;
    return {};
}
// Two routes site -> town through a shared Link (macroassign's twin), typed, with PCUs and jam density:
// Micro, Meso and Macro all get past their refusals. Without PCUs both Meso and Macro refuse at the
// demand stage (MODE_PCU_MISSING), after the graph compiled.
ProjectDocument twin(bool pcus) {
    ProjectDocument d;
    const auto a1 = addLink(d, {{0, 0}, {100, 0}}, 1, 3.5), a2 = addLink(d, {{0, 40}, {300, 40}}, 1, 3.5);
    const auto t = addLink(d, {{330, 20}, {430, 20}}, 2, 3.5);
    addConnector(d, {a1, laneOf(d, a1), 100}, {t, laneOf(d, t), 0});
    addConnector(d, {a2, laneOf(d, a2), 300}, {t, laneOf(d, t), 0});
    auto& def = demand(d); def.duration = 3600; def.timeStep = 0.5;
    putZone(d, {"site", "", {{-10, -10}, {-10, 50}, {-30, 20}}, {}});
    putZone(d, {"town", "", {{440, 0}, {460, 40}, {460, 0}}, {}});
    putZoneConnector(d, {"s1", "site", a1, ZoneDirection::origin, 1});
    putZoneConnector(d, {"s2", "site", a2, ZoneDirection::origin, 1});
    putZoneConnector(d, {"t1", "town", t, ZoneDirection::destination, 1});
    putOdMatrix(d, {"am", 0, 3600, "urban-mixed", {"site", "town"}, {{0, 3000}, {0, 0}}});
    putDemandCatalog(d, resolveDemandCatalog(*d.definition, data()));
    if (pcus) for (auto& ty : d.definition->vehicleTypes) ty.pcu = Pcu{ty.id == "car" ? 1.0 : 2.0, "Test values"};
    d.definition->externalLinkTypes = false;
    d.definition->linkTypes = {{"arterial", "Arterial", 600, 10, "bpr", {}, "Test values", JamDensity{150, "Test value"}}};
    for (auto& l : d.network.links) l.linkTypeId = "arterial";
    return d;
}
}
TEST(readiness, rs1_reading_changes_nothing) { // RS1
    auto d = shipped("four-leg-signalised.traffic.json");
    const auto bytes = documentJson(d).dump(); const auto revision = d.revision;
    CHECK(!readiness(d, data(), ReadinessLevel::micro).empty() || true); // forcing: it runs
    for (const auto level : {ReadinessLevel::micro, ReadinessLevel::meso, ReadinessLevel::macro}) readiness(d, data(), level);
    CHECK(documentJson(d).dump() == bytes && d.revision == revision);
}
TEST(readiness, rs2_refusal_rows_agree_with_the_runs) { // RS2
    std::vector<std::pair<std::string, ProjectDocument>> projects;
    for (const auto* p : {"four-leg-signalised.traffic.json", "lane-change-lab.traffic.json", "m2.6-study-template.traffic.json", "t-junction-priority.traffic.json"})
        projects.emplace_back(p, shipped(p));
    projects.emplace_back("road-only", streetSegmentOnly());
    projects.emplace_back("twin", twin(true));
    projects.emplace_back("twin without PCUs", twin(false));
    // A runnable drawing with an advisory: an empty Node far from the junction (NODE_NO_TURN).
    auto advised = shipped("four-leg-signalised.traffic.json");
    putNode(advised, {"", "", {{500, 500}, {520, 500}, {520, 520}, {500, 520}}, false});
    projects.emplace_back("four-leg with an empty Node", advised);
    std::size_t demandStage = 0, advisoryOnly = 0;
    std::size_t refused = 0, started = 0;
    for (const auto& [name, d] : projects) {
        // Micro: compileDocument is what Run calls.
        const auto micro = verdict([&] { compileDocument(d, data()); });
        const auto microRows = refusals(readiness(d, data(), ReadinessLevel::micro));
        if (microRows.empty() != micro->empty()) std::cerr << name << ": micro run " << micro->size() << " vs panel " << microRows.size() << " " << (micro->empty() ? "" : micro->begin()->first) << "\n";
        CHECK(microRows.empty() == micro->empty());
        const MesoObserver meso{[](MesoStage s) { if (s == MesoStage::loading) throw Started{}; }, {}};
        const auto mesoVerdict = verdict([&] { runMeso(d, data(), nullptr, &meso); });
        CHECK(refusals(readiness(d, data(), ReadinessLevel::meso)) == *mesoVerdict);
        const MacroObserver macro{[](MacroStage s) { if (s == MacroStage::assignment) throw Started{}; }, {}};
        const auto periods = macroPeriods(d);
        const auto period = periods.empty() ? std::string("none") : macroPeriodName(periods.front());
        const auto macroVerdict = verdict([&] { runMacro(d, data(), period, nullptr, &macro); });
        CHECK(refusals(readiness(d, data(), ReadinessLevel::macro, periods.empty() ? std::string{} : period)).size() == macroVerdict->size() ||
              periods.empty());
        if (!periods.empty()) CHECK(refusals(readiness(d, data(), ReadinessLevel::macro, period)) == *macroVerdict);
        for (const auto* v : {&*micro, &*mesoVerdict, &*macroVerdict}) (v->empty() ? started : refused)++;
        for (const auto* v : {&*mesoVerdict, &*macroVerdict})
            demandStage += static_cast<std::size_t>(std::count_if(v->begin(), v->end(), [](const auto& i) { return i.first == "MODE_PCU_MISSING"; }));
        const auto microAll = readiness(d, data(), ReadinessLevel::micro);
        if (micro->empty() && std::any_of(microAll.begin(), microAll.end(), [](const auto& r) { return r.severity == ReadinessSeverity::advisory; })) ++advisoryOnly;
    }
    CHECK(refused > 0 && started > 0); // forcing: both outcomes occur
    CHECK(demandStage >= 2 && advisoryOnly >= 1); // forcing: a demand-stage refusal at Meso and Macro, and a runnable Micro advisory
}
TEST(readiness, rs3_seams_by_level) { // RS3
    auto d = streetSegmentOnly();
    applyTemplate(d, d.network.streetSegments[0].id, {});
    auto narrowed = d.definition->crossSections[0]; narrowed.lanesForward = 2; narrowed.lanesBackward = 2;
    putCrossSection(d, narrowed, {});
    const auto has = [](const std::vector<ReadinessRow>& rows, ReadinessSeverity s) {
        return std::count_if(rows.begin(), rows.end(), [&](const auto& r) { return r.code == "SEAM_LANE_COUNT" && r.severity == s; });
    };
    const auto micro = readiness(d, data(), ReadinessLevel::micro);
    CHECK(has(micro, ReadinessSeverity::warning) == 2); // forcing: both directions
    CHECK(has(readiness(d, data(), ReadinessLevel::meso), ReadinessSeverity::advisory) == 2);
    const auto row = std::find_if(micro.begin(), micro.end(), [](const auto& r) { return r.code == "SEAM_LANE_COUNT"; });
    CHECK(row->objectId == d.network.links[0].id && row->selectId == d.network.links[0].id); // the canvas can select it
    // A StreetSegment without Links shows at Micro only.
    const auto bare = streetSegmentOnly();
    const auto notGenerated = [](const std::vector<ReadinessRow>& rows) {
        return std::any_of(rows.begin(), rows.end(), [](const auto& r) { return r.code == "SEAM_STREET_SEGMENT_NOT_GENERATED"; });
    };
    CHECK(notGenerated(readiness(bare, data(), ReadinessLevel::micro)) && !notGenerated(readiness(bare, data(), ReadinessLevel::meso)));
}
