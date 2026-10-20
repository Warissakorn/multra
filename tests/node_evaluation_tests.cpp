#include "test.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/commands/node_commands.hpp"
#include "../src/eval/los.hpp"
#include "../src/model/network/node_membership.hpp"
#include "../src/model/network/travel_time.hpp"
#include "../src/project/batch_output.hpp"
#include "../src/project/document.hpp"
#include "../src/project/evaluation.hpp"
#include "../src/project/json.hpp"
#include "../src/project/los_output.hpp"
#include "../src/project/node_output.hpp"
#include "../src/runner/batch.hpp"
#include <fstream>

using namespace trafficsim;
// S0.3 (docs/reference/NODES.md §4, D264): Micro node evaluation. Each row first checks that its
// forcing took.
namespace {
ProjectDocument shipped(const char* project) {
    std::ifstream file(test::root() / "data/projects" / project); Json j; file >> j; return parseDocument(j);
}
Node box(double x0, double y0, double x1, double y1) { return {"", "Centre", {{x0, y0}, {x1, y0}, {x1, y1}, {x0, y1}}, false}; }
ProjectDocument withNode(const char* project, Node node) { auto d = shipped(project); putNode(d, std::move(node)); return d; }
ProjectDocument tJunction() { return withNode("t-junction-priority.traffic.json", box(-25, -18, 25, 8)); }
struct Run { RunSnapshot snapshot; EvaluationSpec spec; };
Run compiled(const ProjectDocument& d) {
    const auto data = test::root() / "data";
    auto snapshot = compileDocument(d, data);
    auto spec = evaluationSpec(d, snapshot, data);
    return {std::move(snapshot), std::move(spec)};
}
MovementReport run(const ProjectDocument& d, std::uint32_t seed = 42) {
    const auto c = compiled(d);
    return runSeed(c.snapshot.scenario, c.spec, seed).report;
}
}
TEST(nodeevaluation, n12_no_node_keeps_every_output) { // N12
    for (const char* project : {"four-leg-signalised.traffic.json", "t-junction-priority.traffic.json"}) {
        const auto d = shipped(project);
        CHECK(d.network.nodes.empty());
        const auto c = compiled(d);
        CHECK(c.spec.nodes.empty());
        const auto runs = runSeeds(c.snapshot.scenario, c.spec, {42, 43});
        CHECK(runs.front().report.nodes.empty());
        CHECK(!movementJson(runs.front().report).contains("nodes"));
        CHECK(movementCsv(runs.front().report).find("# Nodes") == std::string::npos);
        const auto batch = aggregate(runs);
        CHECK(batch.nodes.empty() && !batchJson(batch, runs).contains("nodes"));
        CHECK(batchCsv(batch, runs).find("# Nodes") == std::string::npos);
    }
}
TEST(nodeevaluation, n13_a_turn_runs_from_its_in_leg_start_to_its_out_leg_crossing) { // N13
    const auto d = tJunction();
    const auto m = nodeMembership(d.network, d.network.nodes.front());
    CHECK(m.turns.size() == 4); // the forcing: S0.2's membership
    const auto c = compiled(d);
    CHECK(c.spec.nodes.size() == 1 && c.spec.nodes[0].turns.size() == 4 && c.spec.nodes[0].legs.size() == 3);
    const auto table = runtimeSections(c.snapshot.network);
    const auto line = [&](const std::string& link, double station) {
        std::vector<CounterLine> result;
        for (const auto& at : locateSectionLine(c.snapshot.network, table, {link, station})) result.push_back({at.segment, at.position});
        return result;
    };
    for (std::size_t k = 0; k < 4; ++k) {
        const auto& turn = c.spec.nodes[0].turns[k];
        CHECK(turn.section.start == line(m.inLegs[m.turns[k].in].linkId, 0.0));
        CHECK(turn.section.end == line(m.outLegs[m.turns[k].out].linkId, m.outLegs[m.turns[k].out].station));
        CHECK(turn.section.controlType == std::optional<std::string>("unsignalised"));
    }
    const auto r = run(d);
    CHECK(r.sections.empty()); // turns are not authored sections
    CHECK(r.nodes.size() == 1 && r.nodes[0].turns.size() == 4);
    // Each turn counts only its own movement's vehicles: never more than that movement completed.
    std::uint64_t total = 0;
    for (const auto& t : r.nodes[0].turns) {
        CHECK(t.vehicles > 0);
        const auto movement = std::find_if(r.movements.begin(), r.movements.end(), [&](const auto& mv) { return mv.name == t.name; });
        CHECK(movement != r.movements.end() && t.vehicles <= movement->vehicles + movement->unfinished);
        total += t.vehicles;
    }
    CHECK(total == r.nodes[0].vehicles);
}
TEST(nodeevaluation, n14_a_free_flowing_turn_has_no_delay) { // N14
    // Vehicles enter from rest, so a turn whose in-leg starts at a source carries the start-up loss
    // (about 3 s on the T-junction's major roads, D264). Split the eastbound Link 100 m in, so the
    // in-leg starts where a lone vehicle already runs at its desired speed.
    auto d = shipped("t-junction-priority.traffic.json");
    d.network.rightOfWay = {}; d.network.queueCounters = {}; // they name the unsplit Link; one stream needs none
    const auto downstream = splitLink(d, "link-1", 100);
    putNode(d, box(-25, -18, 25, 8));
    auto& inputs = d.definition->inputs;
    const auto through = std::find_if(inputs.begin(), inputs.end(), [](const auto& i) { return i.id == "input-28"; });
    const auto kept = *through; inputs = {kept};
    inputs.front().vehiclesPerHour = 60; // one vehicle a minute: none meets another
    auto& routes = d.definition->routes;
    routes.erase(std::remove_if(routes.begin(), routes.end(), [&](const auto& r) { return r.id != kept.routeId; }), routes.end());
    CHECK(d.definition->inputs.size() == 1 && routes.size() == 1 && !downstream.empty());
    const auto m = nodeMembership(d.network, d.network.nodes.front());
    CHECK(std::any_of(m.inLegs.begin(), m.inLegs.end(), [&](const auto& l) { return l.linkId == downstream; }));
    const auto r = run(d);
    const auto lone = std::find_if(r.nodes[0].turns.begin(), r.nodes[0].turns.end(), [](const auto& t) { return t.vehicles > 0; });
    CHECK(lone != r.nodes[0].turns.end());
    CHECK(std::count_if(r.nodes[0].turns.begin(), r.nodes[0].turns.end(), [](const auto& t) { return t.vehicles > 0; }) == 1);
    CHECK(*lone->delay <= 0.1);
    // At the source itself the same lone vehicles show the start-up loss, well above 0.1 s.
    auto atSource = tJunction();
    atSource.definition->inputs = d.definition->inputs;
    CHECK(*run(atSource).nodes[0].turns[0].delay > 1.0);
}
TEST(nodeevaluation, n15_leg_and_node_rows_weight_turns_by_vehicles) { // N15
    NodeSpec spec{"N", "signalised", {{"A", {}}, {"B", {}}}, {}, {}, {}};
    for (const auto& [name, leg] : std::vector<std::pair<std::string, std::size_t>>{{"A1", 0}, {"A2", 0}, {"B1", 1}})
        spec.turns.push_back({{name, {}, {}, {}, {}}, leg});
    const std::vector<SectionRow> turns{{"A1", 3, 20.0, 10.0, 1}, {"A2", 1, 30.0, 2.0, 0}, {"B1", 0, {}, {}, 2}};
    const auto rows = nodeRows({spec}, turns, {{1.5, 4.0}, {0.0, 0.0}});
    CHECK(rows.size() == 1 && rows[0].legs.size() == 2 && rows[0].turns.size() == 3);
    test::near(*rows[0].legs[0].delay, (3 * 10.0 + 1 * 2.0) / 4, 1e-12);
    CHECK(rows[0].legs[0].vehicles == 4 && rows[0].legs[0].unfinished == 1);
    CHECK(!rows[0].legs[1].delay && rows[0].legs[1].unfinished == 2); // no vehicle, no delay
    test::near(*rows[0].delay, 32.0 / 4, 1e-12);
    CHECK(rows[0].vehicles == 4 && rows[0].unfinished == 3);
    CHECK(rows[0].legs[0].queueMean == 1.5 && rows[0].legs[0].queueMax == 4.0);
    // The same formula holds over a real run's own turn rows.
    const auto r = run(tJunction());
    std::vector<std::pair<std::uint64_t, std::optional<double>>> all;
    for (const auto& t : r.nodes[0].turns) all.push_back({t.vehicles, t.delay});
    test::near(*r.nodes[0].delay, *weightedDelay(all), 1e-12);
}
TEST(nodeevaluation, n16_leg_queue_letters_and_csv_marker) { // N16
    auto d = withNode("four-leg-signalised.traffic.json", box(-25, -25, 25, 25));
    const auto m = nodeMembership(d.network, d.network.nodes.front());
    CHECK(m.inLegs.size() == 4);
    // A hand-placed counter at the same line as the first leg's: every lane at its crossing.
    const auto& leg = m.inLegs.front();
    const auto link = std::find_if(d.network.links.begin(), d.network.links.end(), [&](const auto& l) { return l.id == leg.linkId; });
    AuthoredQueueCounter hand{"queue-900", "Hand", {}};
    for (const auto& lane : link->lanes) hand.lines.push_back({"", ControlPoint{{leg.linkId, lane.id, "", "", ""}, leg.station}});
    d.network.queueCounters.push_back(hand);
    const auto r = run(d);
    const auto counted = std::find_if(r.queues.begin(), r.queues.end(), [](const auto& q) { return q.name == "Hand"; });
    CHECK(counted != r.queues.end() && counted->maxLength > 0);
    CHECK(r.nodes[0].legs[0].name == leg.name);
    test::near(r.nodes[0].legs[0].queueMean, counted->meanLength, 1e-9);
    test::near(r.nodes[0].legs[0].queueMax, counted->maxLength, 1e-9);
    CHECK(r.los.has_value() && r.nodes[0].controlType == std::optional<std::string>("signalised"));
    for (const auto& t : r.nodes[0].turns)
        if (t.delay) CHECK(losCell(t.delay, "signalised", r.los) == std::string(1, *losLetter(*t.delay, "signalised", *r.los)));
    const auto j = movementJson(r);
    CHECK(j["nodes"][0]["los"] == std::string(1, *losLetter(*r.nodes[0].delay, "signalised", *r.los)));
    const auto csv = movementCsv(r);
    CHECK(csv.find("\n# Nodes from simulated section delay, not HCM control delay; not validated (M6)\n"
                   "level,node,name,controlType,vehicles,delay_s,unfinished,queueMean_m,queueMax_m,los\n") != std::string::npos);
    CHECK(csv.find("\nnode,\"Centre\",\"Centre\",signalised,") != std::string::npos);
}
TEST(nodeevaluation, n17_batch_rows_are_estimates_and_repeatable) { // N17
    const auto c = compiled(tJunction());
    const auto forward = runSeeds(c.snapshot.scenario, c.spec, {42, 43});
    const auto backward = runSeeds(c.snapshot.scenario, c.spec, {43, 42});
    const auto a = aggregate(forward), b = aggregate(backward);
    CHECK(a.nodes.size() == 1 && a.nodes[0].legs.size() == 3 && a.nodes[0].turns.size() == 4);
    CHECK(a.nodes[0].delay.n == 2 && a.nodes[0].delay.halfWidth95 && a.nodes[0].legs[2].queueMax.n == 2);
    const auto mean = (*forward[0].report.nodes[0].delay + *forward[1].report.nodes[0].delay) / 2;
    test::near(*a.nodes[0].delay.mean, mean, 1e-12);
    CHECK(batchJson(a, forward).dump() == batchJson(b, backward).dump());
    CHECK(batchCsv(a, forward) == batchCsv(b, backward));
    const auto j = batchJson(a, forward)["nodes"][0];
    CHECK(j["turns"][0]["delay"].contains("halfWidth95") && j["legs"][0]["queueMean"].contains("sd"));
    CHECK(batchCsv(a, forward).find("level,node,name,controlType,n,delay_s,ci95_s,sd_s,vehicles_mean,unfinished_mean,queueMean_m,queueMax_m,los\n") != std::string::npos);
}
