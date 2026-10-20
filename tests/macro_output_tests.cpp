// S2.3 rows MA12, MA13 (docs/reference/MACRO_NETWORK.md §5, §7, §9, D275): a Macro run's settings,
// its VDF read from data, and its link outputs in JSON and CSV. Each row first checks its forcing.
#include "test.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/catalog_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/commands/zone_commands.hpp"
#include "../src/project/macro_output.hpp"
#include "../src/project/macro_run.hpp"
#include "../src/project/csv_format.hpp"
#include "../src/project/input_manifest.hpp"
#include <cmath>
#include <fstream>
#include <sstream>
using namespace trafficsim;
namespace {
std::filesystem::path data() { return test::root() / "data"; }
std::string laneOf(const ProjectDocument& d, const std::string& link) {
    for (const auto& l : d.network.links) if (l.id == link) return l.lanes.front().id;
    throw std::runtime_error("no link " + link);
}
bool refusedAt(const Json& j, const std::string& code, const std::string& path) {
    try { parseDocument(j); } catch (const ValidationError& e) {
        return std::any_of(e.issues.begin(), e.issues.end(), [&](const auto& i) { return i.code == code && i.path == path; });
    } catch (...) {}
    return false;
}
// One origin Link fans out to two (or, with `third`, three) routes that join a terminal Link, so the
// route choice is Frank-Wolfe's, not the zone connectors' shares. Types and PCUs owned.
struct Twin { ProjectDocument d; std::vector<std::string> routes, in, out; };
Twin twin(bool third = false) {
    Twin w; auto& d = w.d;
    const auto s = addLink(d, {{-100, 40}, {-50, 40}}, 5, 3.5), t = addLink(d, {{330, 20}, {430, 20}}, 5, 3.5);
    std::vector<double> lengths{100, 300};
    if (third) lengths.push_back(200);
    for (std::size_t k = 0; k < lengths.size(); ++k) {
        const auto r = addLink(d, {{0, 40.0 * k}, {lengths[k], 40.0 * k}}, 1, 3.5);
        const double end = lengths[k];
        w.routes.push_back(r);
        w.in.push_back(addConnector(d, {s, laneOf(d, s), 50}, {r, laneOf(d, r), 0}));
        w.out.push_back(addConnector(d, {r, laneOf(d, r), end}, {t, laneOf(d, t), 0}));
    }
    auto& def = demand(d); def.duration = 3600; def.timeStep = 0.5;
    putZone(d, {"site", "", {{-110, 30}, {-110, 50}, {-130, 40}}, {}});
    putZone(d, {"town", "", {{440, 0}, {460, 40}, {460, 0}}, {}});
    putZoneConnector(d, {"s1", "site", s, ZoneDirection::origin, 1});
    putZoneConnector(d, {"t1", "town", t, ZoneDirection::destination, 1});
    putOdMatrix(d, {"am", 0, 3600, "urban-mixed", {"site", "town"}, {{0, 1200}, {0, 0}}});
    putDemandCatalog(d, resolveDemandCatalog(*d.definition, data()));
    for (auto& ty : d.definition->vehicleTypes) ty.pcu = Pcu{ty.id == "car" ? 1.0 : 2.0, "Test values"};
    d.definition->externalLinkTypes = false;
    d.definition->linkTypes = {{"arterial", "Arterial", 600, 10, "bpr", {}, "Test values"}};
    for (auto& l : d.network.links) l.linkTypeId = "arterial";
    return w;
}
std::size_t arcOf(const MacroRun& r, const std::string& source) {
    for (std::size_t a = 0; a < r.graph.links.size(); ++a) if (r.graph.links[a].source == source) return a;
    throw std::runtime_error("no arc " + source);
}
std::vector<std::string> lines(const std::string& text) {
    std::vector<std::string> out; std::istringstream in(text);
    for (std::string line; std::getline(in, line);) out.push_back(line);
    return out;
}
}
TEST(macrooutput, ma12_another_alpha_and_beta_come_from_data) { // MA12
    const auto dir = std::filesystem::temp_directory_path() / "trafficsim-macro-vdf";
    std::filesystem::remove_all(dir);
    std::filesystem::copy(data(), dir, std::filesystem::copy_options::recursive);
    std::ofstream(dir / "vdf/bpr.json") << R"({"id":"bpr","form":"bpr","alpha":0.3,"beta":2,"source":"Test values"})";
    const auto w = twin();
    InputManifest shipped, edited;
    const auto base = runMacro(w.d, data(), "matrix:am", &shipped), other = runMacro(w.d, dir, "matrix:am", &edited);
    // The forcing: the two files hash differently, the shipped run is the D158 pair, and both
    // routes carry flow in it (an interior equilibrium).
    const auto hashOf = [](const Json& m) {
        for (const auto& f : m["files"]) if (f["logicalPath"] == "catalog/vdf/bpr.json") return f["sha256"].get<std::string>();
        return std::string{};
    };
    CHECK(!hashOf(base.inputManifest).empty() && hashOf(base.inputManifest) != hashOf(other.inputManifest));
    const auto a1 = arcOf(base, w.routes[0]), a2 = arcOf(base, w.routes[1]);
    CHECK(base.graph.links[a1].alpha == 0.15 && base.graph.links[a1].beta == 4);
    CHECK(base.assignment.classFlows[0][a1] > 0 && base.assignment.classFlows[0][a2] > 0);
    // The edited data, with no code edit: its alpha and beta set the time, and the flows move.
    CHECK(other.graph.links[a1].alpha == 0.3 && other.graph.links[a1].beta == 2);
    const auto& r = other.assignment;
    test::near(r.time[a1], 10 * (1 + 0.3 * std::pow(r.load[a1] / 600, 2)), 1e-9);
    CHECK(r.converged && std::abs(r.classFlows[0][a1] - base.assignment.classFlows[0][a1]) > 10);
    // Equilibrium under the new pair: both routes' times equal (the shared Links cancel).
    const auto routeTime = [&](std::size_t k) {
        return r.time[arcOf(other, w.in[k])] + r.time[arcOf(other, w.routes[k])] + r.time[arcOf(other, w.out[k])];
    };
    CHECK(r.classFlows[0][a1] > 0 && r.classFlows[0][a2] > 0);
    CHECK(std::abs(routeTime(0) - routeTime(1)) / routeTime(0) < 1e-3);
    std::filesystem::remove_all(dir);
}
TEST(macrooutput, ma13_every_field_and_the_marker) { // MA13
    const auto w = twin(); const auto& d = w.d;
    InputManifest inputs;
    const auto run = runMacro(d, data(), "matrix:am", &inputs);
    CHECK(run.assignment.converged && run.classes.size() == 1 && !run.graph.links.empty()); // the forcing
    const auto j = macroJson(run);
    CHECK(j["validated"] == false && j["measure"].get<std::string>().starts_with("Static user-equilibrium assignment"));
    CHECK(j["period"] == "matrix:am" && j["revision"] == d.revision);
    CHECK(j["settings"]["targetGap"] == 1e-4 && j["settings"]["maxIterations"] == 1000);
    CHECK(j["gap"]["formula"].get<std::string>().find("sum v t") != std::string::npos);
    CHECK(j["gap"]["value"] == run.assignment.gap && j["gap"]["target"] == 1e-4);
    CHECK(j["iterations"] == run.assignment.iterations && j["converged"] == true && j["cancelled"] == false);
    CHECK(j["convergence"].size() == run.assignment.convergence.size() && j["convergence"].back()["gap"] == run.assignment.gap); // MA18
    CHECK(j["convergence"][0] == Json({{"iteration", 0}, {"gap", run.assignment.convergence[0].gap}, {"step", run.assignment.convergence[0].step}}));
    CHECK(j["advisories"].is_array() && j["inputManifest"]["files"].size() > 0);
    CHECK(j["classes"].size() == 1 && j["classes"][0]["id"] == "composition:urban-mixed" && j["classes"][0]["mode"] == "");
    CHECK(j["classes"][0]["pcu"] == run.classes[0].pcu);
    CHECK(j["links"].size() == run.graph.links.size());
    const auto a1 = arcOf(run, w.routes[0]);
    const auto& row = j["links"][a1];
    CHECK(row["kind"] == "link" && row["source"] == w.routes[0] && row["fromStation"] == 0.0 && row["toStation"] == 100.0);
    CHECK(row["linkType"] == "arterial");
    CHECK(row["flow"]["composition:urban-mixed"] == run.assignment.classFlows[0][a1] && row["load"] == run.assignment.load[a1]);
    CHECK(row["vc"] == run.assignment.load[a1] / 600 && row["freeTime"] == 10.0 && row["time"] == run.assignment.time[a1]);
    const auto connector = j["links"][arcOf(run, w.in[0])];
    CHECK(connector["kind"] == "connector" && connector["vc"].is_null() && connector["linkType"] == "");
    CHECK(j["links"][arcOf(run, "s1")]["kind"] == "centroid");
    // CSV: the marker first, context lines, a header with a column per class, a row per arc.
    const auto csv = lines(macroCsv(run));
    CHECK(csv[0].starts_with("# MULTRA - not yet validated. Static user-equilibrium assignment"));
    CHECK(csv[1] == "# Period: matrix:am");
    CHECK(csv[2].starts_with("# Relative gap: ") && csv[2].find("converged: yes") != std::string::npos);
    CHECK(csv[3] == "kind,source,fromStation,toStation,linkType,flow:composition:urban-mixed,load,vc,freeTime,time");
    CHECK(csv.size() == 4 + run.graph.links.size());
    const auto& as = run.assignment;
    CHECK(csv[4 + a1] == "link,\"" + w.routes[0] + "\",0.00,100.00,\"arterial\"," + csvNumber(as.classFlows[0][a1]) + "," +
                         csvNumber(as.load[a1]) + "," + csvNumber(as.load[a1] / 600) + ",10.00," + csvNumber(as.time[a1]));
    // A cap of 1 on three routes stops short: flagged, and every row still written.
    auto capped = twin(true).d;
    changeMacroSettings(capped, MacroSettings{1e-4, 1});
    const auto stopped = runMacro(capped, data(), "matrix:am");
    CHECK(!stopped.assignment.converged && stopped.assignment.iterations == 1);
    CHECK(std::any_of(stopped.advisories.begin(), stopped.advisories.end(),
        [](const auto& a) { return a.code == "MACRO_NOT_CONVERGED" && a.path == "definition.macroSettings.maxIterations"; }));
    const auto sj = macroJson(stopped);
    CHECK(sj["converged"] == false && sj["links"].size() == stopped.graph.links.size());
    CHECK(sj["advisories"].back()["code"] == "MACRO_NOT_CONVERGED");
    CHECK(lines(macroCsv(stopped))[2].find("converged: no") != std::string::npos);
    CHECK(std::none_of(run.advisories.begin(), run.advisories.end(), [](const auto& a) { return a.code == "MACRO_NOT_CONVERGED"; }));
    // An unknown period is refused by name.
    test::throws([&] { runMacro(d, data(), "pm"); }, "MACRO_PERIOD_UNKNOWN");
}
TEST(macrooutput, ma13_settings_are_schema_51_only_when_set) { // MA13
    auto d = twin().d;
    const auto plain = documentJson(d);
    CHECK(plain["schemaVersion"] == 50 && !plain["definition"].contains("macroSettings")); // absent keeps 50
    changeMacroSettings(d, MacroSettings{1e-3, 50});
    const auto j = documentJson(d);
    CHECK(j["schemaVersion"] == 51 && j["definition"]["macroSettings"] == Json({{"targetGap", 1e-3}, {"maxIterations", 50}}));
    const auto back = parseDocument(j);
    CHECK(back.definition->macroSettings == (MacroSettings{1e-3, 50}) && documentJson(back) == j);
    auto older = j; older["schemaVersion"] = 50;
    CHECK(refusedAt(older, "EDIT_UNSUPPORTED_FIELD", "definition.macroSettings"));
    for (const auto& [field, value] : std::vector<std::pair<std::string, Json>>{
             {"targetGap", 0.0}, {"targetGap", 1.0}, {"targetGap", "x"}, {"maxIterations", 0}, {"maxIterations", 1.5}, {"maxIterations", 100001}}) {
        auto bad = j; bad["definition"]["macroSettings"][field] = value;
        CHECK(refusedAt(bad, "INVALID_MACRO_SETTING", "definition.macroSettings." + field));
    }
    auto missing = j; missing["definition"]["macroSettings"].erase("maxIterations");
    CHECK(refusedAt(missing, "INVALID_MACRO_SETTING", "definition.macroSettings.maxIterations"));
    auto extra = j; extra["definition"]["macroSettings"]["step"] = 1;
    CHECK(refusedAt(extra, "INVALID_MACRO_SETTING", "definition.macroSettings.step"));
    test::throws([&] { changeMacroSettings(d, MacroSettings{0, 10}); }, "INVALID_MACRO_SETTING");
    changeMacroSettings(d, std::nullopt);
    CHECK(documentJson(d)["schemaVersion"] == 50);
    auto future = j; future["schemaVersion"] = 52;
    test::throws([&] { parseDocument(future); }, "EDIT_VERSION");
}
TEST(macrooutput, ma14_stages_are_reported_and_a_refusal_names_its_stage) { // MA14 (§19 a)
    const auto w = twin();
    std::vector<MacroStage> stages; int iterations = 0;
    const MacroObserver observer{[&](MacroStage s) { stages.push_back(s); }, [&](const MacroIteration&) { ++iterations; return true; }};
    const auto run = runMacro(w.d, data(), "matrix:am", nullptr, &observer);
    CHECK((stages == std::vector{MacroStage::period, MacroStage::graph, MacroStage::demand, MacroStage::assignment}));
    CHECK(iterations == static_cast<int>(run.assignment.convergence.size()) && !run.assignment.cancelled);
    // A Link without a type: the graph stage refuses, and no later stage is entered.
    auto untyped = w.d; untyped.network.links[0].linkTypeId.reset();
    stages.clear();
    test::throws([&] { runMacro(untyped, data(), "matrix:am", nullptr, &observer); }, "MACRO_LINK_TYPE_MISSING");
    CHECK(stages.back() == MacroStage::graph);
    // A cancel from the observer: flagged, and not reported as MACRO_NOT_CONVERGED.
    const MacroObserver stop{{}, [](const MacroIteration&) { return false; }};
    const auto cancelled = runMacro(w.d, data(), "matrix:am", nullptr, &stop);
    CHECK(cancelled.assignment.cancelled && cancelled.advisories.empty() && macroJson(cancelled)["cancelled"] == true);
}
TEST(macrooutput, ma14_link_type_commands) { // MA14
    auto d = twin().d;
    // The installed catalog (none ship): the first put owns it, and the file becomes schema 50.
    auto installed = d; installed.definition->externalLinkTypes = true; installed.definition->linkTypes.clear();
    for (auto& l : installed.network.links) l.linkTypeId.reset();
    CHECK(documentJson(installed)["schemaVersion"] == 49);
    test::throws([&] { deleteLinkType(installed, "arterial"); }, "EDIT_LINK_TYPES_NOT_OWNED");
    const auto id = putLinkType(installed, {"", "Local", 500, 8, "bpr", {}, "Test values"}, {});
    CHECK(id.starts_with("link-type") && !installed.definition->externalLinkTypes && installed.definition->linkTypes.size() == 1);
    CHECK(documentJson(installed)["schemaVersion"] == 50);
    // Replace by id; a type in use cannot go, an unused one can.
    putLinkType(d, {"arterial", "Arterial", 700, 10, "bpr", {}, "Test values"}, {});
    CHECK(d.definition->linkTypes.size() == 1 && d.definition->linkTypes[0].capacityPerLane == 700);
    test::throws([&] { deleteLinkType(d, "arterial"); }, "EDIT_LINK_TYPE_IN_USE");
    for (const auto& l : std::vector(d.network.links)) setLinkType(d, l.id, std::nullopt);
    CHECK(std::none_of(d.network.links.begin(), d.network.links.end(), [](const auto& l) { return l.linkTypeId.has_value(); }));
    setLinkType(d, d.network.links[0].id, std::string("arterial"));
    CHECK(d.network.links[0].linkTypeId == "arterial");
    setLinkType(d, d.network.links[0].id, std::string(""));
    CHECK(!d.network.links[0].linkTypeId);
    deleteLinkType(d, "arterial");
    CHECK(d.definition->linkTypes.empty() && !d.definition->externalLinkTypes);
    test::throws([&] { deleteLinkType(d, "arterial"); }, "EDIT_UNKNOWN_OBJECT");
}
