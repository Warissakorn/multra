// M10.3 D3 rows TD15-TD20 (docs/reference/TRIP_DISTRIBUTION.md §15, D476): origin-constrained purposes
// (Ortuzar and Willumsen 2011, Eq. 5.16, p. 183), against Example 5.2 (Tables 5.4-5.5, pp. 179-180; values
// read from the page images, docs/evidence/ortuzar-ch5-trip-distribution.md §6). Each row first checks
// its forcing.
#include "test.hpp"
#include "../src/macro/trip_distribution.hpp"
#include "../src/project/deterrence_codec.hpp"
#include "../src/project/document.hpp"
#include "../src/project/document_schema.hpp"
#include "../src/project/trip_distribution_output.hpp"
#include "../src/project/trip_distribution_run.hpp"
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
double rowSum(const std::vector<double>& r) { double s = 0; for (const double x : r) s += x; return s; }
double columnSum(const DistributionMatrix& t, std::size_t j) { double s = 0; for (const auto& r : t) s += r[j]; return s; }
// Example 5.2: Table 5.4's prior and targets; Table 5.5's printed cells.
const std::vector<double> kO{400, 460, 400, 702};
const DistributionMatrix kPrior{{5, 50, 100, 200}, {50, 5, 100, 300}, {50, 100, 5, 100}, {100, 200, 250, 20}};
const DistributionMatrix kTable55{{5.6, 56.3, 112.7, 225.4}, {50.5, 5.1, 101.1, 303.3}, {78.4, 156.9, 7.8, 156.9}, {123.2, 246.3, 307.9, 24.6}};
DistributionInput origin(DistributionMatrix costs, std::vector<double> p, std::vector<double> a, double share = 0) {
    DistributionInput in{std::move(costs), std::move(p), std::move(a), 0.1, share, true};
    in.constraint = DistributionConstraint::origin;
    return in;
}
}

TEST(tripdistributionorigin, td15_example_5_2_one_row_normalisation) { // TD15
    for (std::size_t i = 0; i < 4; ++i) CHECK(std::abs(rowSum(kPrior[i]) - kO[i]) > 1); // forcing: the prior misses every target
    const auto r = originConstrained(kPrior, kO);
    CHECK(r.passes == 1 && r.converged && r.error < 1e-12);
    for (std::size_t i = 0; i < 4; ++i) {
        within(rowSum(r.trips[i]), kO[i], 1e-12);
        within(r.a[i], kO[i] / rowSum(kPrior[i]), 1e-15);
        for (std::size_t j = 0; j < 4; ++j) within(r.trips[i][j], kTable55[i][j], 0.05);
    }
    // The columns float; column 4 is printed 701.2, a misprint for the 710.2 its printed cells add to.
    const std::vector<double> columns{257.8, 464.6, 529.5, 710.1};
    for (std::size_t j = 0; j < 4; ++j) within(columnSum(r.trips, j), columns[j], 0.05);
    // A doubly pass on the same prior would also move the rows' cells: one pass is not the same thing.
    const auto doubly = furness(kPrior, kO, {257.8, 464.6, 529.5, 710.1}, [] { DistributionSettings s; s.fixedPasses = 1; return s; }());
    CHECK(std::abs(doubly.trips[0][0] - r.trips[0][0]) > 1e-9 || std::abs(doubly.trips[3][3] - r.trips[3][3]) > 1e-9);
}

TEST(tripdistributionorigin, td16_shares_follow_the_weights) { // TD16
    const DistributionMatrix flat{{5, 5, 5}, {5, 5, 5}, {5, 5, 5}};
    const std::vector<double> p{100, 50, 0}, a{10, 30, 60};
    const auto r = distributeTrips(origin(flat, p, a), {});
    CHECK(r.issues.empty() && r.passes == 1 && r.converged);
    for (std::size_t i = 0; i < 3; ++i)
        for (std::size_t j = 0; j < 3; ++j) within(r.trips[i][j], p[i] * a[j] / 100, 1e-12);
    CHECK(r.b == a);
    // One cheaper destination: rows still meet P exactly, columns move toward it.
    auto skewed = flat; skewed[0][2] = 1; skewed[1][2] = 1;
    const auto s = distributeTrips(origin(skewed, p, a), {});
    for (std::size_t i = 0; i < 3; ++i) within(rowSum(s.trips[i]), p[i], 1e-12);
    CHECK(columnSum(s.trips, 2) > columnSum(r.trips, 2) + 1);
    // Choice 4a: T_ii = s P_i set aside; the rest by the weights, the diagonal unused.
    DistributionMatrix noDiagonal = flat; for (std::size_t z = 0; z < 3; ++z) noDiagonal[z][z] = inf;
    const auto i = distributeTrips(origin(noDiagonal, p, a, 0.2), {});
    within(i.intrazonal[0], 20, 1e-12); within(i.intrazonal[1], 10, 1e-12);
    within(i.trips[0][0], 20, 1e-12);
    within(i.trips[0][1], 80 * 30.0 / 90, 1e-12); within(i.trips[0][2], 80 * 60.0 / 90, 1e-12);
    within(rowSum(i.trips[0]), 100, 1e-12);
}

