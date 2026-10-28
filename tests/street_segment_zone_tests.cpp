#include "test.hpp"
#include "street_segment_zone_fixture.hpp"
#include "../src/commands/node_commands.hpp"
#include "../src/project/macro_output.hpp"
#include "../src/project/meso_run.hpp"
#include "../src/project/od_demand.hpp"
#include "../src/project/readiness.hpp"
#include "../src/project/run.hpp"
#include "../src/project/zone_codec.hpp"
#include <limits>
using namespace multra;
namespace {
std::filesystem::path data() { return test::root() / "data"; }
ProjectDocument fixture() { return test::streetSegmentZoneFixture(test::root() / "tests/macro-two-route.traffic.json"); }
bool issueAt(const std::vector<ValidationIssue>& issues, const std::string& code, const std::string& path) {
    return std::any_of(issues.begin(), issues.end(), [&](const auto& i) { return i.code == code && i.path == path; });
}
bool refusedAt(const Json& j, const std::string& code, const std::string& path) {
    try { parseDocument(Json::parse(j.dump())); } catch (const ValidationError& e) { return issueAt(e.issues, code, path); }
    return false;
}
Point endpoint(const ProjectDocument& d, const ZoneConnector& c) {
    const auto r = std::find_if(d.network.streetSegments.begin(), d.network.streetSegments.end(), [&](const auto& x) { return x.id == c.streetSegmentId; });
    CHECK(r != d.network.streetSegments.end());
    return c.streetSegmentForward == (c.direction == ZoneDirection::destination) ? r->centreline.back() : r->centreline.front();
}
void junction(ProjectDocument& d) { putNode(d, {"", "", {{-10, -10}, {10, -10}, {10, 10}, {-10, 10}}, false}); }
void checkFlows(const MacroRun& run, double forward = 1200, double backward = 300) {
    CHECK(run.assignment.converged && run.classes.size() == 1);
    test::near(run.classes[0].pcu, 1.05, 1e-12);
    unsigned pieces = 0;
    for (std::size_t i = 0; i < run.graph.links.size(); ++i) {
        const auto& arc = run.graph.links[i];
        if (arc.kind != MacroArc::link) continue;
        const double vehicles = arc.toStation > arc.fromStation ? forward : backward;
        test::near(run.assignment.classFlows[0][i], vehicles, 1e-9);
        test::near(run.assignment.load[i], vehicles * 1.05, 1e-9); ++pieces;
    }
    CHECK(pieces == 4);
}
}
TEST(streetsegmentzone, rd30_codec_and_existing_bytes) {
    const auto d = fixture();
    CHECK(d.network.links.empty() && d.zones.zoneConnectors.size() == 4);
    const auto saved = documentJson(d);
    CHECK(saved["schemaVersion"] == 59 && !saved["zoneConnectors"][0].contains("linkId"));
    CHECK(saved["zoneConnectors"][0]["streetSegmentDirection"] == "forward");
    CHECK(saved["zoneConnectors"][2]["streetSegmentDirection"] == "backward");
    CHECK(parseDocument(Json::parse(saved.dump())) == d);
    auto older = test::atSchema(saved, 56); // street-segment zone connectors came at 57
    CHECK(refusedAt(older, "EDIT_UNSUPPORTED_FIELD", "zoneConnectors[0].roadId"));
    older["zoneConnectors"][0].erase("roadId");
    CHECK(refusedAt(older, "EDIT_UNSUPPORTED_FIELD", "zoneConnectors[0].roadDirection"));
    auto unknown = saved; unknown["zoneConnectors"][0]["streetSegmentId"] = "unknown";
    CHECK(refusedAt(unknown, "ZONE_CONNECTOR_UNKNOWN_REF", "zoneConnectors[0].streetSegmentId"));
    for (const auto& [key, value] : std::vector<std::pair<std::string, Json>>{
            {"streetSegmentDirection", "sideways"}, {"streetSegmentDirection", nullptr}, {"streetSegmentId", ""}, {"streetSegmentId", 1}, {"extra", 1}, {"linkId", "link"}}) {
        auto bad = saved; bad["zoneConnectors"][0][key] = value;
        test::throws([&] { parseDocument(Json::parse(bad.dump())); }, "EDIT_ZONE_INVALID");
    }
    auto missing = saved; missing["zoneConnectors"][0].erase("streetSegmentDirection");
    test::throws([&] { parseDocument(Json::parse(missing.dump())); }, "EDIT_ZONE_INVALID");
    auto ambiguous = d; ambiguous.zones.zoneConnectors[0].linkId = "some-link";
    CHECK(issueAt(zoneLayerIssues(ambiguous), "ZONE_CONNECTOR_TARGET_INVALID", "zoneConnectors[0]"));
    ambiguous.zones.zoneConnectors[0].linkId.clear(); ambiguous.zones.zoneConnectors[0].streetSegmentId.clear();
    CHECK(issueAt(zoneLayerIssues(ambiguous), "ZONE_CONNECTOR_TARGET_INVALID", "zoneConnectors[0]"));
    for (const auto& entry : std::filesystem::directory_iterator(data() / "projects")) {
        if (!entry.path().string().ends_with(".traffic.json")) continue;
        std::ifstream file(entry.path()); Json old; file >> old;
        CHECK(documentJson(parseDocument(old)).dump() == old.dump());
    }
    std::ifstream base(test::root() / "tests/macro-two-route.traffic.json"); Json link; base >> link;
    const auto unchanged = documentJson(parseDocument(link));
    CHECK(unchanged.dump() == link.dump() && unchanged["schemaVersion"] == 50);
}
TEST(streetsegmentzone, rd31_direction_shares_periods_and_replay) {
    auto d = fixture(); const auto before = documentJson(d).dump();
    const auto run = runMacro(d, data(), "matrix:am");
    checkFlows(run);
    CHECK(run.assignment == runMacro(d, data(), "matrix:am").assignment);
    CHECK(documentJson(d).dump() == before && d.network.links.empty());
    CHECK(!macroJson(run)["validated"].get<bool>());
    auto second = d.zones.zoneConnectors[0]; second.id = "west-in-second"; second.share = 3;
    d.zones.zoneConnectors.push_back(second);
    const auto split = runMacro(d, data(), "matrix:am"); checkFlows(split);
    for (std::size_t i = 0; i < split.graph.links.size(); ++i) {
        const auto& arc = split.graph.links[i];
        if (arc.source == "west-in") test::near(split.assignment.classFlows[0][i], 300);
        if (arc.source == "west-in-second") test::near(split.assignment.classFlows[0][i], 900);
    }
    d.zones.studyPeriods = {{"am-period", "AM", 0, 1800}, {"pm-period", "PM", 1800, 3600}};
    d.zones.odMatrices[0].periodId = "am-period";
    auto pm = d.zones.odMatrices[0]; pm.id = "pm-matrix"; pm.periodId = "pm-period"; pm.cells = {{0, 25}, {10, 0}};
    d.zones.odMatrices.push_back(pm);
    checkFlows(runMacro(d, data(), "am-period"), 2400, 600);
    checkFlows(runMacro(d, data(), "pm-period"), 50, 20);
    d.macroRuns = {{"saved", "StreetSegment AM", "am-period", {1e-4, 100}}};
    const auto named = runNamedMacro(d, data(), "saved");
    CHECK(named.definitionId == "saved"); checkFlows(named, 2400, 600);
}
TEST(streetsegmentzone, rd31_missing_direction_banned_turn_and_mixed_targets) {
    auto d = fixture();
    auto& section = d.definition->crossSections[0]; section.directions = 1; section.lanesBackward = 0;
    const auto oneway = macroNetwork(d, data());
    CHECK(std::count_if(oneway.graph.links.begin(), oneway.graph.links.end(), [](const auto& l) { return l.kind == MacroArc::link; }) == 2);
    CHECK(issueAt(oneway.issues, "MACRO_ZONE_STREET_SEGMENT_UNAVAILABLE", "zoneConnectors[2].streetSegmentDirection"));
    test::throws([&] { runMacro(d, data(), "matrix:am"); }, "MACRO_ZONE_STREET_SEGMENT_UNAVAILABLE");
    d = fixture(); d.definition->externalModes = false;
    d.definition->modes = {{"car", "Car", "urban-mixed"}, {"bus", "Bus", "urban-mixed"}};
    d.definition->linkTypes[0].modes = {"car"};
    d.zones.odMatrices[0].compositionId.clear(); d.zones.odMatrices[0].modeId = "bus";
    CHECK(macroNetwork(d, data()).issues.empty()); // forcing: the graph is valid, but the mode cannot enter
    test::throws([&] { runMacro(d, data(), "matrix:am"); }, "MACRO_NO_PATH");
    d.zones.odMatrices[0].modeId = "car"; checkFlows(runMacro(d, data(), "matrix:am"));
    d = fixture(); junction(d);
    d.network.nodes[0].coarse = NodeCoarse{std::nullopt, {}, {{"west", "east"}}, std::nullopt};
    const auto banned = macroNetwork(d, data());
    CHECK(banned.issues.empty());
    CHECK(std::count_if(banned.graph.links.begin(), banned.graph.links.end(), [](const auto& l) { return l.kind == MacroArc::connector; }) == 1);
    const auto demand = macroDemand(d, data(), {"", "am"}, banned.graph);
    CHECK(issueAt(demand.issues, "MACRO_NO_PATH", "odMatrices[0].cells[0][1]"));
    d = fixture(); junction(d); applyTemplate(d, "east", {});
    CHECK(d.network.links.size() == 2 && d.zones.zoneConnectors[1].streetSegmentId.empty() && !d.zones.zoneConnectors[1].linkId.empty());
    const auto mixed = runMacro(d, data(), "matrix:am");
    CHECK(mixed.assignment.converged && mixed.classes[0].cells.size() == 2);
    for (std::size_t i = 0; i < mixed.graph.links.size(); ++i)
        if (mixed.graph.links[i].source == "west" && mixed.graph.links[i].kind == MacroArc::link)
            test::near(mixed.assignment.classFlows[0][i], mixed.graph.links[i].toStation > mixed.graph.links[i].fromStation ? 1200 : 300);
}
TEST(streetsegmentzone, rd32_apply_and_atomic_refusal) {
    auto d = fixture();
    // All four zone/travel direction combinations on one StreetSegment.
    for (auto& c : d.zones.zoneConnectors) c.streetSegmentId = "west";
    History h; h.reset(d); const auto before = documentJson(h.document()).dump();
    std::vector<std::string> ids;
    CHECK(h.execute("apply", [&](auto& x) { ids = applyTemplate(x, "west", {}); }));
    CHECK(ids.size() == 2 && h.states().size() == 2);
    for (std::size_t i = 0; i < 4; ++i) {
        const auto& original = d.zones.zoneConnectors[i]; const auto& c = h.document().zones.zoneConnectors[i];
        CHECK(c.streetSegmentId.empty() && c.streetSegmentForward && c.linkId == ids[original.streetSegmentForward ? 0 : 1]);
        CHECK(c.id == original.id && c.zoneId == original.zoneId && c.direction == original.direction && c.share == original.share);
    }
    CHECK(documentJson(h.document())["schemaVersion"] == 59);
    const auto after = documentJson(h.document()).dump(); h.undo(); CHECK(documentJson(h.document()).dump() == before);
    h.redo(); CHECK(documentJson(h.document()).dump() == after);
    auto invalid = h.document(); invalid.zones.zoneConnectors[0] = d.zones.zoneConnectors[0];
    CHECK(issueAt(zoneLayerIssues(invalid), "ZONE_CONNECTOR_STREET_SEGMENT_HAS_LINKS", "zoneConnectors[0].streetSegmentId"));
    d.definition->crossSections[0].directions = 1; d.definition->crossSections[0].lanesBackward = 0;
    h.reset(d); const auto refused = documentJson(h.document()).dump();
    test::throws([&] { h.execute("apply", [](auto& x) { applyTemplate(x, "west", {}); }); }, "ZONE_CONNECTOR_STREET_SEGMENT_REQUIRES_DIRECTION");
    CHECK(documentJson(h.document()).dump() == refused && h.states().size() == 1 && !h.dirty());
    CHECK(h.document().network.links.empty());
}
TEST(streetsegmentzone, rd32_split_delete_and_merge_preserve_endpoints) {
    auto d = fixture(); for (auto& c : d.zones.zoneConnectors) c.streetSegmentId = "west";
    std::vector<Point> ends; for (const auto& c : d.zones.zoneConnectors) ends.push_back(endpoint(d, c));
    History h; h.reset(d); const auto counts = h.document().zones.odMatrices;
    std::string next;
    CHECK(h.execute("split", [&](auto& x) { next = splitStreetSegment(x, x.network.streetSegments.front().id, 100); }));
    CHECK(!next.empty() && h.document().network.streetSegments.size() == 3);
    for (std::size_t i = 0; i < 4; ++i) CHECK(endpoint(h.document(), h.document().zones.zoneConnectors[i]) == ends[i]);
    CHECK(h.document().zones.zoneConnectors[1].streetSegmentId == next && h.document().zones.zoneConnectors[2].streetSegmentId == next);
    CHECK(h.execute("delete", [&](auto& x) { deleteStreetSegment(x, next); }));
    CHECK(h.document().zones.zoneConnectors.size() == 2 && h.document().zones.odMatrices == counts);
    h.undo(); CHECK(h.document().zones.zoneConnectors.size() == 4); h.undo(); CHECK(h.document() == d);
    // A T becoming two compatible legs merges away, with the external endpoints preserved.
    d = fixture(); junction(d);
    putStreetSegment(d, {"stem", "", {{0, -200}, {0, 0}}, std::string("road-section"), 0, {}, {}});
    h.reset(d); CHECK(h.document().network.streetSegments.size() == 3 && h.document().network.nodes.size() == 1);
    ends.clear(); for (const auto& c : d.zones.zoneConnectors) ends.push_back(endpoint(d, c));
    CHECK(h.execute("delete", [](auto& x) { deleteStreetSegment(x, "stem"); }));
    CHECK(h.document().network.streetSegments.size() == 1 && h.document().network.nodes.empty());
    for (std::size_t i = 0; i < 4; ++i) CHECK(endpoint(h.document(), h.document().zones.zoneConnectors[i]) == ends[i]);
    h.undo(); CHECK(h.document() == d);
    // A connector at the shared end prevents merging; it is never silently moved outwards.
    auto internal = d.zones.zoneConnectors[0]; internal.id = "at-junction"; internal.direction = ZoneDirection::destination;
    d.zones.zoneConnectors.push_back(internal); h.reset(d);
    CHECK(endpoint(d, internal) == (Point{0, 0}));
    CHECK(h.execute("delete", [](auto& x) { deleteStreetSegment(x, "stem"); }));
    CHECK(h.document().network.streetSegments.size() == 2 && h.document().network.nodes.size() == 1);
    CHECK(endpoint(h.document(), h.document().zones.zoneConnectors.back()) == (Point{0, 0}));
}
TEST(streetsegmentzone, rd34_readiness_and_execution_refuse_unread_demand) {
    const auto d = fixture(); CHECK(runMacro(d, data(), "matrix:am").assignment.converged);
    for (const auto level : {ReadinessLevel::micro}) {
        const auto rows = readiness(d, data(), level);
        CHECK(std::any_of(rows.begin(), rows.end(), [](const auto& r) {
            return r.code == "ZONE_CONNECTOR_STREET_SEGMENT_REQUIRES_LINKS" && r.path == "zoneConnectors[0]" && r.severity == ReadinessSeverity::refusal;
        }));
    }
    test::throws([&] { compileDocument(d, data()); }, "ZONE_CONNECTOR_STREET_SEGMENT_REQUIRES_LINKS");
    test::throws([&] { withOdDemand(d); }, "ZONE_CONNECTOR_STREET_SEGMENT_REQUIRES_LINKS");
    const auto graph = mesoNetwork(d, data()); CHECK(graph.issues.empty());
    const auto md = mesoDemand(d, data(), graph.graph);
    CHECK(md.issues.empty() && !md.classes.empty()); // D341 adds the Meso reader; Micro still refuses
}
