// M10.3 D1 rows TD1-TD10 (docs/reference/TRIP_DISTRIBUTION.md §13, D473): Furness and the gravity model
// against Ortuzar and Willumsen Examples 5.3 and 5.4 (book pp. 181, 189-190; values read from the page
// images, docs/evidence/ortuzar-ch5-trip-distribution.md), stopping, refusals, the skim and the codec.
#include "test.hpp"
#include "../src/macro/assign.hpp"
#include "../src/macro/trip_distribution.hpp"
#include "../src/project/deterrence_codec.hpp"
#include "../src/project/trip_distribution_output.hpp"
#include "../src/project/trip_distribution_run.hpp"
#include "../src/project/document.hpp"
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
using namespace multra;
namespace {
constexpr double inf = std::numeric_limits<double>::infinity();
std::filesystem::path tdData() { return test::root() / "tests/data/trip-distribution"; }
Json readJson(const std::filesystem::path& p) { std::ifstream f(p); Json j; f >> j; return j; }
ProjectDocument fixture() { return parseDocument(readJson(test::root() / "tests/trip-generation.traffic.json")); }
template<class F> std::vector<ValidationIssue> refuses(F f) {
    try { f(); } catch (const ValidationError& e) { return e.issues; }
    return {};
}
bool has(const std::vector<ValidationIssue>& issues, const std::string& code, const std::string& path) {
    return std::any_of(issues.begin(), issues.end(), [&](const auto& i) { return i.code == code && i.path == path; });
}
void within(double a, double b, double tolerance) {
    if (!(std::abs(a - b) <= tolerance)) { std::cerr << a << " vs " << b << '\n'; CHECK(std::abs(a - b) <= tolerance); }
}
// Examples 5.3 and 5.4 (Tables 5.6 and 5.8).
const std::vector<double> kO{400, 460, 400, 702}, kD{260, 400, 500, 802};
const DistributionMatrix kPrior{{5, 50, 100, 200}, {50, 5, 100, 300}, {50, 100, 5, 100}, {100, 200, 250, 20}};
const DistributionMatrix kCosts{{3, 11, 18, 22}, {12, 3, 12, 19}, {15.5, 13, 5, 7}, {24, 18, 8, 5}};
DistributionSettings passes(int n) { DistributionSettings s; s.fixedPasses = n; return s; }
// A data root holding the shipped test sets plus the given deterrence set.
std::filesystem::path dataWith(const std::string& name, const Json& deterrence) {
    const auto root = std::filesystem::temp_directory_path() / ("multra-td-" + name);
    std::filesystem::remove_all(root);
    std::filesystem::copy(tdData(), root, std::filesystem::copy_options::recursive);
    std::filesystem::remove(root / "deterrence" / "hand.json");
    std::ofstream(root / "deterrence" / "d.json") << deterrence.dump();
    return root;
}
Json deterrence(const Json& purposes) {
    return {{"id", "d"}, {"name", "d"}, {"source", {{"citation", "hand"}}}, {"function", "exponential"}, {"costUnit", "min"}, {"purposes", purposes}};
}
}

TEST(tripdistribution, td1_example_5_3_furness) { // TD1: Table 5.7 after three row-first passes
    const auto r = furness(kPrior, kO, kD, passes(3));
    CHECK(r.passes == 3);
    const DistributionMatrix printed{{5.25, 44.12, 98.24, 254.25}, {45.30, 3.81, 84.78, 329.11},
                                     {77.04, 129.50, 7.21, 186.58}, {132.41, 222.57, 309.77, 32.07}};
    for (std::size_t i = 0; i < 4; ++i) for (std::size_t j = 0; j < 4; ++j) within(r.trips[i][j], printed[i][j], 0.005);
    for (std::size_t j = 0; j < 4; ++j) { double s = 0; for (const auto& row : r.trips) s += row[j]; test::near(s, kD[j], 1e-9); }
    within(r.error, 0.0074, 0.0001); // the book: "within 1 %"
    // Columns first (the transposed problem) does not give the printed table.
    DistributionMatrix t(4, std::vector<double>(4));
    for (std::size_t i = 0; i < 4; ++i) for (std::size_t j = 0; j < 4; ++j) t[j][i] = kPrior[i][j];
    const auto c = furness(t, kD, kO, passes(3));
    CHECK(std::abs(c.trips[1][0] - 44.12) > 0.05);
}