TEST(tripdistributionorigin, td17_scaling_the_weights_changes_nothing) { // TD17
    const DistributionMatrix c{{3, 11, 18}, {12, 3, 12}, {15.5, 13, 5}};
    const std::vector<double> p{400, 460, 400}, raw{100, 300, 200}, balanced{1260.0 / 600 * 100, 1260.0 / 600 * 300, 1260.0 / 600 * 200};
    DistributionInput doubly{c, p, raw, 0.1, 0, true};
    CHECK(distributeTrips(doubly, {}).issues.size() == 1); // forcing: unbalanced ends are refused when doubly
    const auto a = distributeTrips(origin(c, p, raw), {}), b = distributeTrips(origin(c, p, balanced), {});
    CHECK(a.issues.empty() && b.issues.empty());
    for (std::size_t i = 0; i < 3; ++i)
        for (std::size_t j = 0; j < 3; ++j) within(a.trips[i][j], b.trips[i][j], 1e-9);
}

TEST(tripdistributionorigin, td18_double_keeps_every_byte) { // TD18
    auto d = fixture();
    const auto before = runTripDistribution(d, tdData(), "one-way", "hand", "");
    auto set = parseDeterrenceSet(readJson(tdData() / "deterrence/hand.json"), "d", true);
    set.id = "explicit"; set.purposes[0].constraint = "double";
    d.deterrenceSets = {set};
    CHECK(documentSchemaVersion(d) == 68); // forcing: the field is present
    const auto after = runTripDistribution(d, tdData(), "one-way", "explicit", "");
    auto j = tripDistributionJson(after), k = tripDistributionJson(before);
    CHECK(!j["purposes"][0].contains("constraint") && j["purposes"] == k["purposes"] && j["costs"] == k["costs"]);
    auto strip = [](std::string csv) { const auto at = csv.find("deterrence set: "); return csv.substr(csv.find('\n', at)); };
    CHECK(strip(tripDistributionCsv(after)) == strip(tripDistributionCsv(before)));
    d.deterrenceSets[0].purposes[0].constraint.reset();
    CHECK(documentSchemaVersion(d) == 67);
}

TEST(tripdistributionorigin, td19_refusals_and_schema_68) { // TD19
    const DistributionMatrix c{{inf, 5}, {5, inf}};
    const auto none = distributeTrips(origin(c, {10, 10}, {0, 0}), {});
    CHECK(none.issues.size() == 2 && none.issues[0].code == DistributionIssue::Code::rowImpossible);
    const auto unreachable = distributeTrips(origin({{inf, inf}, {5, inf}}, {10, 10}, {5, 5}), {});
    CHECK(unreachable.issues.size() == 1 && unreachable.issues[0].zone == 0);
    auto bad = readJson(tdData() / "deterrence/hand-origin.json");
    CHECK(parseDeterrenceSet(bad, "d", true).purposes[0].constraint == std::optional<std::string>("origin"));
    bad["purposes"][0]["constraint"] = "both"; // D482 admits "destination"
    CHECK(has(refuses([&] { parseDeterrenceSet(bad, "d", true); }), "INVALID_DETERRENCE_SET", "d.purposes[0].constraint"));
    auto d = fixture();
    d.deterrenceSets = {parseDeterrenceSet(readJson(tdData() / "deterrence/hand-origin.json"), "d", true)};
    d.deterrenceSets[0].id = "own";
    const auto saved = documentJson(d);
    CHECK(saved["schemaVersion"] == 68 && saved["deterrenceSets"][0]["purposes"][0]["constraint"] == "origin");
    CHECK(documentJson(parseDocument(saved)) == saved);
    auto older = saved; older["schemaVersion"] = 67;
    CHECK(has(refuses([&] { parseDocument(older); }), "EDIT_UNSUPPORTED_FIELD", "deterrenceSets[0].purposes[0].constraint"));
    older["deterrenceSets"][0]["purposes"][0].erase("constraint");
    CHECK(parseDocument(older).deterrenceSets.size() == 1);
    // The run refuses a row with nothing reachable at its path: town reaches nothing on the fixture.
    CHECK(has(refuses([&] { runTripDistribution(fixture(), tdData(), "example-4-1", "hand-origin", ""); }),
              "DISTRIBUTION_ROW_IMPOSSIBLE", "zones[1].all.productions"));
}

TEST(tripdistributionorigin, td20_outputs_name_the_constraint) { // TD20
    const auto run = runTripDistribution(fixture(), tdData(), "one-way", "hand-origin", "");
    const auto& p = run.purposes[0];
    CHECK(p.constraint == DistributionConstraint::origin && p.result.passes == 1);
    within(p.result.trips[0][1], 150, 1e-9);
    within(p.attractionsModelled[1], 150, 1e-9);
    const auto j = tripDistributionJson(run);
    CHECK(j["validated"] == false && j["purposes"][0]["constraint"] == "origin" && j["purposes"][0]["attractionsModelled"].size() == 2);
    const auto csv = tripDistributionCsv(run);
    CHECK(csv.rfind("# MULTRA - not yet validated.", 0) == 0 && csv.find("origin constrained") != std::string::npos);
}
