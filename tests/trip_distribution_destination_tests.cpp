// M10.3 D4 rows TD21-TD26 (docs/reference/TRIP_DISTRIBUTION.md §16, D482): destination-constrained purposes
// (Ortuzar and Willumsen 2011, p. 183: A_i = 1 in Eq. 5.14, B_j from Eq. 5.18), checked against Example 5.2
// (Tables 5.4-5.5, pp. 179-180) transposed: the destination problem on (P, A, f) is the origin problem on
// (A, P, f^T), transposed. Each row first checks its forcing.
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
    const bool found = std::any_of(issues.begin(), issues.end(), [&](const auto& i) { return i.code == code && i.path == path; });
    if (!found) for (const auto& i : issues) std::cerr << "  got " << i.code << " at " << i.path << '\n';
    return found;
}
void within(double a, double b, double tolerance) {
    if (!(std::abs(a - b) <= tolerance)) { std::cerr << a << " vs " << b << '\n'; CHECK(std::abs(a - b) <= tolerance); }
}
double rowSum(const std::vector<double>& r) { double s = 0; for (const double x : r) s += x; return s; }
double columnSum(const DistributionMatrix& t, std::size_t j) { double s = 0; for (const auto& r : t) s += r[j]; return s; }
DistributionMatrix transposed(const DistributionMatrix& m) {
    DistributionMatrix t(m.size(), std::vector<double>(m.size()));
    for (std::size_t i = 0; i < m.size(); ++i) for (std::size_t j = 0; j < m.size(); ++j) t[j][i] = m[i][j];
    return t;
}
DistributionInput constrained(DistributionConstraint c, DistributionMatrix costs, std::vector<double> p, std::vector<double> a, double share = 0) {
    DistributionInput in{std::move(costs), std::move(p), std::move(a), 0.1, share, true};
    in.constraint = c;
    return in;
}
// Example 5.2 as costs: f = exp(-0.1 c) reproduces Table 5.4's prior / weights when c = -10 ln(prior / A_j).
// Here the weights are the Table 5.4 prior itself: A_j f_ij = prior_ij with A = 1 and c = -10 ln prior.
const std::vector<double> kO{400, 460, 400, 702};
const DistributionMatrix kPrior{{5, 50, 100, 200}, {50, 5, 100, 300}, {50, 100, 5, 100}, {100, 200, 250, 20}};
const DistributionMatrix kTable55{{5.6, 56.3, 112.7, 225.4}, {50.5, 5.1, 101.1, 303.3}, {78.4, 156.9, 7.8, 156.9}, {123.2, 246.3, 307.9, 24.6}};
DistributionMatrix exampleCosts(const DistributionMatrix& prior) {
    auto c = prior;
    for (auto& r : c) for (auto& x : r) x = -10 * std::log(x);
    return c;
}
}

TEST(tripdistributiondestination, td21_example_5_2_transposed) { // TD21
    // The destination problem: Table 5.4's prior transposed, its targets as the columns, unit productions.
    const auto costs = exampleCosts(transposed(kPrior));
    const std::vector<double> ones(4, 1.0);
    const auto r = distributeTrips(constrained(DistributionConstraint::destination, costs, ones, kO), {});
    CHECK(r.issues.empty() && r.passes == 1 && r.converged);
    for (std::size_t j = 0; j < 4; ++j) CHECK(std::abs(columnSum(transposed(kPrior), j) - kO[j]) > 1); // forcing: the prior misses
    for (std::size_t j = 0; j < 4; ++j) {
        within(columnSum(r.trips, j), kO[j], 1e-9);
        for (std::size_t i = 0; i < 4; ++i) within(r.trips[i][j], kTable55[j][i], 0.05);
    }
    // The identity: exactly the origin path on the transposed problem, transposed back, a and b swapped.
    const auto o = distributeTrips(constrained(DistributionConstraint::origin, transposed(costs), kO, ones), {});
    for (std::size_t i = 0; i < 4; ++i) for (std::size_t j = 0; j < 4; ++j) CHECK(r.trips[i][j] == o.trips[j][i]);
    CHECK(r.a == o.b && r.b == o.a);
    // The rows float: they are not the weights.
    CHECK(std::abs(rowSum(r.trips[0]) - 1) > 1);
}

TEST(tripdistributiondestination, td22_shares_follow_the_weights) { // TD22
    const DistributionMatrix flat{{5, 5, 5}, {5, 5, 5}, {5, 5, 5}};
    const std::vector<double> p{10, 30, 60}, a{100, 50, 0};
    const auto r = distributeTrips(constrained(DistributionConstraint::destination, flat, p, a), {});
    CHECK(r.issues.empty());
    for (std::size_t i = 0; i < 3; ++i)
        for (std::size_t j = 0; j < 3; ++j) within(r.trips[i][j], a[j] * p[i] / 100, 1e-12);
    CHECK(r.a == p); // the productions are the weights
    const std::vector<double> scaled{20, 60, 120};
    const auto s = distributeTrips(constrained(DistributionConstraint::destination, flat, scaled, a), {});
    for (std::size_t i = 0; i < 3; ++i) for (std::size_t j = 0; j < 3; ++j) within(s.trips[i][j], r.trips[i][j], 1e-12);
    // Doubly on the same ends is refused (unbalanced): the destination path really is a different model.
    CHECK(distributeTrips(constrained(DistributionConstraint::doubly, flat, p, a), {}).issues.size() == 1);
}

