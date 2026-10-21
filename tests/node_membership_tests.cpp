#include "test.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/commands/node_commands.hpp"
#include "../src/model/network/node_membership.hpp"
#include "../src/project/document.hpp"
#include "../src/project/json.hpp"
#include "../src/project/run.hpp"
#include <fstream>

using namespace multra;
// S0.2 (docs/reference/NODES.md §3, D262, D263): what a Node holds, derived from the network
// inside its polygon. Each row first checks that its forcing took.
namespace {
ProjectDocument shipped(const char* project) {
    std::ifstream file(test::root() / "data/projects" / project); Json j; file >> j; return parseDocument(j);
}
Node box(double x0, double y0, double x1, double y1) { return {"", "", {{x0, y0}, {x1, y0}, {x1, y1}, {x0, y1}}, false}; }
std::vector<std::string> names(const NodeMembership& m) {
    std::vector<std::string> result;
    for (const auto& t : m.turns) result.push_back(t.name);
    return result;
}
bool has(const std::vector<std::string>& items, const std::string& item) { return std::find(items.begin(), items.end(), item) != items.end(); }
}
TEST(nodemembership, n6_the_four_leg_junction_has_its_legs_turns_and_signals) { // N6
    const auto d = shipped("four-leg-signalised.traffic.json");
    CHECK(d.network.connectors.size() == 20 && d.network.signalHeads.size() == 12); // the forcing: the shipped drawing
    const auto m = nodeMembership(d.network, box(-25, -25, 25, 25));
    CHECK(m.issues.empty());
    CHECK(m.inLegs.size() == 4 && m.outLegs.size() == 4);
    for (const auto& leg : m.inLegs) CHECK(leg.name.find("right-turn pocket") != std::string::npos);
    for (const auto& leg : m.outLegs) CHECK(leg.name.find("exit") != std::string::npos);
    CHECK(m.turns.size() == 12);
    CHECK(has(names(m), "West approach, right-turn pocket → East exit"));
    CHECK(m.connectors.size() == 12 && m.links.empty());
    CHECK(m.signalHeads.size() == 12);
    CHECK(m.control == SectionControl::signalised);
    CHECK(m.conflicts.empty());
    // Heads 1 m upstream of a tighter boundary still count: they stand within the delay window.
    const auto tight = nodeMembership(d.network, box(-16.5, -16.5, 16.5, 16.5));
    CHECK(tight.turns.size() == 12 && tight.signalHeads.size() == 12 && tight.control == SectionControl::signalised);
    // An object chain no lane can drive is not a turn: a Connector leaving a lane its Link lacks.
    auto broken = d.network;
    for (auto& c : broken.connectors) if (c.id == "connector-49") c.from.laneId = "lane-missing";
    CHECK(nodeMembership(broken, box(-25, -25, 25, 25)).turns.size() == 11);
}
TEST(nodemembership, n7_the_t_junction_runs_its_major_links_through) { // N7
    const auto d = shipped("t-junction-priority.traffic.json");
    CHECK(d.network.links.size() == 3 && d.network.rightOfWay.conflictAreas.size() == 3);
    const auto m = nodeMembership(d.network, box(-25, -18, 25, 8));
    CHECK(m.issues.empty());
    CHECK(m.inLegs.size() == 3 && m.outLegs.size() == 2);
    CHECK(names(m) == std::vector<std::string>({"Major eastbound → Major eastbound", "Major westbound → Major westbound",
                                                "Minor approach → Major eastbound", "Minor approach → Major westbound"}));
    CHECK(std::abs(m.inLegs[0].station - 125) < 1e-6 && std::abs(m.outLegs[0].station - 175) < 1e-6);
    CHECK(m.control == SectionControl::unsignalised);
    CHECK(m.conflictAreas.size() == 3);
    // Minor crosses eastbound: minor → westbound against the eastbound through movement.
    CHECK(m.conflicts == std::vector<NodeConflict>({{"conflict-11", 3, 0, ConflictKind::crossing, ConflictPriority::firstYields},
                                                    {"conflict-15", 3, 1, ConflictKind::merge, ConflictPriority::firstYields},
                                                    {"conflict-19", 2, 0, ConflictKind::merge, ConflictPriority::firstYields}}));
}
TEST(nodemembership, n8_an_unconnected_overpass_is_ignored) { // N8
    auto d = shipped("four-leg-signalised.traffic.json");
    const auto before = nodeMembership(d.network, box(-25, -25, 25, 25));
    auto bridge = d.network.links.front();
    bridge.id = "link-bridge"; bridge.level = 1; bridge.geometry = {{-100, 0}, {100, 0}};
    d.network.links.push_back(bridge);
    CHECK(d.network.links.back().geometry.size() == 2);
    const auto m = nodeMembership(d.network, box(-25, -25, 25, 25));
    CHECK(m == before);
    for (const auto& leg : m.inLegs) CHECK(leg.linkId != "link-bridge");
}
TEST(nodemembership, n9_reentry_and_a_cut_connector_are_refused_but_run) { // N9
    auto d = shipped("four-leg-signalised.traffic.json");
    auto bent = d.network;
    for (auto& l : bent.links)
        if (l.id == "link-8") l.geometry = {{-16, -3.75}, {-40, -3.75}, {-40, -12}, {-20, -12}, {-20, -40}, {-300, -40}};
    const auto reenters = nodeMembership(bent, box(-25, -25, 25, 25));
    CHECK(reenters.issues == std::vector<std::string>({"NODE_LINK_REENTERS"}));
    CHECK(reenters.turns.empty() && reenters.inLegs.empty() && !reenters.control);

    const auto id = putNode(d, box(-10, -10, 10, 10));
    const auto cut = nodeMembership(d.network, d.network.nodes.front());
    CHECK(cut.issues == std::vector<std::string>({"NODE_CONNECTOR_CROSSES"}));
    CHECK(cut.turns.empty());
    const auto rows = runDiagnostics(d, test::root() / "data");
    CHECK(std::any_of(rows.begin(), rows.end(), [&](const auto& r) {
        return r.code == "NODE_CONNECTOR_CROSSES" && r.path == "nodes[0]" && r.severity == DiagnosticSeverity::advisory && r.objectId == id;
    }));
    CHECK(!compileDocument(d, test::root() / "data").scenario.segments.empty());
}
TEST(nodemembership, n10_a_polygon_with_no_turn) { // N10
    const auto d = shipped("four-leg-signalised.traffic.json");
    CHECK(nodeMembership(d.network, box(100, 100, 120, 120)).issues == std::vector<std::string>({"NODE_NO_TURN"}));
    // Across the middle of the West approach: it crosses, but nothing inside connects to it.
    const auto across = nodeMembership(d.network, box(-210, 0, -190, 20));
    CHECK(across.issues == std::vector<std::string>({"NODE_NO_TURN"}));
    CHECK(across.inLegs.empty());
}
TEST(nodemembership, n11_membership_follows_network_edits) { // N11
    auto d = shipped("four-leg-signalised.traffic.json");
    putNode(d, box(-25, -25, 25, 25));
    const auto node = d.network.nodes.front();
    const auto at = [&] { return nodeMembership(d.network, d.network.nodes.front()); };
    CHECK(at().turns.size() == 12);
    splitLink(d, "link-4", 20);
    CHECK(d.network.links.size() == 13); // the split took
    CHECK(at().inLegs.size() == 4 && at().turns.size() == 12 && at().issues.empty());
    const auto split = at();
    CHECK(std::none_of(split.inLegs.begin(), split.inLegs.end(), [](const auto& l) { return l.linkId == "link-4"; }));
    deleteObjects(d, {"link-8"}); // with its Connectors, heads and routes
    CHECK(at().outLegs.size() == 3 && at().turns.size() == 9);
    // An exit whose Connectors are gone connects to nothing inside: no longer a leg, reversed or not.
    deleteObjects(d, {"connector-49", "connector-57", "connector-59"});
    CHECK(at().outLegs.size() == 2 && at().turns.size() == 6);
    reverseLink(d, "link-20");
    const auto east = std::find_if(d.network.links.begin(), d.network.links.end(), [](const auto& l) { return l.id == "link-20"; });
    CHECK(east->geometry.front().x == 300.0); // the reversal took
    const auto reversed = at();
    CHECK(reversed.outLegs.size() == 2 && reversed.inLegs.size() == 4 && reversed.turns.size() == 6);
    CHECK(std::none_of(reversed.inLegs.begin(), reversed.inLegs.end(), [](const auto& l) { return l.linkId == "link-20"; }));
    CHECK(d.network.nodes.size() == 1 && d.network.nodes.front() == node);
}
