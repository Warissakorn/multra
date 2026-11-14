// The shipped Node analysis example (tools/node_example_network.hpp,
// data/projects/signalised-node-analysis.traffic.json, D500): the committed file is the builder's
// output, HCM 7 Ch. 19 analyses every lane group below capacity, and one section per movement
// carries the control type that gives simulated section delay its letter.
#include "test.hpp"
#include "../tools/node_example_network.hpp"
#include "../src/project/document.hpp"
#include "../src/project/demand_paths.hpp"
#include "../src/project/node_analysis_run.hpp"
#include <cmath>
#include <fstream>
#include <set>
using namespace multra;
namespace {
const auto kFile = "data/projects/signalised-node-analysis.traffic.json";
Json committed() { std::ifstream f(test::root() / kFile); Json j; f >> j; return j; }
// Structure and strings exactly, numbers to 1e-9: Connector curves are computed geometry.
bool same(const Json& a, const Json& b) {
    if (a.is_number() && b.is_number()) return std::abs(a.get<double>() - b.get<double>()) <= 1e-9;
    if (a.type() != b.type() || a.size() != b.size()) return false;
    if (a.is_object()) {
        for (auto it = a.begin(); it != a.end(); ++it) if (!b.contains(it.key()) || !same(it.value(), b.at(it.key()))) return false;
        return true;
    }
    if (a.is_array()) { for (std::size_t i = 0; i < a.size(); ++i) if (!same(a[i], b[i])) return false; return true; }
    return a == b;
}
}
TEST(nodeexample, committed_file_is_the_builder_output) {
    const auto built = documentJson(fixture::nodeExample(test::root() / "data").document);
    CHECK(same(committed(), built));
    CHECK(documentJson(parseDocument(committed())) == committed()); // and the file reopens exactly
}
TEST(nodeexample, every_lane_group_is_analysed_below_capacity) {
    const auto d = parseDocument(committed());
    const auto run = runNodeAnalysis(d, test::root() / "data");
    CHECK(run.nodes.size() == 1);
    const auto& node = run.nodes[0];
    CHECK(node.advisories.empty() && node.los && node.delay && node.critical && node.approaches.size() == 4);
    CHECK(node.groups.size() == 10); // major: pocket, through, through+kerb; minor: far-turn lane, through+kerb (D513)
    for (const auto& g : node.groups) CHECK(!g.issue && g.result && g.los && g.result->ratio < 1 && *g.los != 'F');
    for (const auto& a : node.approaches) CHECK(a.delay && a.los);
    // The counts are the demand: each approach's analysis volume is its input's, over the evaluation window.
    double counted = 0, entering = 0;
    for (const auto& t : d.network.nodes[0].analysis->turns) counted += t.vehiclesPerHour;
    for (const auto& i : d.definition->inputs) entering += i.vehiclesPerHour;
    test::near(counted, entering);
    const auto& e = *d.definition->evaluation;
    test::near(*d.network.nodes[0].analysis->analysisPeriod * 3600, *e.end - e.warmup);
}
TEST(nodeexample, each_movement_has_a_signalised_section_and_a_route) {
    const auto t = fixture::nodeExample(test::root() / "data");
    const auto& d = t.document;
    CHECK(d.network.travelTimeSections.size() == 12);
    std::set<std::pair<std::string, std::string>> movements;
    for (const auto& s : d.network.travelTimeSections) {
        CHECK(s.controlType == SectionControl::signalised && !s.name.empty());
        movements.insert({s.start.linkId, s.end.linkId});
    }
    CHECK(movements.size() == 12);
    for (const auto& entry : t.entries) CHECK(std::count_if(movements.begin(), movements.end(), [&](const auto& m) { return m.first == entry; }) == 3);
    // Every decision destination becomes a path, the far turn into the major's pocket included (D501,
    // D513): none is dropped from the routeless walk, and nothing blocks a run.
    const auto issues = routelessIssues(d.network, *d.definition);
    CHECK(issues.blocking.empty() && issues.advisory.empty());
    const auto placed = placedDecisions(d.network, *d.definition);
    CHECK(placed.size() == 4);
    for (std::size_t a = 0; a < 4; ++a) {
        const auto walk = routelessChains(d.network, t.entries[a], placed);
        std::set<std::string> ends;
        for (const auto& c : walk.chains) if (!c.stub) ends.insert(c.laneChain.back());
        CHECK(walk.issues.empty() && ends.size() >= 3); // a lane of each of the three exits
    }
}