TEST(tripdistribution, td2_example_5_4_gravity) { // TD2: Table 5.10, its factors and the band table
    const DistributionInput in{kCosts, kO, kD, 0.10, 0.0, true};
    const auto r = distributeTrips(in, passes(3));
    CHECK(r.issues.empty());
    const DistributionMatrix printed{{155.73, 99.00, 64.46, 74.17}, {57.54, 200.22, 106.73, 90.98},
                                     {25.87, 47.01, 137.16, 192.77}, {20.86, 53.77, 191.65, 444.08}};
    for (std::size_t i = 0; i < 4; ++i) for (std::size_t j = 0; j < 4; ++j) within(r.trips[i][j], printed[i][j], 0.005);
    const std::vector<double> a{1.17, 1.07, 0.68, 1.28}, b{179.17, 253.50, 332.37, 570.53};
    for (std::size_t k = 0; k < 4; ++k) { within(r.a[k], a[k], 0.005); within(r.b[k], b[k], 0.005); }
    const auto bands = tripLengthBands(r.trips, kCosts, 4);
    const std::vector<double> printedBands{355.9, 965.7, 263.3, 72.9, 209.2, 95.0};
    CHECK(bands.size() == 6);
    for (std::size_t k = 0; k < 6; ++k) within(bands[k], printedBands[k], 0.05);
    const auto converged = distributeTrips(in, DistributionSettings{1e-12, 1000, {}});
    CHECK(converged.converged);
    const std::vector<double> row1{157.04, 100.36, 66.14, 76.46};
    for (std::size_t j = 0; j < 4; ++j) within(converged.trips[0][j], row1[j], 0.005);
    within(meanCost(converged.trips, kCosts), 8.65, 0.005);
}

TEST(tripdistribution, td3_td4_stopping) { // TD3 converged within the target; TD4 the cap is flagged, not hidden
    const DistributionInput in{kCosts, kO, kD, 0.10, 0.0, true};
    const auto r = distributeTrips(in, {});
    CHECK(r.converged && r.error <= 1e-3 && r.passes == 6);
    for (std::size_t i = 0; i < 4; ++i) { double s = 0; for (const double x : r.trips[i]) s += x; CHECK(std::abs(s - kO[i]) / kO[i] <= 1e-3); }
    const auto capped = distributeTrips(in, DistributionSettings{1e-3, 1, {}});
    CHECK(!capped.converged && capped.passes == 1 && capped.error > 1e-3 && capped.trips.size() == 4);
}

TEST(tripdistribution, td5_td6_refusals) { // TD5 infeasible ends; TD6 unbalanced input
    DistributionMatrix c = kCosts;
    c[2] = {inf, inf, inf, inf};
    auto r = distributeTrips({c, kO, kD, 0.1, 0.0, true}, {});
    CHECK(r.issues.size() == 1 && r.issues[0].code == DistributionIssue::Code::rowImpossible && r.issues[0].zone == 2 && r.trips.empty());
    r = distributeTrips({kCosts, kO, {260, 400, 500, 803}, 0.1, 0.0, true}, {});
    CHECK(r.issues.size() == 1 && r.issues[0].code == DistributionIssue::Code::unbalanced);
    // The fixture's network runs site -> town only: Example 4.1's ends cannot be distributed.
    const auto issues = refuses([] { runTripDistribution(fixture(), tdData(), "example-4-1", "hand", ""); });
    CHECK(has(issues, "DISTRIBUTION_ROW_IMPOSSIBLE", "zones[1].all.productions"));
    CHECK(has(issues, "DISTRIBUTION_COLUMN_IMPOSSIBLE", "zones[0].all.attractions"));
}

TEST(tripdistribution, td7_free_flow_skim) { // TD7: minutes by hand, unreachable pairs, a mode-restricted arc
    MacroGraph g;
    g.nodes = {"a", "b", "c", "d"};
    MacroLink l; l.from = 0; l.to = 1; l.freeTime = 120; g.links.push_back(l);
    l.from = 1; l.to = 2; l.freeTime = 60; l.modes = {"bus"}; g.links.push_back(l);
    l.from = 0; l.to = 2; l.freeTime = 600; l.modes = {}; g.links.push_back(l);
    const auto car = freeFlowSkim(g, {0}, {2, 3, 4}, "car"), bus = freeFlowSkim(g, {0}, {2}, "bus"), every = freeFlowSkim(g, {0}, {2}, "");
    test::near(car[0][0], 10.0); test::near(bus[0][0], 3.0); test::near(every[0][0], 3.0);
    CHECK(std::isinf(car[0][1]) && std::isinf(car[0][2])); // unreached node; absent node
    // The run on the fixture: site -> town only, every trip on that pair.
    const auto run = runTripDistribution(fixture(), tdData(), "one-way", "hand", "");
    CHECK(run.generation.zoneIds == std::vector<std::string>({"site", "town"}));
    CHECK(std::isfinite(run.costs[0][1]) && run.costs[0][1] > 0 && std::isinf(run.costs[1][0]));
    test::near(run.purposes[0].result.trips[0][1], 150.0, 1e-9);
    test::near(run.purposes[0].meanCost, run.costs[0][1]);
    const auto csv = tripDistributionCsv(run);
    CHECK(csv.rfind("# MULTRA - not yet validated.", 0) == 0);
    CHECK(csv.find("\"all\",\"town\",\"site\",0.00,\n") != std::string::npos);
    CHECK(csv.find("\"all\",totals,,150.00,") != std::string::npos);
}

