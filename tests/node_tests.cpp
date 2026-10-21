#include "test.hpp"
#include "../src/commands/node_commands.hpp"
#include "../src/model/network/nodes.hpp"
#include "../src/model/network/polygon.hpp"
#include "../src/project/document.hpp"
#include "../src/project/json.hpp"
#include "../src/core/validate.hpp"
#include <fstream>

using namespace multra;
// S0.1 (docs/reference/NODES.md §2, D261): the Node object, its codec at schema 48 and validation.
namespace {
Json fileJson(const char* project) {
    std::ifstream file(test::root() / "data/projects" / project); Json j; file >> j; return j;
}
std::vector<Point> square(double x, double y, double size) { return {{x, y}, {x + size, y}, {x + size, y + size}, {x, y + size}}; }
bool refused(const Json& doc, const std::string& code, const std::string& path) {
    try { parseDocument(doc); } catch (const ValidationError& e) {
        return std::any_of(e.issues.begin(), e.issues.end(), [&](const auto& i) { return i.code == code && i.path == path; });
    }
    return false;
}
bool hasIssue(const Network& n, const std::string& code, const std::string& path) {
    const auto issues = validateNetwork(n);
    return std::any_of(issues.begin(), issues.end(), [&](const auto& i) { return i.code == code && i.path == path; });
}
}
TEST(nodes, n1_a_project_without_a_node_keeps_its_bytes) { // N1
    for (const char* project : {"four-leg-signalised.traffic.json", "t-junction-priority.traffic.json", "m2.6-study-template.traffic.json", "lane-change-lab.traffic.json"}) {
        const auto j = fileJson(project);
        const auto d = parseDocument(j);
        CHECK(d.network.nodes.empty());
        const auto saved = documentJson(d);
        CHECK(saved["schemaVersion"] == j["schemaVersion"]);
        CHECK(!saved["network"].contains("nodes"));
    }
}
TEST(nodes, n2_one_node_round_trips_at_schema_48) { // N2
    auto d = parseDocument(fileJson("four-leg-signalised.traffic.json"));
    const auto id = putNode(d, {"", "Centre", square(-20, -20, 40), false});
    CHECK(id.rfind("node-", 0) == 0);
    const auto j = documentJson(d);
    CHECK(j["schemaVersion"] == 48);
    CHECK(j["network"]["nodes"].size() == 1);
    CHECK(j["network"]["nodes"][0]["polygon"][1] == Json({{"x", 20.0}, {"y", -20.0}}));
    CHECK(parseDocument(Json::parse(j.dump())) == d);
    auto older = j; older["schemaVersion"] = 47;
    CHECK(refused(older, "EDIT_UNSUPPORTED_FIELD", "network.nodes"));
    auto stray = j; stray["network"]["nodes"][0]["colour"] = "red";
    CHECK(refused(stray, "EDIT_NODE_INVALID", "network.nodes[0].colour"));
    auto badPoint = j; badPoint["network"]["nodes"][0]["polygon"][2] = Json::array({1, 2});
    CHECK(refused(badPoint, "EDIT_NODE_INVALID", "network.nodes[0].polygon[2]"));
    // The id is taken: a second allocation skips it, and another object cannot reuse it.
    CHECK(putNode(d, {"", "", square(100, 100, 10), false}) != id);
    deleteNode(d, id); deleteNode(d, d.network.nodes.front().id);
    CHECK(documentJson(d)["schemaVersion"] < 45); // back to the file's own schema
    test::throws([&] { deleteNode(d, id); }, "EDIT_UNKNOWN_OBJECT");
}
TEST(nodes, n3_bad_polygons_and_overlap_are_refused_at_their_paths) { // N3
    Network n; n.id = "net";
    n.nodes = {{"a", "", square(0, 0, 10), false}};
    CHECK(nodeIssues(n).empty()); // forcing: a good one passes
    for (const auto& polygon : std::vector<std::vector<Point>>{
             {{0, 0}, {1, 0}},                                         // two points
             {{0, 0}, {1, 0}, {std::numeric_limits<double>::quiet_NaN(), 1}}, // not finite
             {{0, 0}, {10, 10}, {10, 0}, {0, 10}}}) {                   // a bow tie crosses itself
        n.nodes = {{"a", "", polygon, false}};
        CHECK(hasIssue(n, "NODE_POLYGON_INVALID", "nodes[0].polygon"));
    }
    n.nodes = {{"a", "", square(0, 0, 10), false}, {"b", "", square(5, 5, 10), false}};
    CHECK(hasIssue(n, "NODE_OVERLAP", "nodes[1].polygon"));
    n.nodes[1].polygon = square(2, 2, 3); // wholly inside
    CHECK(hasIssue(n, "NODE_OVERLAP", "nodes[1].polygon"));
    n.nodes[1].polygon = square(20, 0, 10); // apart
    CHECK(!hasIssue(n, "NODE_OVERLAP", "nodes[1].polygon"));
    n.nodes[1].id = "a";
    CHECK(hasIssue(n, "DUPLICATE_ID", "nodes[1].id"));
    n.nodes[1].id = "net";
    CHECK(hasIssue(n, "DUPLICATE_ID", "nodes[1].id")); // the network's own id is taken
    n.nodes[1].id = " ";
    CHECK(hasIssue(n, "INVALID_ID", "nodes[1].id"));
    // The shared helper is the one zones use: the bow tie is not simple there either.
    CHECK(!simplePolygon({{0, 0}, {10, 10}, {10, 0}, {0, 10}}));
}
TEST(nodes, n4_generated_is_written_only_when_true_and_an_edit_clears_it) { // N4
    auto d = parseDocument(fileJson("four-leg-signalised.traffic.json"));
    const auto suggested = putNode(d, {"", "", square(-20, -20, 40), true});
    const auto drawn = putNode(d, {"", "", square(100, 100, 10), false});
    auto j = documentJson(d);
    CHECK(j["network"]["nodes"][0]["generated"] == true); // forcing: an allocated Node keeps the flag
    CHECK(!j["network"]["nodes"][1].contains("generated"));
    auto written = j; written["network"]["nodes"][1]["generated"] = false;
    CHECK(refused(written, "EDIT_NODE_INVALID", "network.nodes[1].generated")); // never written false
    auto moved = d.network.nodes.front(); moved.polygon[0] = {-25, -25};
    putNode(d, moved);
    CHECK(!d.network.nodes.front().generated);
    d.network.nodes.front().generated = true;
    renameNode(d, suggested, "Centre");
    CHECK(!d.network.nodes.front().generated);
    CHECK(d.network.nodes.front().name == "Centre");
    (void)drawn;
}