TEST(tripdistributiondestination, td23_intrazonal_share_of_attractions) { // TD23
    DistributionMatrix c{{inf, 5, 5}, {5, inf, 5}, {5, 5, inf}};
    const std::vector<double> p{10, 30, 60}, a{100, 50, 0};
    const auto r = distributeTrips(constrained(DistributionConstraint::destination, c, p, a, 0.2), {});
    CHECK(r.issues.empty());
    within(r.intrazonal[0], 20, 1e-12); within(r.intrazonal[1], 10, 1e-12); within(r.intrazonal[2], 0, 1e-12);
    CHECK(std::abs(r.intrazonal[1] - 0.2 * p[1]) > 1); // forcing: s A_i, not s P_i
    within(r.trips[0][0], 20, 1e-12);
    within(r.trips[1][0], 80 * 30.0 / 90, 1e-12); within(r.trips[2][0], 80 * 60.0 / 90, 1e-12);
    within(columnSum(r.trips, 0), 100, 1e-12); within(columnSum(r.trips, 1), 50, 1e-12);
}

TEST(tripdistributiondestination, td24_refusals) { // TD24
    const auto none = distributeTrips(constrained(DistributionConstraint::destination, {{inf, inf}, {5, inf}}, {10, 10}, {5, 5}), {});
    CHECK(none.issues.size() == 1 && none.issues[0].code == DistributionIssue::Code::columnImpossible && none.issues[0].zone == 1);
    auto set = readJson(tdData() / "deterrence/hand-destination.json");
    CHECK(parseDeterrenceSet(set, "d", true).purposes[0].constraint == std::optional<std::string>("destination"));
    set["purposes"][0]["constraint"] = "both";
    CHECK(has(refuses([&] { parseDeterrenceSet(set, "d", true); }), "INVALID_DETERRENCE_SET", "d.purposes[0].constraint"));
    // On the one-way fixture site never receives: a column with attractions and no origin reaching it.
    const auto issues = refuses([&] { runTripDistribution(fixture(), tdData(), "example-4-1", "hand-destination", ""); });
    CHECK(!issues.empty() && std::all_of(issues.begin(), issues.end(), [](const auto& i) { return i.code == "DISTRIBUTION_COLUMN_IMPOSSIBLE"; }));
}

TEST(tripdistributiondestination, td25_schema_72) { // TD25
    auto d = fixture();
    d.deterrenceSets = {parseDeterrenceSet(readJson(tdData() / "deterrence/hand-destination.json"), "d", true)};
    d.deterrenceSets[0].id = "own";
    CHECK(documentSchemaVersion(d) == 72); // forcing: the value selects 72
    const auto saved = documentJson(d);
    CHECK(saved["schemaVersion"] == 72 && saved["deterrenceSets"][0]["purposes"][0]["constraint"] == "destination");
    CHECK(documentJson(parseDocument(saved)) == saved);
    auto older = saved; older["schemaVersion"] = 71;
    CHECK(has(refuses([&] { parseDocument(older); }), "EDIT_UNSUPPORTED_FIELD", "deterrenceSets[0].purposes[0].constraint"));
    older["deterrenceSets"][0]["purposes"][0]["constraint"] = "origin";
    CHECK(parseDocument(older).deterrenceSets[0].purposes[0].constraint == std::optional<std::string>("origin"));
    d.deterrenceSets[0].purposes[0].constraint = "origin";
    CHECK(documentSchemaVersion(d) == 68);
}

TEST(tripdistributiondestination, td26_outputs_name_the_constraint) { // TD26
    const auto run = runTripDistribution(fixture(), tdData(), "one-way", "hand-destination", "");
    const auto& p = run.purposes[0];
    CHECK(p.constraint == DistributionConstraint::destination && p.result.passes == 1);
    const auto& ends = run.generation.purposes[0];
    for (std::size_t j = 0; j < ends.attractions.size(); ++j) within(p.attractionsModelled[j], ends.attractions[j], 1e-9);
    for (std::size_t i = 0; i < p.productionsModelled.size(); ++i) within(p.productionsModelled[i], rowSum(p.result.trips[i]), 1e-12);
    const auto j = tripDistributionJson(run);
    CHECK(j["validated"] == false && j["purposes"][0]["constraint"] == "destination" && j["purposes"][0]["productionsModelled"].size() == 2 &&
          !j["purposes"][0].contains("attractionsModelled"));
    const auto csv = tripDistributionCsv(run);
    CHECK(csv.rfind("# MULTRA - not yet validated.", 0) == 0 && csv.find("destination constrained (rows not balanced)") != std::string::npos);
    const auto doubly = tripDistributionJson(runTripDistribution(fixture(), tdData(), "one-way", "hand", ""));
    CHECK(!doubly["purposes"][0].contains("productionsModelled") && !doubly["purposes"][0].contains("constraint"));
}
