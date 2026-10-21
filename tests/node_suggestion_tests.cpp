#include "test.hpp"
#include "../src/commands/node_commands.hpp"
#include "../src/model/network/node_membership.hpp"
#include "../src/model/network/node_suggestions.hpp"
#include "../src/model/network/polygon.hpp"
#include "../src/project/document.hpp"
#include "../src/project/json.hpp"
#include <fstream>

using namespace multra;
// S0.5 (docs/reference/NODES.md §6, D265): Suggest nodes. Each row first checks its forcing.
namespace {
ProjectDocument shipped(const char* project) {
    std::ifstream file(test::root() / "data/projects" / project); Json j; file >> j; return parseDocument(j);
}
bool inside(const std::vector<Point>& polygon, const std::vector<Point>& line) {
    return std::all_of(line.begin(), line.end(), [&](Point p) { return pointInPolygon(polygon, p); });
}
}
TEST(nodesuggest, n18_the_four_leg_junction_gets_one_signalised_node) { // N18
    const auto d = shipped("four-leg-signalised.traffic.json");
    CHECK(d.network.nodes.empty() && d.network.rightOfWay.conflictAreas.empty()); // automatic conflicts only
    const auto s = nodeSuggestions(d.network);
    CHECK(s.size() == 1 && s[0].generated && s[0].id.empty() && simplePolygon(s[0].polygon));
    const auto m = nodeMembership(d.network, s[0]);
    CHECK(m.issues.empty() && m.inLegs.size() == 4 && m.outLegs.size() == 4 && m.turns.size() == 12);
    CHECK(m.control == SectionControl::signalised);
    for (const auto& c : d.network.connectors) {
        const bool turn = c.id >= "connector-49" && c.id <= "connector-60";
        CHECK(inside(s[0].polygon, connectorCentreline(d.network, c)) == turn);
    }
}
TEST(nodesuggest, n18_the_t_junction_gets_one_unsignalised_node) { // N18
    const auto d = shipped("t-junction-priority.traffic.json");
    CHECK(d.network.rightOfWay.conflictAreas.size() == 3);
    const auto s = nodeSuggestions(d.network);
    CHECK(s.size() == 1);
    const auto m = nodeMembership(d.network, s[0]);
    CHECK(m.issues.empty() && m.inLegs.size() == 3 && m.outLegs.size() == 2 && m.turns.size() == 4);
    CHECK(m.control == SectionControl::unsignalised && m.conflicts.size() == 3);
}
TEST(nodesuggest, n18_the_study_template_gets_one_node) { // N18
    const auto d = shipped("m2.6-study-template.traffic.json");
    CHECK(d.network.connectors.size() == 20);
    const auto s = nodeSuggestions(d.network);
    CHECK(s.size() == 1);
    const auto m = nodeMembership(d.network, s[0]);
    CHECK(m.issues.empty() && m.turns.size() == 12);
}
TEST(nodesuggest, n18_no_conflict_no_suggestion) { // N18
    const auto d = shipped("lane-change-lab.traffic.json");
    CHECK(!d.network.connectors.empty());
    CHECK(nodeSuggestions(d.network).empty());
}
TEST(nodesuggest, n18_covered_junctions_are_skipped) { // N18
    auto d = shipped("four-leg-signalised.traffic.json");
    const auto ids = suggestNodes(d);
    CHECK(ids.size() == 1 && d.network.nodes.size() == 1 && d.network.nodes[0].generated);
    CHECK(suggestNodes(d).empty() && d.network.nodes.size() == 1);
    // A hand-drawn Node over the junction, or over part of it, stops the suggestion.
    for (const double size : {25.0, 5.0}) {
        auto hand = shipped("four-leg-signalised.traffic.json");
        putNode(hand, {"", "Mine", {{-size, -size}, {size, -size}, {size, size}, {-size, size}}, false});
        CHECK(suggestNodes(hand).empty());
        CHECK(hand.network.nodes.size() == 1 && !hand.network.nodes[0].generated);
    }
}
namespace {
// The T-junction with a second minor approach `dx` metres east: its own Link, Connectors and
// conflict areas, joining the same major Links.
Network twinned(double dx) {
    const auto d = shipped("t-junction-priority.traffic.json");
    auto twin = d.network;
    for (auto& l : twin.links) if (l.id == "link-5") {
        auto copy = l; copy.id = "link-twin";
        for (auto& p : copy.geometry) p.x += dx;
        for (auto& lane : copy.lanes) lane.id += "-twin";
        twin.links.push_back(copy); break;
    }
    for (const auto& c : d.network.connectors) {
        auto copy = c; copy.id += "-twin"; copy.from.linkId = "link-twin"; copy.from.laneId += "-twin";
        // link-3 runs westbound, so east is a smaller station on it; link-1 a larger one.
        copy.to.station = *c.to.station + (c.to.linkId == "link-1" ? dx : -dx);
        for (auto& p : copy.geometry) p.x += dx;
        twin.connectors.push_back(copy);
        for (const auto& original : d.network.rightOfWay.conflictAreas) {
            if (original.first.path.connectorId != c.id) continue;
            auto area = original; area.id += "-" + copy.id;
            area.first.path.connectorId = copy.id; area.first.path.fromLaneId += "-twin";
            area.first.waitingLineId.clear(); area.second.waitingLineId.clear();
            const double shift = area.second.path.linkId == "link-1" ? dx : -dx;
            area.second.entryStation += shift; area.second.exitStation += shift;
            twin.rightOfWay.conflictAreas.push_back(area);
        }
    }
    return twin;
}
}
TEST(nodesuggest, n18_overlapping_suggestions_merge_and_apart_ones_do_not) { // N18
    // 44 m east: no conflict joins the two (the Connectors end 4 m apart), but the padded hulls overlap.
    const auto near = twinned(44), far = twinned(100);
    CHECK(near.connectors.size() == 4 && near.rightOfWay.conflictAreas.size() == 6); // the forcing
    CHECK(nodeSuggestions(near).size() == 1);
    // Connectors arriving on one Link at different places belong to different junctions.
    const auto apart = nodeSuggestions(far);
    CHECK(apart.size() == 2);
    for (const auto& node : apart) CHECK(nodeMembership(far, node).turns.size() == 4);
}
