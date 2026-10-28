// S3.5 rows RD12-RD15 (docs/reference/ROADS.md §5, §13, §18, D309): Find crossings, junction splits
// with their generated Nodes, overpasses and Merge nodes. Each row first checks its forcing.
#include "test.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/commands/node_commands.hpp"
#include "../src/commands/road_commands.hpp"
#include "../src/model/network/node_membership.hpp"
#include "../src/model/network/polygon.hpp"
#include "../src/project/road_codec.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
using namespace multra;
namespace {
template <class F> std::string thrown(F f) { try { f(); } catch (const std::exception& e) { return e.what(); } return {}; }
bool issueAt(const std::vector<ValidationIssue>& issues, const std::string& code, const std::string& path) {
    return std::any_of(issues.begin(), issues.end(), [&](const auto& i) { return i.code == code && i.path == path; });
}
std::string road(ProjectDocument& d, std::vector<Point> centreline, int level = 0, std::optional<std::string> section = {}) {
    return putRoad(d, {"", "", std::move(centreline), std::move(section), level, {}, {}});
}
// Test values only: no template ships (D304, 2a).
CrossSection divided() { return {"divided", "2+2 divided", 2, 2, 2, 3.25, {}, 2.0, std::nullopt, "Test values"}; }
const Road& named(const ProjectDocument& d, const std::string& id) {
    return *std::find_if(d.network.roads.begin(), d.network.roads.end(), [&](const auto& r) { return r.id == id; });
}
const Link& link(const ProjectDocument& d, const std::string& id) {
    return *std::find_if(d.network.links.begin(), d.network.links.end(), [&](const auto& l) { return l.id == id; });
}
std::vector<CrossingChoice> all(std::size_t n, CrossingChoice c) { return std::vector<CrossingChoice>(n, c); }
}
TEST(roadcrossings, rd12_find_crossings) { // RD12
    ProjectDocument d;
    road(d, {{0, 0}, {300, 0}}); road(d, {{0, 100}, {300, 100}});
    road(d, {{100, -50}, {100, 150}}); road(d, {{200, -50}, {200, 150}});
    auto found = findCrossings(d.network, 2);
    CHECK(found.size() == 4); // forcing: a two-by-two grid
    for (const auto& c : found) CHECK(c.legs == 4 && c.roads.size() == 2 && !c.endpoint && c.level == 0);
    test::near(found[0].at.x, 100, 1e-9); test::near(found[0].at.y, 0, 1e-9);
    road(d, {{150, -50}, {150, 150}}, 1); // another level never meets them
    CHECK(findCrossings(d.network, 2) == found);
    // A T: a Road ending 0.5 m short of another is one place of three legs, with an end.
    road(d, {{50, 51}, {50, 99.5}});
    found = findCrossings(d.network, 2);
    CHECK(found.size() == 5);
    const auto t = std::find_if(found.begin(), found.end(), [](const auto& c) { return c.legs == 3; });
    CHECK(t != found.end() && t->endpoint && t->roads.size() == 2);
    test::near(t->at.x, 50, 1e-9); test::near(t->at.y, 99.75, 1e-9);
    // Two Roads meeting end to end are a continuation, not a junction.
    road(d, {{400, 0}, {500, 0}}); road(d, {{500, 0}, {600, 0}});
    CHECK(findCrossings(d.network, 2).size() == 5);
    CHECK(thrown([&] { findCrossings(d.network, 0); }) == "EDIT_CROSSING_TOLERANCE");
    // Three Roads through one point are one place of six legs, not three pairs; a stem that overshoots
    // its Road by 0.5 m (a crossing and an end at one place) is one place of three legs.
    ProjectDocument star;
    road(star, {{-100, 0}, {100, 0}}); road(star, {{0, -100}, {0, 100}}); road(star, {{-100, -100}, {100, 100}});
    const auto six = findCrossings(star.network, 2);
    CHECK(six.size() == 1 && six[0].legs == 6 && six[0].roads.size() == 3);
    road(star, {{300, 0}, {500, 0}}); road(star, {{400, 50}, {400, -0.5}});
    const auto stem = findCrossings(star.network, 2);
    CHECK(stem.size() == 2 && stem[1].legs == 3 && stem[1].endpoint);
}
TEST(roadcrossings, rd13_junction_splits_and_adds_a_node) { // RD13
    ProjectDocument start;
    putCrossSection(start, divided(), {});
    const auto a = road(start, {{0, 0}, {200, 0}}, 0, "divided"), b = road(start, {{100, -100}, {100, 100}}, 0, "divided");
    const auto aLinks = applyTemplate(start, a, {}); applyTemplate(start, b, {});
    putRoute(start, {"", {aLinks[0]}});
    validateDocument(start);
    History h; h.reset(start);
    const auto before = documentJson(h.document()).dump();
    const auto found = findCrossings(h.document().network, 2);
    CHECK(found.size() == 1 && found[0].legs == 4); // forcing: one crossing
    std::vector<std::string> nodes;
    CHECK(h.execute("editorFindCrossings", [&](auto& d) { nodes = applyCrossings(d, found, all(1, CrossingChoice::junction), 2, {}); }));
    const auto& d = h.document();
    CHECK(d.network.roads.size() == 4 && d.network.links.size() == 8 && nodes.size() == 1);
    for (const auto& r : d.network.roads) {
        CHECK(r.links.size() == 2);
        for (const auto& id : r.links) {
            CHECK(link(d, id).roadId == r.id);
            // Each piece lies along its own Road: its middle within half the carriageway of the centreline.
            const auto& g = link(d, id).geometry;
            const auto mid = pointAlong(g, polylineLength(g) / 2);
            const auto near = pointAlong(r.centreline, stationOfClosestPoint(r.centreline, mid));
            CHECK(std::hypot(mid.x - near.x, mid.y - near.y) < 5);
        }
    }
    CHECK(named(d, a).centreline.back().x == 100 && named(d, a).links[0] == aLinks[0]);
    CHECK(d.definition->routes[0].segmentIds.size() == 2 && d.definition->routes[0].segmentIds[0] == aLinks[0]);
    validateDocument(d);
    const auto& node = d.network.nodes.at(0);
    CHECK(node.generated && node.polygon.size() == 16 && pointInPolygon(node.polygon, {100, 0}));
    // Radius: the forward Link's centre 4.25 m off plus half its 6.5 m, plus the widest lane.
    test::near(std::hypot(node.polygon[0].x - 100, node.polygon[0].y), 4.25 + 3.25 + 3.25, 1e-9);
    CHECK(nodeMembership(d.network, node).issues.empty());
    CHECK(findCrossings(d.network, 2).empty()); // the four ends meet inside the new Node: not asked again
    h.undo();
    CHECK(documentJson(h.document()).dump() == before);
}
TEST(roadcrossings, rd14_overpass) { // RD14
    ProjectDocument start;
    const auto a = road(start, {{0, 0}, {200, 0}}), b = road(start, {{100, -100}, {100, 100}});
    History h; h.reset(start);
    const auto found = findCrossings(h.document().network, 2);
    CHECK(found.size() == 1); // forcing
    CHECK(h.execute("editorFindCrossings", [&](auto& d) { applyCrossings(d, found, all(1, CrossingChoice::overpass), 2, {}); }));
    const auto& d = h.document();
    CHECK(d.network.overpasses == (std::vector<std::pair<std::string, std::string>>{{std::min(a, b), std::max(a, b)}}));
    CHECK(d.network.roads.size() == 2 && d.network.nodes.empty() && findCrossings(d.network, 2).empty());
    auto j = documentJson(d);
    CHECK(j["schemaVersion"] == 53 && j["network"]["overpasses"].size() == 1);
    CHECK(parseDocument(Json::parse(j.dump())).network.overpasses == d.network.overpasses);
    j["schemaVersion"] = 52; j["network"].erase("roads");
    try { parseDocument(j); CHECK(false); } catch (const ValidationError& e) { CHECK(issueAt(e.issues, "EDIT_UNSUPPORTED_FIELD", "network.overpasses")); }
    auto unknown = d; unknown.network.overpasses.push_back({a, "nope"});
    try { validateDocument(unknown); CHECK(false); } catch (const ValidationError& e) { CHECK(issueAt(e.issues, "ROAD_OVERPASS_UNKNOWN", "overpasses[1]")); }
    // A split carries the overpass to the new piece: a junction elsewhere on `a` does not bring it back.
    auto split = d; road(split, {{50, -50}, {50, 50}});
    const auto other = findCrossings(split.network, 2);
    CHECK(other.size() == 1);
    applyCrossings(split, other, all(1, CrossingChoice::junction), 2, {});
    CHECK(split.network.overpasses.size() == 2 && findCrossings(split.network, 2).empty());
    validateDocument(split);
    // An end cannot pass over; deleting a Road drops its pairs; Undo removes the pair.
    auto tee = d; road(tee, {{150, 30}, {150, 0.5}});
    const auto t = findCrossings(tee.network, 2);
    CHECK(t.size() == 1 && t[0].endpoint);
    CHECK(thrown([&] { auto x = tee; applyCrossings(x, t, all(1, CrossingChoice::overpass), 2, {}); }) == "EDIT_CROSSING_OVERPASS");
    auto gone = d; deleteRoad(gone, a);
    CHECK(gone.network.overpasses.empty());
    h.undo();
    CHECK(h.document().network.overpasses.empty());
}
TEST(roadcrossings, rd15_merge_nodes) { // RD15
    // Four generated discs around a small ring merge into one hull, generated, in one step.
    ProjectDocument start;
    for (const auto [x, y] : {std::pair{20.0, 0.0}, {0.0, 20.0}, {-20.0, 0.0}, {0.0, -20.0}}) {
        std::vector<Point> disc;
        for (int k = 0; k < 8; ++k) disc.push_back({x + 5 * std::cos(k * std::numbers::pi / 4), y + 5 * std::sin(k * std::numbers::pi / 4)});
        putNode(start, {"", "", disc, true});
    }
    History h; h.reset(start);
    const auto revision = h.revision();
    std::vector<std::string> ids; for (const auto& n : start.network.nodes) ids.push_back(n.id);
    CHECK(ids.size() == 4); // forcing
    CHECK(h.execute("editorMergeNodes", [&](auto& d) { mergeNodes(d, ids); }));
    const auto& merged = h.document().network.nodes;
    CHECK(merged.size() == 1 && merged[0].id == ids[0] && merged[0].generated && h.revision() == revision + 1);
    CHECK(pointInPolygon(merged[0].polygon, {0, 0}) && pointInPolygon(merged[0].polygon, {24.9, 0}) && simplePolygon(merged[0].polygon));
    CHECK(thrown([&] { auto x = start; mergeNodes(x, {ids[0]}); }) == "EDIT_NODE_MERGE");
    CHECK(thrown([&] { auto x = start; mergeNodes(x, {ids[0], "nope"}); }) == "EDIT_NODE_MERGE");
    // A merge that would make a Link leave and come back is refused, and nothing changes. The U-shaped
    // Road's junction Node holds valid detail; the hull with a disc on its far side makes it re-enter.
    ProjectDocument u;
    putCrossSection(u, divided(), {});
    const auto ring = road(u, {{0, 0}, {60, 0}, {60, 40}, {0, 40}}, 0, "divided"), stem = road(u, {{30, -50}, {30, 20}}, 0, "divided");
    applyTemplate(u, ring, {}); applyTemplate(u, stem, {});
    const auto found = findCrossings(u.network, 2);
    CHECK(found.size() == 1);
    const auto junction = applyCrossings(u, found, all(1, CrossingChoice::junction), 2, {}).at(0);
    CHECK(nodeMembership(u.network, u.network.nodes[0]).issues.empty()); // forcing: valid detail
    std::vector<Point> far;
    for (int k = 0; k < 8; ++k) far.push_back({30 + 5 * std::cos(k * std::numbers::pi / 4), 40 + 5 * std::sin(k * std::numbers::pi / 4)});
    const auto top = putNode(u, {"", "", far, true});
    const auto kept = u;
    try { mergeNodes(u, {junction, top}); CHECK(false); }
    catch (const ValidationError& e) { CHECK(issueAt(e.issues, "NODE_LINK_REENTERS", "nodes[0]")); }
    CHECK(u == kept);
}
TEST(roadcrossings, rd30_levels_never_share_a_place) { // RD30 (D382)
    // Two crossings at one x, y, one on each level: two places, each with its own level's Roads.
    ProjectDocument d;
    const auto a = road(d, {{-50, 0}, {50, 0}}), b = road(d, {{0, -50}, {0, 50}});
    const auto c = road(d, {{-50, 1}, {50, 1}}, 1), e = road(d, {{1, -50}, {1, 50}}, 1);
    const auto found = findCrossings(d.network, 2);
    CHECK(found.size() == 2); // forcing: both are within the tolerance of one another
    for (const auto& place : found) {
        CHECK(place.roads.size() == 2 && place.legs == 4);
        for (const auto& r : place.roads) CHECK(named(d, r.roadId).level == place.level);
    }
    CHECK(found.size() == 2 && found[0].level == 0 && found[1].level == 1);
    (void)a; (void)b; (void)c; (void)e;
}