TEST(tripdistribution, td8_intrazonal_share) { // TD8: T_ii = s min(P_i, A_i), set aside before Furness
    const DistributionMatrix c{{2, 5, 9}, {5, 2, 4}, {9, 4, 2}};
    const std::vector<double> p{100, 50, 30}, q{60, 90, 30};
    const auto r = distributeTrips({c, p, q, 0.1, 0.5, false}, DistributionSettings{1e-12, 1000, {}});
    CHECK(r.issues.empty() && r.converged);
    test::near(r.intrazonal[0], 30); test::near(r.intrazonal[1], 25); test::near(r.intrazonal[2], 15);
    for (std::size_t i = 0; i < 3; ++i) {
        test::near(r.trips[i][i], r.intrazonal[i]); // f_ii = 0: the diagonal is the share alone
        double row = 0, column = 0;
        for (std::size_t k = 0; k < 3; ++k) { row += r.trips[i][k]; column += r.trips[k][i]; }
        test::near(row, p[i], 1e-9); test::near(column, q[i], 1e-9);
    }
}

TEST(tripdistribution, td9_deterrence_codec) { // TD9
    const auto j = readJson(tdData() / "deterrence/hand.json");
    const auto set = parseDeterrenceSet(j, "x", true);
    CHECK(set.id == "hand" && set.purposes.size() == 1 && set.purposes[0].beta == 0.1 && !set.purposes[0].intrazonalShare);
    CHECK(parseDeterrenceSet(deterrenceSetJson(set), "x", true) == set);
    const auto faults = [](Json bad) { return refuses([&] { parseDeterrenceSet(bad, "x", true); }); };
    auto bad = j; bad["function"] = "power";
    CHECK(has(faults(bad), "INVALID_DETERRENCE_SET", "x.function"));
    bad = j; bad["costUnit"] = "s";
    CHECK(has(faults(bad), "INVALID_DETERRENCE_SET", "x.costUnit"));
    bad = j; bad["purposes"][0]["beta"] = 0;
    CHECK(has(faults(bad), "INVALID_DETERRENCE_SET", "x.purposes[0].beta"));
    bad = j; bad["purposes"][0]["intrazonalShare"] = 1;
    CHECK(has(faults(bad), "INVALID_DETERRENCE_SET", "x.purposes[0].intrazonalShare"));
    bad = j; bad["source"] = Json::object();
    CHECK(has(faults(bad), "INVALID_DETERRENCE_SET", "x.source.citation"));
    bad = j; bad["extra"] = 1;
    CHECK(has(faults(bad), "INVALID_DETERRENCE_SET", "x"));
    const auto doc = fixture();
    CHECK(has(refuses([&] { runTripDistribution(doc, tdData(), "one-way", "none", ""); }), "DISTRIBUTION_SET_UNKNOWN", "deterrenceSets"));
    const auto other = dataWith("purpose", deterrence(Json::array({{{"id", "work"}, {"beta", 0.1}}})));
    CHECK(has(refuses([&] { runTripDistribution(doc, other, "one-way", "d", ""); }), "DISTRIBUTION_PURPOSE_MISSING", "catalog/deterrence/d.json.purposes"));
    CHECK(resolveDeterrenceSets(test::root() / "no-such-folder").empty());
}

TEST(tripdistribution, td10_same_bytes_and_permutation) { // TD10
    const auto doc = fixture();
    const auto once = tripDistributionJson(runTripDistribution(doc, tdData(), "one-way", "hand", "")).dump();
    CHECK(tripDistributionJson(runTripDistribution(doc, tdData(), "one-way", "hand", "")).dump() == once);
    auto swapped = doc;
    std::swap(swapped.zones.zones[0], swapped.zones.zones[1]);
    std::reverse(swapped.network.links.begin(), swapped.network.links.end());
    CHECK(swapped.zones.zones[0].id == "town");
    CHECK(tripDistributionJson(runTripDistribution(swapped, tdData(), "one-way", "hand", "")).dump() == once);
    CHECK(tripDistributionCsv(runTripDistribution(swapped, tdData(), "one-way", "hand", "")) ==
          tripDistributionCsv(runTripDistribution(doc, tdData(), "one-way", "hand", "")));
}
