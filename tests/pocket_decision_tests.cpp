// D501: a placed routing decision reaches a destination that only a pocket lane serves. The taper
// Connector Fill generates is 1 -> 2: its first lane path continues on the ordinary lane and its
// second enters the pocket. A decision's chain used to take the first only, stop short of the far
// turn and lose the destination with no issue.
#include "junction_signal_fixture.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/model/network/routeless.hpp"
#include "../src/project/demand_paths.hpp"
#include "../src/project/evaluation.hpp"
#include "../src/runner/batch.hpp"
#include <set>
using namespace multra;
using namespace junctionSignalTest;
namespace {
struct Pocketed { ProjectDocument d; std::vector<std::string> entries; };
// The P3 pocket template filled, with one routeless input and one decision per approach that
// names every exit a chain reaches (three: no U-turn is drawn).
Pocketed pocketed(DrivingSide side) {
    auto d = signalCross(side, "P3");
    std::vector<std::string> entries, exits;
    for (const auto& l : nodeLinkLegs(d.network, node(d))) (l.in ? entries : exits).push_back(l.linkId);
    CHECK(entries.size() == 4 && exits.size() == 4);
    History h; h.reset(d); apply(h); d = h.document();
    for (const auto& entry : entries) {
        VehicleInput input{"", "", "car", 900, 0, 240}; input.linkId = entry; putInput(d, input);
        RoutingDecision decision{"", "", {}, entry, {}, {}, std::nullopt};
        for (const auto& exit : exits)
            if (!routeShortestChains(d.network, entry, exit).empty()) decision.routes.push_back({"", 1, exit, {}});
        CHECK(decision.routes.size() == 3);
        putRoutingDecision(d, decision);
    }
    return {std::move(d), std::move(entries)};
}
bool endsOn(const Network& n, const std::string& laneId, const std::string& linkId) {
    for (const auto& l : n.links) if (l.id == linkId) for (const auto& lane : l.lanes) if (lane.id == laneId) return true;
    return false;
}
}
TEST(pocketdecision, every_destination_gets_a_full_chain) {
    for (const auto side : {DrivingSide::left, DrivingSide::right}) {
        const auto [d, entries] = pocketed(side);
        const auto issues = routelessIssues(d.network, *d.definition);
        CHECK(issues.blocking.empty() && issues.advisory.empty());
        const auto placed = placedDecisions(d.network, *d.definition);
        for (const auto& decision : d.definition->routingDecisions) {
            const auto walk = routelessChains(d.network, decision.linkId, placed);
            CHECK(walk.issues.empty() && walk.advisories.empty() && walk.byDestination);
            double share = 0;
            for (const auto& c : walk.chains) share += c.share;
            test::near(share, 1.0);
            for (const auto& route : decision.routes) { // each destination: a third of the input, on a full chain
                double to = 0; bool full = false;
                for (const auto& c : walk.chains) {
                    if (c.families.empty() || c.families.front().name != decision.id + ">" + route.destinationLinkId) continue;
                    to += c.share;
                    full = full || (!c.stub && endsOn(d.network, c.laneChain.back(), route.destinationLinkId));
                }
                CHECK(full);
                test::near(to, 1.0 / 3);
            }
        }
    }
}
TEST(pocketdecision, all_twelve_turns_carry_vehicles) {
    for (const auto side : {DrivingSide::left, DrivingSide::right}) {
        auto d = pocketed(side).d;
        changeRunSettings(d, 240, 0.1);
        const auto data = test::root() / "data"; const auto snapshot = compileDocument(d, data);
        const auto report = runSeed(snapshot.scenario, evaluationSpec(d, snapshot, data), 42).report;
        CHECK(report.nodes.size() == 1 && report.nodes[0].turns.size() == 12);
        for (const auto& turn : report.nodes[0].turns) CHECK(turn.vehicles > 0);
    }
}
TEST(pocketdecision, a_destination_no_lane_serves_is_named) {
    // A (2 lanes) -> B (2 lanes) by lane 1 only; B leaves to C from lane 2 only. The objects connect,
    // no lane path does, and a decision does not change lanes on a later Link: say so, do not drop it.
    ProjectDocument d; d.network.drivingSide = DrivingSide::left; d.definition = AuthoringDefinition{};
    const auto a = addLink(d, {{0, 0}, {100, 0}}, 2, 3.5), b = addLink(d, {{110, 0}, {210, 0}}, 2, 3.5),
               c = addLink(d, {{220, 10}, {320, 10}}, 1, 3.5), other = addLink(d, {{220, -10}, {320, -10}}, 1, 3.5);
    const auto lane = [&](const std::string& id, std::size_t k) {
        for (const auto& l : d.network.links) if (l.id == id) return l.lanes.at(k).id;
        throw std::logic_error("lane");
    };
    const auto ab = addConnectorRange(d, {a, lane(a, 0)}, {b, lane(b, 0)}, 1, 1);
    const auto bc = addConnectorRange(d, {b, lane(b, 1)}, {c, lane(c, 0)}, 1, 1);
    const auto bo = addConnectorRange(d, {b, lane(b, 0)}, {other, lane(other, 0)}, 1, 1);
    const std::vector<PlacedDecision> placed{{"decision", a, "routingDecisions[0]",
        {{{{a, ab, b, bc, c}}, 1.0}, {{{a, ab, b, bo, other}}, 3.0}}, std::nullopt}};
    const auto walk = routelessChains(d.network, a, placed);
    CHECK(std::count_if(walk.advisories.begin(), walk.advisories.end(), [](const auto& i) {
        return i.code == "ROUTING_DECISION_UNREACHABLE" && i.path == "routingDecisions[0]"; }) == 1);
    for (const auto& chain : walk.chains) CHECK(!endsOn(d.network, chain.laneChain.back(), c));
}
TEST(pocketdecision, a_static_route_keeps_its_first_path_and_lane_change) {
    // joining = true (routes) is untouched: the chain still takes the taper's first path and reaches the
    // pocket by D459's joined chain, never by the second path.
    const auto [d, entries] = pocketed(DrivingSide::left);
    bool checked = false;
    for (const auto& decision : placedDecisions(d.network, *d.definition))
        for (const auto& destination : decision.destinations) for (const auto& objects : destination.chains) {
            const auto family = routeLaneFamily(d.network, objects);
            if (std::none_of(family.begin(), family.end(), [](const auto& f) { return f.joined; })) continue;
            checked = true;
            for (const auto& f : family) if (!f.joined)
                for (const auto& id : f.ids) CHECK(id.find("/lane-") == std::string::npos || id.find(objects[1]) == std::string::npos);
        }
    CHECK(checked);
}
