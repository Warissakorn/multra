#include "test.hpp"
#include "../tools/count_study_network.hpp"
#include "../src/project/run.hpp"
#include <fstream>
#include <map>
using namespace multra;
// The counted Ratchadaphisek x Lat Phrao study (tools/count_study_network.hpp): the committed file
// is what the builder writes from its two CSV files, and every movement gets what was counted.
namespace {
const auto kDir = "studies/ratchada-latphrao";
ProjectDocument built() {
    const auto dir = test::root() / kDir;
    return fixture::countStudy(test::root() / "data", dir / "counts.csv", dir / "site.csv", "ratchada-latphrao");
}
bool same(const Json& a, const Json& b) { // numbers to 1e-9, as m26study: MSVC may round a last bit
    if (a.is_number() && b.is_number()) return std::abs(a.get<double>() - b.get<double>()) <= 1e-9;
    if (a.type() != b.type() || a.size() != b.size()) return false;
    if (a.is_object()) {
        for (auto it = a.begin(); it != a.end(); ++it)
            if (!b.contains(it.key()) || !same(it.value(), b.at(it.key()))) return false;
        return true;
    }
    if (a.is_array()) {
        for (std::size_t i = 0; i < a.size(); ++i) if (!same(a[i], b[i])) return false;
        return true;
    }
    return a == b;
}
}
TEST(countstudy, the_committed_file_is_what_the_builder_writes) {
    std::ifstream file(test::root() / kDir / "ratchada-latphrao.traffic.json");
    Json committed; file >> committed;
    CHECK(same(documentJson(built()), committed));
    CHECK(documentJson(parseDocument(committed)) == committed);
}
TEST(countstudy, each_approach_carries_exactly_the_counted_vehicles) {
    // Hourly totals per approach from counts.csv (U-turns joined to the right turn).
    const std::map<std::string, double> expected{{"West", 2568}, {"East", 3616}, {"South", 1741}, {"North", 2744}};
    const auto d = built();
    const auto split = withRoutingDecisions(*d.definition).inputs;
    std::map<std::string, double> vehicles;
    for (const auto& input : split) {
        double n = 0;
        for (const auto& p : input.intervals) n += p.vehiclesPerHour * (p.endTime - p.startTime) / 3600;
        const auto& comp = input.compositionId;
        vehicles[comp.substr(comp.rfind('-') + 1)] += n;
    }
    for (const auto& [approach, total] : expected) test::near(vehicles[approach], total, 1e-6);
    validateDocument(d);  // throws on an invalid document
    CHECK(runDiagnostics(d, test::root() / "data").empty());
}
TEST(countstudy, a_left_turn_stub_is_held_before_its_turn_leaves) { // LANE_LOCKS LL1, D579
    // NW (West) has five stop-line lanes; the left Connector leaves the kerb lane 6 m before the
    // stop line. Before D579 a left-turner on another lane was held at the end of its own lane,
    // where the only change left led to another stub, and stood there for the rest of the run.
    // Every stub that reaches West's stop-line Link must now be held at least those 6 m short of its end.
    const auto d = built();
    const auto scenario = compileDocument(d, test::root() / "data").scenario;
    std::string upstream;
    for (const auto& l : d.network.links) if (l.name == "West approach") upstream = l.id;
    std::map<std::string, double> length;
    for (const auto& s : scenario.segments) length[s.id] = s.length;
    int stubs = 0;
    for (const auto& dead : scenario.routeDeadEnds) {
        if (dead.routeId.rfind("link:" + upstream + "/", 0) != 0) continue;
        double total = 0;
        for (const auto& route : scenario.routes)
            if (route.id == dead.routeId) for (const auto& id : route.segmentIds) total += length[id];
        if (total <= 230) continue; // a right-turn stub ends on the upstream Link itself, rightly at its end
        ++stubs;
        CHECK(dead.at <= total - 6 + 1e-6);
    }
    CHECK(stubs > 0);
}
