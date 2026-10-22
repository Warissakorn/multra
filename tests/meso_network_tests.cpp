// M11.2 rows ME1-ME2 (docs/reference/MESO_NETWORK.md §3, §4, §8, D292, D293): the Meso graph from the
// drawn network and its Nodes, a link type's jamDensity and mesoSettings at schema 52, and the
// refusals. Each row first checks its forcing.
#include "test.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/commands/node_commands.hpp"
#include "../src/model/network/node_membership.hpp"
#include "../src/model/network/polygon.hpp"
#include "../src/project/meso_input.hpp"
#include <cmath>
#include <filesystem>
#include <fstream>
#include <set>
using namespace multra;
namespace {
std::filesystem::path data() { return test::root() / "data"; }
Json fileJson(const char* project) { std::ifstream file(data() / "projects" / project); Json j; file >> j; return j; }
Json reread(const ProjectDocument& d) { return Json::parse(documentJson(d).dump()); }
bool refusedAt(const Json& j, const std::string& code, const std::string& path) {
    try { parseDocument(j); } catch (const ValidationError& e) {
        return std::any_of(e.issues.begin(), e.issues.end(), [&](const auto& i) { return i.code == code && i.path == path; });
    } catch (...) {}
    return false;
}
bool issueAt(const std::vector<ValidationIssue>& issues, const std::string& code, const std::string& path) {
    return std::any_of(issues.begin(), issues.end(), [&](const auto& i) { return i.code == code && i.path == path; });
}
// Test values only: no type ships (D292 2a). 1000 pcu/h/lane at 10 m/s is 27.8 pcu/km critical.
LinkType arterial(std::optional<JamDensity> jam = JamDensity{150, "Test value"}) {
    return {"arterial", "Arterial", 1000, 10, "bpr", {}, "Test values", jam};
}
ProjectDocument typed(const char* project, const LinkType& type = arterial()) {
    auto d = parseDocument(fileJson(project));
    auto& def = demand(d); def.externalLinkTypes = false; def.linkTypes = {type};
    for (auto& l : d.network.links) l.linkTypeId = type.id;
    return d;
}
Point pointAt(const std::vector<Point>& line, double s) {
    for (std::size_t i = 0; i + 1 < line.size(); ++i) {
        const double length = std::hypot(line[i + 1].x - line[i].x, line[i + 1].y - line[i].y);
        if (s <= length || i + 2 == line.size()) {
            const double t = length > 0 ? std::clamp(s / length, 0.0, 1.0) : 0.0;
            return {line[i].x + t * (line[i + 1].x - line[i].x), line[i].y + t * (line[i + 1].y - line[i].y)};
        }
        s -= length;
    }
    return line.front();
}
const Link& link(const ProjectDocument& d, const std::string& id) {
    for (const auto& l : d.network.links) if (l.id == id) return l;
    throw std::runtime_error("no link " + id);
}
// The one drawn Node compiled as one Meso node with membership's turns, legs and conflicts.
void checkNode(const ProjectDocument& d, std::size_t turns) {
    const auto& node = d.network.nodes.at(0);
    const auto m = nodeMembership(d.network, node);
    CHECK(m.issues.empty() && m.turns.size() == turns); // forcing
    const auto c = mesoNetwork(d, data());
    CHECK(c.issues.empty());
    const auto& g = c.graph;
    const auto n = std::find_if(g.nodes.begin(), g.nodes.end(), [&](const auto& x) { return x.nodeId == node.id; });
    CHECK(n != g.nodes.end());
    CHECK(std::count_if(g.nodes.begin(), g.nodes.end(), [](const auto& x) { return !x.nodeId.empty(); }) == 1);
    CHECK(n->turns.size() == turns);
    for (std::size_t k = 0; k < turns; ++k) {
        const auto& t = g.turns[n->turns[k]]; const auto& mt = m.turns[k];
        CHECK(t.name == mt.name);
        CHECK(g.links[t.in].source == m.inLegs[mt.in].linkId && std::abs(g.links[t.in].toStation - m.inLegs[mt.in].station) < 1e-9);
        CHECK(g.links[t.out].source == m.outLegs[mt.out].linkId && std::abs(g.links[t.out].fromStation - m.outLegs[mt.out].station) < 1e-9);
        CHECK(t.freeTime > 0 && !t.via.empty());
    }
    CHECK(n->conflicts.size() == m.conflicts.size());
    for (std::size_t k = 0; k < m.conflicts.size(); ++k)
        CHECK(n->conflicts[k].first == n->turns[m.conflicts[k].first] && n->conflicts[k].second == n->turns[m.conflicts[k].second]);
    for (const auto& l : g.links) // nothing inside the polygon
        CHECK(!pointInPolygon(node.polygon, pointAt(link(d, l.source).geometry, (l.fromStation + l.toStation) / 2)));
    for (const auto& t : g.turns) CHECK(g.links[t.in].to == t.node && g.links[t.out].from == t.node);
}
}
TEST(mesonetwork, me1_nodes_and_links_from_the_drawing) { // ME1
    auto tee = typed("t-junction-priority.traffic.json");
    putNode(tee, {"", "Centre", {{-25, -18}, {25, -18}, {25, 8}, {-25, 8}}, false});
    checkNode(tee, 4);
    auto four = typed("four-leg-signalised.traffic.json");
    CHECK(suggestNodes(four).size() == 1);
    checkNode(four, 12);
    const auto fourGraph = mesoNetwork(four, data()).graph;
    CHECK(std::any_of(fourGraph.nodes.begin(), fourGraph.nodes.end(), [](const auto& x) { return !x.nodeId.empty() && x.control == "signalised"; }));
    // Without a Node: simple nodes only; every Connector is in exactly one turn; one turn per (in, out).
    const auto plain = typed("t-junction-priority.traffic.json");
    const auto g = mesoNetwork(plain, data()).graph;
    CHECK(std::all_of(g.nodes.begin(), g.nodes.end(), [](const auto& x) { return x.nodeId.empty(); }));
    std::multiset<std::string> via; std::set<std::pair<std::size_t, std::size_t>> pairs;
    for (const auto& t : g.turns) { via.insert(t.via.begin(), t.via.end()); CHECK(pairs.insert({t.in, t.out}).second); }
    CHECK(!plain.network.connectors.empty());
    for (const auto& c : plain.network.connectors) CHECK(via.count(c.id) == 1);
    // Two lane Connectors between the same two Links make one turn naming both.
    ProjectDocument pair;
    const auto a = addLink(pair, {{0, 0}, {100, 0}}, 2, 3.5), b = addLink(pair, {{120, 0}, {220, 0}}, 2, 3.5);
    for (std::size_t k = 0; k < 2; ++k) addConnector(pair, {a, link(pair, a).lanes[k].id, 100}, {b, link(pair, b).lanes[k].id, 0});
    auto& pairDef = demand(pair); pairDef.externalLinkTypes = false; pairDef.linkTypes = {arterial()};
    for (auto& l : pair.network.links) l.linkTypeId = "arterial";
    CHECK(pair.network.connectors.size() == 2); // forcing
    const auto merged = mesoNetwork(pair, data()).graph;
    CHECK(merged.turns.size() == 1 && merged.turns[0].via.size() == 2 && merged.nodes.size() == 3);
    // A Link that continues through a Connector station keeps a through turn.
    CHECK(std::any_of(g.turns.begin(), g.turns.end(), [&](const auto& t) {
        return t.via.empty() && g.links[t.in].source == g.links[t.out].source; }));
    CHECK(!g.entries.empty() && !g.exits.empty());
    // A refused Node over the junction (a Connector crosses its edge) is reported, and its interior
    // compiled as if it were not drawn.
    auto refused = plain;
    putNode(refused, {"", "Cut", {{-25, -18}, {10, -18}, {10, 8}, {-25, 8}}, false}); // connector-8 leaves through x = 10
    const auto cut = nodeMembership(refused.network, refused.network.nodes[0]);
    CHECK(std::find(cut.issues.begin(), cut.issues.end(), "NODE_CONNECTOR_CROSSES") != cut.issues.end()); // forcing
    const auto r = mesoNetwork(refused, data());
    CHECK(issueAt(r.issues, "MESO_NODE_REFUSED", "network.nodes[0]"));
    CHECK(r.graph == g);
}
TEST(mesonetwork, me2_jam_density_settings_and_refusals) { // ME2
    // A type without a jam density cannot run Meso.
    const auto bare = typed("t-junction-priority.traffic.json", arterial(std::nullopt));
    CHECK(!bare.definition->linkTypes[0].jamDensity); // forcing
    CHECK(issueAt(mesoNetwork(bare, data()).issues, "MESO_JAM_DENSITY_MISSING", "definition.linkTypes[0].jamDensity"));
    CHECK(reread(bare)["schemaVersion"] == 50);
    // jamDensity and mesoSettings round-trip at 52 and are refused below it.
    auto d = typed("t-junction-priority.traffic.json");
    demand(d).mesoSettings = MesoSettings{0.5, 60};
    const auto j = reread(d);
    CHECK(j["schemaVersion"] == 52);
    CHECK(j["definition"]["linkTypes"][0]["jamDensity"] == Json({{"value", 150.0}, {"source", "Test value"}}));
    CHECK(j["definition"]["mesoSettings"] == Json({{"step", 0.5}, {"slice", 60.0}}));
    CHECK(parseDocument(j) == d);
    auto older = j; older["schemaVersion"] = 51;
    CHECK(refusedAt(older, "EDIT_UNSUPPORTED_FIELD", "definition.mesoSettings"));
    older["definition"].erase("mesoSettings");
    CHECK(refusedAt(older, "EDIT_UNSUPPORTED_FIELD", "definition.linkTypes[0].jamDensity"));
    // A jam density at or below the critical density, or without a source.
    for (const auto& [field, value] : std::vector<std::pair<std::string, Json>>{{"value", 27.0}, {"value", -1.0}, {"source", " "}}) {
        auto bad = j; bad["definition"]["linkTypes"][0]["jamDensity"][field] = value;
        CHECK(refusedAt(bad, "INVALID_LINK_TYPE", "definition.linkTypes[0].jamDensity." + field));
    }
    auto above = j; above["definition"]["linkTypes"][0]["jamDensity"]["value"] = 28.0; // just above 27.8
    CHECK(parseDocument(above).definition->linkTypes[0].jamDensity->value == 28.0);
    // Bad settings at their field.
    for (const auto& [field, value] : std::vector<std::pair<std::string, Json>>{{"step", 0.0}, {"step", 61.0}, {"slice", 0.75}, {"slice", 0.0}}) {
        auto bad = j; bad["definition"]["mesoSettings"][field] = value;
        CHECK(refusedAt(bad, "INVALID_MESO_SETTING", "definition.mesoSettings." + field));
    }
    auto unknown = j; unknown["definition"]["mesoSettings"]["cfl"] = 1;
    CHECK(refusedAt(unknown, "INVALID_MESO_SETTING", "definition.mesoSettings.cfl"));
    // The step limit: the shortest link's free-flow time.
    const auto g = mesoNetwork(d, data()).graph;
    const double limit = mesoStepLimit(g);
    CHECK(limit > 0 && mesoShortestLink(g) < g.links.size());
    auto slow = d; demand(slow).mesoSettings = MesoSettings{limit * 1.01, limit * 1.01};
    CHECK(issueAt(mesoNetwork(slow, data()).issues, "MESO_STEP_TOO_LONG", "definition.mesoSettings.step"));
    auto fine = d; demand(fine).mesoSettings = MesoSettings{limit, limit};
    CHECK(!issueAt(mesoNetwork(fine, data()).issues, "MESO_STEP_TOO_LONG", "definition.mesoSettings.step"));
    // Without a Meso field every shipped project keeps its schema.
    for (const char* project : {"four-leg-signalised.traffic.json", "t-junction-priority.traffic.json", "m2.6-study-template.traffic.json", "lane-change-lab.traffic.json"}) {
        const auto file = fileJson(project);
        const auto saved = reread(parseDocument(file));
        CHECK(saved["schemaVersion"] == file["schemaVersion"] && !saved["definition"].contains("mesoSettings"));
    }
}
