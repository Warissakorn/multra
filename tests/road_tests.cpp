// S3.3 rows RD1 (Road part), RD5-RD9, and S3.4's RD10-RD11 at the end (docs/reference/ROADS.md §3, §4, §13, D305): the Road, the
// cross-section template catalog, Apply template and detaching. Each row first checks its forcing.
#include "test.hpp"
#include "../src/commands/appearance_commands.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/commands/road_commands.hpp"
#include "../src/model/network/roads.hpp"
#include "../src/project/road_codec.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <numbers>
using namespace multra;
namespace {
std::filesystem::path data() { return test::root() / "data"; }
bool refusedAt(const Json& j, const std::string& code, const std::string& path) {
    try { parseDocument(j); } catch (const ValidationError& e) {
        return std::any_of(e.issues.begin(), e.issues.end(), [&](const auto& i) { return i.code == code && i.path == path; });
    } catch (...) {}
    return false;
}
bool issueAt(const std::vector<ValidationIssue>& issues, const std::string& code, const std::string& path) {
    return std::any_of(issues.begin(), issues.end(), [&](const auto& i) { return i.code == code && i.path == path; });
}
template <class F> std::string thrown(F f) { try { f(); } catch (const std::exception& e) { return e.what(); } return {}; }
// Test values only: no template ships (D304, 2a).
CrossSection divided() { return {"divided", "2+2 divided", 2, 2, 2, 3.25, {}, 2.0, std::string("local"), "Test values"}; }
// A straight 200 m Road along +x with an owned 2+2 template.
ProjectDocument withRoad(DrivingSide side = DrivingSide::left) {
    ProjectDocument d;
    d.network.drivingSide = side;
    putCrossSection(d, divided(), {});
    putRoad(d, {"", "Main", {{0, 0}, {200, 0}}, std::string("divided"), 0, {}, {}});
    return d;
}
const Link& link(const ProjectDocument& d, const std::string& id) {
    return *std::find_if(d.network.links.begin(), d.network.links.end(), [&](const auto& l) { return l.id == id; });
}
}
TEST(roads, rd1_roads_are_schema_53) { // RD1 (Road part)
    auto d = withRoad();
    CHECK(d.network.roads.size() == 1 && d.network.links.empty()); // forcing: a Road, no Links yet
    auto j = documentJson(d);
    CHECK(j["schemaVersion"] == 53 && j["network"]["roads"][0]["crossSectionId"] == "divided" && j["definition"]["crossSections"].size() == 1);
    CHECK(parseDocument(Json::parse(j.dump())) .network.roads == d.network.roads);
    applyTemplate(d, d.network.roads[0].id, {});
    j = documentJson(d);
    CHECK(j["network"]["links"][0]["roadId"] == d.network.roads[0].id);
    const auto again = parseDocument(Json::parse(j.dump()));
    CHECK(again.network == d.network && again.definition->crossSections == d.definition->crossSections);
    auto old = j; old["schemaVersion"] = 52;
    CHECK(refusedAt(old, "EDIT_UNSUPPORTED_FIELD", "definition.crossSections"));
    old["definition"].erase("crossSections");
    CHECK(refusedAt(old, "EDIT_UNSUPPORTED_FIELD", "network.roads"));
    old["network"].erase("roads");
    CHECK(refusedAt(old, "EDIT_UNSUPPORTED_FIELD", "network.links[0].roadId"));
    for (const auto& entry : std::filesystem::directory_iterator(data() / "projects")) { // shipped projects keep their schema
        if (!entry.path().string().ends_with(".traffic.json")) continue; // the folder's README
        std::ifstream file(entry.path()); Json p; file >> p;
        CHECK(documentJson(parseDocument(p))["schemaVersion"] == p["schemaVersion"]);
    }
}
TEST(roads, rd5_the_template_codec) { // RD5
    const auto t = divided();
    CHECK(parseCrossSection(crossSectionJson(t)) == t);
    auto widths = t; widths.laneWidths = {3.0, 3.5, 3.25, 3.25};
    CHECK(parseCrossSection(crossSectionJson(widths)) == widths);
    CHECK(resolveCrossSections(AuthoringDefinition{}, data()).empty()); // installed: none ships
    const auto bad = [&](auto change, const std::string& field) {
        auto x = t; change(x);
        return issueAt(crossSectionIssues({{x, "definition.crossSections[0]"}}), "INVALID_CROSS_SECTION", "definition.crossSections[0]." + field);
    };
    CHECK(crossSectionIssues({{t, "p"}}).empty()); // forcing: the good one has none
    CHECK(bad([](auto& x) { x.directions = 3; }, "directions"));
    CHECK(bad([](auto& x) { x.lanesForward = 0; }, "lanesForward"));
    CHECK(bad([](auto& x) { x.directions = 1; }, "lanesBackward")); // one direction needs lanesBackward 0
    CHECK(bad([](auto& x) { x.laneWidth = 0; }, "laneWidth"));
    CHECK(bad([](auto& x) { x.laneWidths = {3, 3}; }, "laneWidths"));
    CHECK(bad([](auto& x) { x.median = -1; }, "median"));
    CHECK(bad([](auto& x) { x.source = " "; }, "source"));
    const std::vector<std::string> types{"other"};
    CHECK(issueAt(crossSectionIssues({{t, "p"}}, &types), "INVALID_CROSS_SECTION", "p.linkType"));
    test::throws([&] { parseCrossSection(Json{{"id", "x"}}); }, "EDIT_CATALOG_READ");
}
TEST(roads, rd6_apply_template_generates_both_directions) { // RD6
    History h; h.reset(withRoad());
    const auto road = h.document().network.roads[0].id;
    const auto before = documentJson(h.document()).dump();
    std::vector<std::string> ids;
    CHECK(h.execute("editorApplyTemplate", [&](auto& d) { ids = applyTemplate(d, road, {}); }));
    const auto& d = h.document();
    CHECK(ids.size() == 2 && d.network.roads[0].links == ids); // forcing: both generated
    const auto& f = link(d, ids[0]); const auto& b = link(d, ids[1]);
    // Left-hand driving: forward centred 1 + 3.25 m to the left (+y), backward to the right, reversed.
    CHECK(f.geometry.front().x == 0 && f.geometry.back().x == 200);
    test::near(f.geometry.front().y, 4.25, 1e-9); test::near(b.geometry.front().y, -4.25, 1e-9);
    CHECK(b.geometry.front().x == 200 && b.geometry.back().x == 0);
    CHECK(f.lanes.size() == 2 && b.lanes.size() == 2 && f.lanes[0].width == 3.25 && f.laneOffset == 0);
    CHECK(f.linkTypeId == "local" && f.roadId == road && b.roadId == road && f.level == 0);
    h.undo();
    CHECK(documentJson(h.document()).dump() == before); // one Undo step restores the bytes
    // Right-hand driving mirrors it.
    auto r = withRoad(DrivingSide::right);
    const auto mirrored = applyTemplate(r, r.network.roads[0].id, {});
    test::near(link(r, mirrored[0]).geometry.front().y, -4.25, 1e-9);
    test::near(link(r, mirrored[1]).geometry.front().y, 4.25, 1e-9);
}
TEST(roads, rd7_reapply_keeps_ids_and_routes) { // RD7
    auto d = withRoad();
    const auto road = d.network.roads[0].id;
    const auto ids = applyTemplate(d, road, {});
    const auto lanes = link(d, ids[0]).lanes;
    // A downstream Link joined by a Connector, and a route over both.
    const auto next = addLink(d, {{210, 4.25}, {300, 4.25}}, 2, 3.25);
    addConnector(d, {ids[0], lanes[0].id, 200}, {next, link(d, next).lanes[0].id, 0});
    const auto connector = d.network.connectors[0].id;
    putRoute(d, {"", {ids[0], connector, next}});
    validateDocument(d); // forcing: valid before
    d.network.roads[0].overrides.lanesForward = 3;
    const auto again = applyTemplate(d, road, {});
    CHECK(again == ids);
    const auto& f = link(d, ids[0]);
    CHECK(f.lanes.size() == 3 && f.lanes[0].id == lanes[0].id && f.lanes[1].id == lanes[1].id && f.roadId == road);
    test::near(f.geometry.front().y, 1 + 3 * 3.25 / 2, 1e-9);
    validateDocument(d);
    CHECK(d.definition->routes[0].segmentIds == (std::vector<std::string>{ids[0], connector, next}));
    // Dropping a lane the Connector uses is refused; so is dropping a direction.
    auto narrow = d; narrow.network.roads[0].overrides.lanesForward = 1;
    narrow.network.connectors[0].from.laneId = f.lanes[1].id;
    CHECK(thrown([&] { applyTemplate(narrow, road, {}); }) == "EDIT_REFERENCED_LANE");
    auto oneWay = d; oneWay.definition->crossSections[0].directions = 1; oneWay.definition->crossSections[0].lanesBackward = 0;
    CHECK(thrown([&] { applyTemplate(oneWay, road, {}); }) == "EDIT_ROAD_DIRECTIONS");
}
TEST(roads, rd8_a_hand_edit_detaches) { // RD8
    auto d = withRoad();
    const auto road = d.network.roads[0].id;
    const auto ids = applyTemplate(d, road, {});
    CHECK(link(d, ids[0]).roadId == road); // forcing: attached
    changeLanes(d, ids[0], {3.5, 3.5});
    CHECK(!link(d, ids[0]).roadId && d.network.roads[0].links == std::vector<std::string>{ids[1]});
    validateDocument(d);
    auto moved = withRoad(); const auto m = applyTemplate(moved, moved.network.roads[0].id, {});
    changeGeometry(moved, m[1], {{200, -5}, {0, -5}});
    CHECK(!link(moved, m[1]).roadId && moved.network.roads[0].links == std::vector<std::string>{m[0]});
    deleteLink(moved, m[0]);
    CHECK(moved.network.roads[0].links.empty());
    validateDocument(moved);
    auto gone = withRoad(); const auto g = applyTemplate(gone, gone.network.roads[0].id, {});
    deleteRoad(gone, gone.network.roads[0].id);
    CHECK(gone.network.roads.empty() && gone.network.links.size() == 2 && !link(gone, g[0]).roadId);
    validateDocument(gone);
}
TEST(roads, rd9_road_refusals) { // RD9
    auto d = withRoad();
    validateDocument(d); // forcing: valid
    auto line = d; line.network.roads[0].centreline = {{0, 0}, {0, 0}};
    try { validateDocument(line); CHECK(false); } catch (const ValidationError& e) { CHECK(issueAt(e.issues, "ROAD_GEOMETRY_INVALID", "roads[0].centreline")); }
    auto unknown = d; unknown.network.roads[0].crossSectionId = "nope";
    try { validateDocument(unknown); CHECK(false); } catch (const ValidationError& e) { CHECK(issueAt(e.issues, "ROAD_CROSS_SECTION_UNKNOWN", "network.roads[0].crossSectionId")); }
    // On the installed catalog (none ships), Apply refuses an unknown template at the Road.
    ProjectDocument installed; putRoad(installed, {"", "", {{0, 0}, {10, 0}}, std::string("divided"), 0, {}, {}});
    try { applyTemplate(installed, installed.network.roads[0].id, {}); CHECK(false); }
    catch (const ValidationError& e) { CHECK(issueAt(e.issues, "ROAD_CROSS_SECTION_UNKNOWN", "network.roads[0].crossSectionId")); }
    CHECK(applyTemplate(installed, installed.network.roads[0].id, {divided()}).size() == 2); // given installed, it applies
    auto mismatch = d; applyTemplate(mismatch, mismatch.network.roads[0].id, {});
    mismatch.network.links[0].roadId = "elsewhere";
    try { validateDocument(mismatch); CHECK(false); } catch (const ValidationError& e) {
        CHECK(issueAt(e.issues, "ROAD_LINK_MISMATCH", "roads[0].links[0]") && issueAt(e.issues, "ROAD_LINK_MISMATCH", "links[0].roadId"));
    }
    test::throws([&] { auto x = d; putRoad(x, {"", "", {{0, 0}, {1, 0}}, {}, 0, {0, {}, {}, {}}, {}}); }, "EDIT_ROAD_OVERRIDE");
}
// S3.4 rows RD10, RD11 (ROADS §3, §17, D308).
TEST(roads, rd10_join_opposite_links) { // RD10
    ProjectDocument d;
    const auto a = addLink(d, {{0, 0}, {100, 0}, {200, 50}}, 2, 3.5);
    const auto b = oppositeLink(d, a, 2.0);
    CHECK(link(d, b).geometry.size() == 3 && link(d, b).geometry.front().x > 150); // forcing: opposite, reversed
    const auto f = linkCentreline(link(d, a), d.network.drivingSide), r = linkCentreline(link(d, b), d.network.drivingSide);
    const auto id = joinOppositeLinks(d, a, b);
    const auto& road = d.network.roads.at(0);
    CHECK(road.id == id && road.links == (std::vector<std::string>{a, b}) && !road.crossSectionId && road.level == 0);
    CHECK(link(d, a).roadId == id && link(d, b).roadId == id && road.centreline.size() == 3);
    for (std::size_t i = 0; i < 3; ++i) { // midway, vertex by vertex, at the bend too
        test::near(road.centreline[i].x, (f[i].x + r[2 - i].x) / 2, 1e-9);
        test::near(road.centreline[i].y, (f[i].y + r[2 - i].y) / 2, 1e-9);
    }
    CHECK(link(d, a).geometry == (std::vector<Point>{{0, 0}, {100, 0}, {200, 50}})); // the Links are not moved
    validateDocument(d);
    CHECK(thrown([&] { auto x = d; joinOppositeLinks(x, a, b); }) == "EDIT_ROAD_JOIN_TAKEN");
    // Unequal vertex counts: each vertex pairs with its closest point, ordered along the forward Link.
    ProjectDocument u;
    const auto p = addLink(u, {{0, 0}, {200, 0}}, 1, 3.5), q = addLink(u, {{200, 10}, {100, 10}, {0, 10}}, 1, 3.5);
    joinOppositeLinks(u, p, q);
    const auto& mean = u.network.roads[0].centreline;
    CHECK(mean.size() == 3);
    for (const auto& v : mean) test::near(v.y, 5, 1e-9);
    CHECK(mean.front().x < mean[1].x && mean[1].x < mean.back().x);
    // Refusals: the same direction, more than 15 degrees off opposite, farther than the widths plus
    // 30 m, another level, or one Link twice.
    ProjectDocument s;
    const auto base = addLink(s, {{0, 0}, {200, 0}}, 2, 3.5);
    const auto same = addLink(s, {{0, 10}, {200, 10}}, 2, 3.5);
    const auto skew = addLink(s, {{200, 10}, {0, 10 + 200 * std::tan(20 * std::numbers::pi / 180)}}, 2, 3.5);
    const auto slight = addLink(s, {{200, 10}, {0, 10 + 200 * std::tan(10 * std::numbers::pi / 180)}}, 2, 3.5);
    const auto far = addLink(s, {{200, 45}, {0, 45}}, 2, 3.5);
    const auto near = addLink(s, {{200, 30}, {0, 30}}, 2, 3.5);
    CHECK(thrown([&] { auto x = s; joinOppositeLinks(x, base, same); }) == "EDIT_ROAD_JOIN_DIRECTION");
    CHECK(thrown([&] { auto x = s; joinOppositeLinks(x, base, skew); }) == "EDIT_ROAD_JOIN_PARALLEL");
    CHECK(thrown([&] { auto x = s; joinOppositeLinks(x, base, far); }) == "EDIT_ROAD_JOIN_PARALLEL"); // 45 m > 7 m of half widths + 30 m
    CHECK(thrown([&] { auto x = s; joinOppositeLinks(x, base, slight); }) == "EDIT_ROAD_JOIN_PARALLEL"); // 10 degrees, but 45 m apart at x = 0
    CHECK(thrown([&] { auto x = s; joinOppositeLinks(x, base, base); }) == "EDIT_ROAD_JOIN_DIRECTION");
    CHECK(thrown([&] { auto x = s; changeAppearance(x, near, 1, link(x, near).displayType); joinOppositeLinks(x, base, near); }) == "EDIT_ROAD_JOIN_LEVEL");
    { auto x = s; joinOppositeLinks(x, base, near); validateDocument(x); } // 30 m apart: within 7 + 30 m
    // The angle on its own: 40 m Links within the distance, 20 degrees off opposite is refused, 10 is joined.
    ProjectDocument t;
    const auto shortBase = addLink(t, {{0, 0}, {40, 0}}, 2, 3.5);
    const auto steep = addLink(t, {{40, 5}, {0, 5 + 40 * std::tan(20 * std::numbers::pi / 180)}}, 2, 3.5);
    const auto gentle = addLink(t, {{40, 5}, {0, 5 + 40 * std::tan(10 * std::numbers::pi / 180)}}, 2, 3.5);
    CHECK(thrown([&] { auto x = t; joinOppositeLinks(x, shortBase, steep); }) == "EDIT_ROAD_JOIN_PARALLEL");
    { auto x = t; joinOppositeLinks(x, shortBase, gentle); CHECK(x.network.roads.size() == 1); }
}
TEST(roads, rd11_assign_template_generates_nothing) { // RD11
    History h; h.reset(withRoad());
    CHECK(h.execute("x", [&](auto& d) { putRoad(d, {"", "", {{0, 50}, {200, 50}}, {}, 0, {}, {}}); putRoad(d, {"", "", {{0, 90}, {200, 90}}, {}, 0, {}, {}}); }));
    const auto& roads = [&]() -> const std::vector<Road>& { return h.document().network.roads; };
    CHECK(roads().size() == 3 && !roads()[1].crossSectionId && h.document().network.links.empty()); // forcing
    const auto before = h.revision();
    std::vector<std::string> ids; for (const auto& r : roads()) ids.push_back(r.id);
    CHECK(h.execute("editorAssignTemplate", [&](auto& d) { assignTemplate(d, ids, std::string("divided"), {}); }));
    CHECK(h.revision() == before + 1 && h.document().network.links.empty());
    CHECK(std::all_of(roads().begin(), roads().end(), [](const auto& r) { return r.crossSectionId == "divided"; }));
    auto x = h.document();
    try { assignTemplate(x, ids, std::string("nope"), {}); CHECK(false); }
    catch (const ValidationError& e) { CHECK(issueAt(e.issues, "ROAD_CROSS_SECTION_UNKNOWN", "crossSectionId")); }
    assignTemplate(x, {ids[1]}, std::nullopt, {});
    CHECK(!x.network.roads[1].crossSectionId && x.network.roads[0].crossSectionId == "divided");
    CHECK(thrown([&] { auto y = h.document(); assignTemplate(y, {"missing"}, std::nullopt, {}); }) == "EDIT_UNKNOWN_OBJECT");
}
TEST(roads, rd32_direction_survives_a_detach) { // RD32 (D382)
    // The forward Link detached by hand leaves the backward one first in `links`: it stays backward.
    auto d = withRoad();
    const auto road = d.network.roads[0].id;
    const auto ids = applyTemplate(d, road, {});
    changeLanes(d, ids[0], {3.5, 3.5, 3.5});
    CHECK(d.network.roads[0].links == std::vector<std::string>{ids[1]}); // forcing: only the backward Link is listed
    CHECK(!forwardOnRoad(d.network.roads[0], link(d, ids[1])));
    useLinkLanes(d, ids[1]);
    CHECK(d.network.roads[0].overrides.lanesBackward == 2 && !d.network.roads[0].overrides.lanesForward);
    d.network.roads[0].overrides = {};
    const auto start = link(d, ids[1]).geometry.front();
    const auto again = applyTemplate(d, road, {});
    CHECK(again.size() == 2 && again[1] == ids[1] && again[0] != ids[0] && again[0] != ids[1]);
    CHECK(d.network.roads[0].links == again);
    CHECK(link(d, ids[1]).geometry.front() == start && link(d, again[0]).geometry.front().x == 0);
    validateDocument(d);
    // Split with only the backward Link listed: its original id runs from the far end, so the new Road takes it.
    auto lone = withRoad(); const auto l = applyTemplate(lone, lone.network.roads[0].id, {});
    changeLanes(lone, l[0], {3.5, 3.5, 3.5});
    const auto next = splitRoad(lone, lone.network.roads[0].id, 50);
    const auto& rest = *std::find_if(lone.network.roads.begin(), lone.network.roads.end(), [&](const auto& r) { return r.id == next; });
    CHECK(rest.links == std::vector<std::string>{l[1]});
    validateDocument(lone);
}
TEST(roads, rd33_direction_on_a_u_shaped_road) { // RD33 (D382)
    // A U whose ends point the same way: the start-to-end chord says nothing, the stations along it do.
    ProjectDocument d;
    putCrossSection(d, divided(), {});
    const auto road = putRoad(d, {"", "U", {{0, 0}, {100, 0}, {100, 60}, {0, 60}}, std::string("divided"), 0, {}, {}});
    const auto ids = applyTemplate(d, road, {});
    const auto& r = d.network.roads[0];
    CHECK(ids.size() == 2); // forcing: both generated
    CHECK(forwardOnRoad(r, link(d, ids[0])) && !forwardOnRoad(r, link(d, ids[1])));
    changeLanes(d, ids[0], {3.5, 3.5, 3.5});
    useLinkLanes(d, ids[1]);
    CHECK(d.network.roads[0].overrides.lanesBackward == 2 && !d.network.roads[0].overrides.lanesForward);
}
