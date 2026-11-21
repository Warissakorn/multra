#include "test.hpp"
#include "../src/commands/appearance_commands.hpp"
#include "../src/commands/catalog_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/history.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/commands/priority_marker_commands.hpp"
#include "../src/commands/right_of_way_commands.hpp"
#include "../src/model/network/priority_markers.hpp"
#include <algorithm>
using namespace multra;
// PRIORITY_RULES slice 2 (D616), rows PR7 and PR8: the Stop line tool's marker commands, one Undo
// step each, and the lifecycle that carries a rule's line and markers with their roads. A minor
// Link ends short of a 200 m major Link; neither area nor rule exists until the commands make them.
namespace {
struct Site { ProjectDocument d; std::string minor, major; ControlPoint line, marker; };
Site site() {
    Site s;
    s.minor = addLink(s.d, {{0, -60}, {0, -10}}, 1, 3.5);
    s.major = addLink(s.d, {{-100, 0}, {100, 0}}, 1, 3.5);
    const auto route = putRoute(s.d, {"route", {s.minor}}); changeRunSettings(s.d, 60, .1);
    putDemandCatalog(s.d, resolveDemandCatalog(AuthoringDefinition{}, test::root() / "data"));
    putInput(s.d, {"in", route, "car", 600, 0, 60});
    s.line = {{s.minor, s.d.network.links[0].lanes[0].id}, 30};
    s.marker = {{s.major, s.d.network.links[1].lanes[0].id}, 100};
    validateDocument(s.d);
    return s;
}
const AuthoredPriorityRule* ruleOf(const ProjectDocument& d) {
    for (const auto& r : d.network.rightOfWay.priorityRules) if (r.markerForm()) return &r;
    return nullptr;
}
// A site with a placed line, one marker and a Stop on the line, all through History.
History authored(const Site& s, std::string& line, std::string& rule) {
    History h; h.reset(s.d);
    CHECK(h.execute("line", [&](auto& d) { line = placeStopLine(d, s.line); }));
    CHECK(h.execute("marker", [&](auto& d) { rule = addPriorityMarker(d, line, s.marker); }));
    CHECK(h.execute("stop", [&](auto& d) { putStopControl(d, {"", "", line, StopMode::stop, {}}); }));
    return h;
}
}
TEST(priority_marker, pr7_place_add_remove_are_one_undo_step_each) {
    const auto s = site();
    History h; h.reset(s.d);
    std::string line, rule;
    // A line with no area downstream is placed (amends D595's refusal) and names no area.
    CHECK(h.execute("line", [&](auto& d) { line = placeStopLine(d, s.line); }));
    CHECK(!line.empty() && !ruleOf(h.document()));
    CHECK(h.execute("marker", [&](auto& d) { rule = addPriorityMarker(d, line, s.marker); }));
    const auto* r = ruleOf(h.document());
    CHECK(r && r->id == rule && r->waitingLineId == line && r->markers.size() == 1);
    CHECK(r->markers[0].gapTime == kMarkerGapTime && r->markers[0].headway == kMarkerHeadway && !r->markers[0].maxSpeed);
    auto second = s.marker; second.station = 150;
    CHECK(h.execute("second", [&](auto& d) { CHECK(addPriorityMarker(d, line, second) == rule); }));
    CHECK(ruleOf(h.document())->markers.size() == 2);
    CHECK(h.execute("move", [&](auto& d) { movePriorityMarker(d, rule, 1, 160); }));
    CHECK(ruleOf(h.document())->markers[1].point.station == 160);
    h.undo(); CHECK(ruleOf(h.document())->markers[1].point.station == 150);
    h.undo(); CHECK(ruleOf(h.document())->markers.size() == 1);
    // The line's own path is refused and nothing changes.
    const auto before = documentJson(h.document());
    auto own = s.line; own.station = 45;
    test::throws([&] { h.execute("own", [&](auto& d) { addPriorityMarker(d, line, own); }); }, "PRIORITY_MARKER_ON_OWN_PATH");
    CHECK(documentJson(h.document()) == before);
    // Removing the last marker removes the rule; Undo brings it back.
    CHECK(h.execute("remove", [&](auto& d) { removePriorityMarker(d, rule, 0); }));
    CHECK(!ruleOf(h.document()));
    h.undo(); CHECK(ruleOf(h.document()) && ruleOf(h.document())->markers.size() == 1);
    // Removing the line takes the rule with it, and a line a rule waits at cannot be deleted alone.
    test::throws([&] { auto d = h.document(); deleteWaitingLine(d, line); }, "EDIT_REFERENCED");
    CHECK(h.execute("drop", [&](auto& d) { removeStopLine(d, line); }));
    CHECK(!ruleOf(h.document()) && h.document().network.rightOfWay.waitingLines.empty());
}
TEST(priority_marker, pr7_removing_the_last_marker_takes_a_stop_with_no_area) {
    const auto s = site();
    std::string line, rule;
    auto h = authored(s, line, rule);
    CHECK(h.document().network.rightOfWay.stopControls.size() == 1);
    CHECK(h.execute("remove", [&](auto& d) { removePriorityMarker(d, rule, 0); }));
    CHECK(h.document().network.rightOfWay.stopControls.empty() && h.document().network.rightOfWay.waitingLines.size() == 1);
}
TEST(priority_marker, pr8_split_carries_markers_and_refuses_inside_their_span) {
    const auto s = site();
    std::string line, rule;
    auto h = authored(s, line, rule);
    std::string downstream;
    CHECK(h.execute("split", [&](auto& d) { downstream = splitLink(d, s.major, 50); }));
    const auto& n = h.document().network;
    const auto piece = std::find_if(n.links.begin(), n.links.end(), [&](const auto& l) { return l.id == downstream; });
    CHECK(piece != n.links.end());
    const auto& m = ruleOf(h.document())->markers[0];
    CHECK(m.point.path.linkId == downstream && m.point.path.laneId == piece->lanes[0].id);
    test::near(m.point.station, 100 - (200 - polylineLength(piece->geometry)), 1e-6);
    // The compiled marker is where it was: the same runtime position as before the split.
    const auto at = locateControlPoint(n, runtimeSections(n), m.point);
    CHECK(at.has_value());
    h.undo(); CHECK(ruleOf(h.document())->markers[0].point == s.marker);
    test::throws([&] { auto d = h.document(); splitLink(d, s.major, 100); }, "EDIT_SPLIT_CONTROL");
}
TEST(priority_marker, pr8_deleting_a_road_cascades_in_one_step) {
    const auto s = site();
    std::string line, rule;
    auto h = authored(s, line, rule);
    const auto before = documentJson(h.document());
    // The major Link: its marker goes, then the rule (no marker left) and the Stop with no area.
    CHECK(h.execute("major", [&](auto& d) { deleteLink(d, s.major); }));
    CHECK(!ruleOf(h.document()) && h.document().network.rightOfWay.stopControls.empty());
    CHECK(h.document().network.rightOfWay.waitingLines.size() == 1); // the line stands alone on its road
    h.undo(); CHECK(documentJson(h.document()) == before);
    // The minor Link: the line goes, and the rule and its Stop with it.
    CHECK(h.execute("minor", [&](auto& d) { deleteLink(d, s.minor); }));
    const auto& row = h.document().network.rightOfWay;
    CHECK(!ruleOf(h.document()) && row.stopControls.empty() && row.waitingLines.empty());
    h.undo(); CHECK(documentJson(h.document()) == before);
}
TEST(priority_marker, pr8_copying_both_roads_copies_the_rule) {
    const auto s = site();
    std::string line, rule;
    auto h = authored(s, line, rule);
    CHECK(h.execute("copy", [&](auto& d) { duplicateObjects(d, {s.minor, s.major}, {0, 200}); }));
    const auto& row = h.document().network.rightOfWay;
    std::size_t rules = 0;
    for (const auto& r : row.priorityRules) rules += r.markerForm();
    CHECK(rules == 2 && row.waitingLines.size() == 2 && row.stopControls.size() == 2);
    const auto copy = std::find_if(row.priorityRules.begin(), row.priorityRules.end(), [&](const auto& r) { return r.markerForm() && r.id != rule; });
    CHECK(copy->waitingLineId != line && copy->markers[0].point.path.linkId != s.major);
    validateDocument(h.document());
    // Copying the minor road alone: its line stands alone, with no rule and no Stop.
    h.undo();
    CHECK(h.execute("copy minor", [&](auto& d) { duplicateObjects(d, {s.minor}, {0, 200}); }));
    rules = 0;
    for (const auto& r : h.document().network.rightOfWay.priorityRules) rules += r.markerForm();
    CHECK(rules == 1 && h.document().network.rightOfWay.stopControls.size() == 1);
}
