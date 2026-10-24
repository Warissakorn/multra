#pragma once
// The study-area rows' fixture (STUDY_AREA §15): two F4 junctions on a split Road, with zones and
// demand for the cuts; test values only. Shared by study_area_tests.cpp and study_area_marking_tests.cpp.
#include "test.hpp"
#include "../src/commands/coarse_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/node_commands.hpp"
#include "../src/commands/node_detail_commands.hpp"
#include "../src/commands/road_commands.hpp"
#include "../src/model/network/node_membership.hpp"
#include "../src/model/network/study_area.hpp"
#include "../src/project/evaluation.hpp"
#include "../src/project/json.hpp"
#include "../src/project/run.hpp"
#include "../src/project/study_area_run.hpp"
#include "../src/project/study_area_macro.hpp"
#include "../src/project/study_area_meso.hpp"
#include "../src/project/meso_run.hpp"
#include "../src/project/macro_run.hpp"
#include "../src/project/side_files.hpp"
#include "../src/project/input_manifest.hpp"
#include <memory>
#include <cmath>
#include <map>
#include "../src/project/demand_catalog.hpp"
#include "../src/commands/catalog_commands.hpp"
#include "../src/commands/zone_commands.hpp"
#include "../src/macro/assign.hpp"
#include "../src/runner/batch.hpp"
#include <algorithm>
#include <fstream>
#include <set>
using namespace multra;
namespace studyfix {
inline std::vector<Point> square(Point c, double h) { return {{c.x - h, c.y - h}, {c.x + h, c.y - h}, {c.x + h, c.y + h}, {c.x - h, c.y + h}}; }
inline CrossSection section() { return {"ns", "ns", 2, 1, 1, 3.5, {}, 0, std::nullopt, "Test template"}; } // test values only (D304)
inline const std::vector<CrossSection>& templates(const ProjectDocument& d) { return d.definition->crossSections; }
inline NodeFillCatalogs catalogs(const ProjectDocument& d) { return {templates(d), {}, {3.0, 7.0}}; }
inline const Road& road(const ProjectDocument& d, const std::string& id) {
    return *std::find_if(d.network.roads.begin(), d.network.roads.end(), [&](const auto& r) { return r.id == id; });
}
// Two priority junctions F4 generated: A at the origin (north, south, west arms) and B at (300, 0)
// (north, south, east arms), joined by a Road split at x = 100 and x = 200. The middle piece M2 is a
// leg of neither Node. Run settings from the shipped T-junction project; one period of 600 s.
struct Corridor { ProjectDocument d; std::string a, b, m1, m2, m3, aw, be; };
inline Corridor corridor() {
    std::ifstream file(test::root() / "data/projects/t-junction-priority.traffic.json"); Json j; file >> j;
    auto base = *parseDocument(j).definition;
    base.inputs.clear(); base.routes.clear(); base.routingDecisions.clear(); base.signalPrograms.clear();
    base.crossSections.clear(); base.externalCrossSections = false;
    Corridor c; auto& d = c.d;
    d.definition = base;
    putCrossSection(d, section(), {});
    const auto add = [&](std::vector<Point> line) { const auto id = putRoad(d, {"", "", line, std::string("ns"), 0, {}, {}}); applyTemplate(d, id, templates(d)); return id; };
    const auto an = add({{0, 100}, {0, 0}}), as = add({{0, -100}, {0, 0}}); c.aw = add({{-100, 0}, {0, 0}});
    const auto bn = add({{300, 100}, {300, 0}}), bs = add({{300, -100}, {300, 0}}); c.be = add({{400, 0}, {300, 0}});
    c.m1 = add({{300, 0}, {0, 0}});
    c.m2 = splitRoad(d, c.m1, 100);    // m1: 300 -> 200
    c.m3 = splitRoad(d, c.m2, 100);    // m2: 200 -> 100, m3: 100 -> 0
    c.a = putNode(d, {"", "A", square({0, 0}, 10), false});
    c.b = putNode(d, {"", "B", square({300, 0}, 10), false});
    setNodeCoarse(d, c.a, NodeCoarse{std::nullopt, {an, as}, {}, std::nullopt});
    setNodeCoarse(d, c.b, NodeCoarse{std::nullopt, {bn, bs}, {}, std::nullopt});
    generateNodeDetail(d, c.a, catalogs(d));
    generateNodeDetail(d, c.b, catalogs(d));
    d.zones.studyPeriods = {{"p", "P", 0, 600}};
    return c;
}
inline std::set<std::string> ids(const std::vector<std::string>& v) { return {v.begin(), v.end()}; }
inline bool has(const std::vector<ValidationIssue>& issues, const std::string& code, const std::string& path = {}) {
    return std::any_of(issues.begin(), issues.end(), [&](const auto& i) { return i.code == code && (path.empty() || i.path == path); });
}
inline std::vector<ValidationIssue> refusal(const std::function<void()>& f) {
    try { f(); } catch (const ValidationError& e) { return e.issues; }
    return {};
}
inline std::string saved(const ProjectDocument& d) { return documentJson(d).dump(); }
// MA8's linear two-link pair (test values): 10 (1 + 0.15 x/1000) against 12 (1 + 0.15 x/1000).
inline MacroGraph pair(double beta, std::vector<double> times = {10, 12}) {
    MacroGraph g; g.nodes = {"o", "d"};
    for (const double t0 : times) { MacroLink l; l.from = 0; l.to = 1; l.freeTime = t0; l.capacity = 1000; l.alpha = 0.15; l.beta = beta; g.links.push_back(l); }
    return g;
}
// The corridor with a zone west of A (origin, on A's west arm inbound) and one east of B (destination,
// on B's east arm outbound), `trips` from west to east in period p (0-600 s); owned catalog, PCUs and
// one link type, all test values.
inline Corridor demandCorridor(double trips) {
    auto c = corridor();
    auto& d = c.d;
    putDemandCatalog(d, resolveDemandCatalog(*d.definition, test::root() / "data"));
    for (auto& ty : d.definition->vehicleTypes) ty.pcu = Pcu{1.0, "Test values"};
    d.definition->externalLinkTypes = false;
    d.definition->linkTypes = {{"arterial", "Arterial", 1800, 15, "bpr", {}, "Test values"}};
    for (auto& l : d.network.links) l.linkTypeId = "arterial";
    putZone(d, {"west", "West", square({-110, 0}, 5), {}});
    putZone(d, {"east", "East", square({410, 0}, 5), {}});
    putZoneConnector(d, {"zw", "west", road(d, c.aw).links[0], ZoneDirection::origin, 1});
    putZoneConnector(d, {"ze", "east", road(d, c.be).links[1], ZoneDirection::destination, 1});
    putOdMatrix(d, {"am", 0, 0, "car-only", {"west", "east"}, {{0, trips}, {0, 0}}, "p"});
    return c;
}
inline double total(const std::vector<GateFlow>& flows) { double s = 0; for (const auto& f : flows) s += f.vehiclesPerHour; return s; }
// demandCorridor with a jam density on its type, so Meso loads it (test value, HCM's 118.06 veh/km/ln).
inline Corridor mesoCorridor(double trips) {
    auto c = demandCorridor(trips);
    c.d.definition->linkTypes[0].jamDensity = JamDensity{118.06, "Test values"};
    return c;
}
}
using namespace studyfix;
