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
