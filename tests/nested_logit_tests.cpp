// M10.4 N1 rows NL1-NL7 (docs/reference/NESTED_LOGIT.md §6, D481): the two-level nested logit against
// Ortuzar and Willumsen Example 6.3 (book p. 216, read from the page image,
// docs/evidence/ortuzar-ch6-7-mode-choice.md §4), its collapse to the MNL, availability, numerics, the nests
// codec, schema 71 and the run. Each row first checks its forcing.
#include "test.hpp"
#include "../src/macro/mode_choice.hpp"
#include "../src/project/document.hpp"
#include "../src/project/document_schema.hpp"
#include "../src/project/mode_choice_output.hpp"
#include "../src/project/mode_choice_run.hpp"
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
using namespace multra;
namespace {
const double kNaN = std::numeric_limits<double>::quiet_NaN();
std::filesystem::path tdData() { return test::root() / "tests/data/trip-distribution"; }
Json readJson(const std::filesystem::path& p) { std::ifstream f(p); Json j; f >> j; return j; }
Json fixtureJson() { return readJson(tdData() / "mode-choice.traffic.json"); }
Json nestedJson() { return readJson(tdData() / "mode-choice/nested.json"); }
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
double sum(const std::vector<double>& s) { double t = 0; for (const double x : s) t += x; return t; }
// Example 6.3: car alone, red bus and blue bus in one nest, every cost 50, lambda2 = 0.9. In Eq. (7.23) form
// (beta = 1): V = -lambda1 C, phi = lambda1 / lambda2, so V / phi = -lambda2 C and exp(W) = exp(-lambda1 C_B).
std::vector<double> example63(double lambda1) { return nestedLogitShares(std::vector<double>(3, -lambda1 * 50), {-1, 0, 0}, {lambda1 / 0.9}); }
}

TEST(nestedlogit, nl1_example_6_3) { // NL1
    within(logitShares({-0.05, -0.05, -0.05})[0], 1.0 / 3, 1e-15); // forcing: the MNL gives 1/3 whatever lambda1
    const double cb = 50 - std::log(2.0) / 0.9;
    within(cb, 49.2298, 1e-4); // printed 49.23
    // The composite from the nest's W: W = -lambda1 C_B, recovered from the car share.
    const double l1 = 0.5;
    const auto s = example63(l1);
    within(std::log(s[0]) - std::log(s[1] + s[2]), -l1 * (50 - cb), 1e-12);
    const std::vector<std::pair<double, double>> table{{0.001, 0.500}, {0.005, 0.499}, {0.010, 0.498}, {0.050, 0.490}, {0.100, 0.481},
                                                       {0.500, 0.405}, {0.600, 0.386}, {0.700, 0.368}, {0.800, 0.351}, {0.900, 0.333}};
    for (const auto& [lambda1, printed] : table) {
        const auto p = example63(lambda1);
        within(std::round(p[0] * 1000) / 1000, printed, 1e-12);
        within(p[0], 1 / (1 + std::exp(-lambda1 * (cb - 50))), 1e-12);
        within(p[1], p[2], 1e-15); within(sum(p), 1, 1e-15);
    }
}

TEST(nestedlogit, nl2_collapse_to_the_mnl) { // NL2
    const std::vector<double> v{-1.0, -2.5, -0.3, -1.7};
    const auto mnl = logitShares(v);
    const auto half = nestedLogitShares(v, {-1, 0, 0, -1}, {0.5});
    CHECK(std::abs(half[0] - mnl[0]) > 1e-3); // forcing: phi < 1 departs from the MNL
    const auto one = nestedLogitShares(v, {-1, 0, 0, 1}, {1.0, 1.0});
    for (std::size_t k = 0; k < v.size(); ++k) within(one[k], mnl[k], 1e-15);
    const auto singletons = nestedLogitShares(v, {0, 1, 2, 3}, {0.2, 0.4, 0.6, 0.8});
    for (std::size_t k = 0; k < v.size(); ++k) within(singletons[k], mnl[k], 1e-15);
    const auto none = nestedLogitShares(v, {-1, -1, -1, -1}, {});
    CHECK(none == mnl);
    for (const double p : example63(0.9)) within(p, 1.0 / 3, 1e-15); // lambda1 = lambda2
}

TEST(nestedlogit, nl3_availability) { // NL3
    const auto s = nestedLogitShares({-1.0, kNaN, -2.0}, {-1, 0, 0}, {0.5});
    CHECK(s[1] == 0); // forcing: the unavailable member gets nothing
    const auto alone = logitShares({-1.0, -2.0}); // a one-member nest's W is its V
    within(s[0], alone[0], 1e-15); within(s[2], alone[1], 1e-15);
    const auto gone = nestedLogitShares({-1.0, kNaN, -std::numeric_limits<double>::infinity()}, {-1, 0, 0}, {0.5});
    CHECK(gone == std::vector<double>({1, 0, 0}));
    CHECK(nestedLogitShares({kNaN, kNaN}, {0, 0}, {0.5}) == std::vector<double>({0, 0}));
    const double inf = std::numeric_limits<double>::infinity();
    const DistributionMatrix trips{{0, 10}, {0, 0}};
    const DistributionMatrix closed{{inf, inf}, {inf, inf}};
    const auto none = splitModes(trips, {{0, -0.1, closed}, {0, -0.1, closed}}, {{0, 0}, {0.5}});
    CHECK(none.issues.size() == 1 && none.persons.empty());
}

TEST(nestedlogit, nl4_numerics_and_iia) { // NL4
    const double phi = 1e-3;
    CHECK(std::exp(-0.1 * 1e4 / phi) == 0); // forcing: the naive inner numerator underflows
    const auto s = nestedLogitShares({-0.1 * 1e4, -0.1 * 1e4, -0.1 * (1e4 + 10)}, {-1, 0, 0}, {phi});
    for (const double x : s) CHECK(std::isfinite(x));
    within(sum(s), 1, 1e-15);
    within(s[0], 1 / (1 + std::exp(-0.0)), 1e-12); // the nest's W is its best member's V within phi ln(1 + e^{-1000})
    // Red bus / blue bus: car and red bus share half; adding blue bus in a tight nest keeps car near half.
    within(logitShares({-5.0, -5.0})[0], 0.5, 1e-15);
    within(logitShares({-5.0, -5.0, -5.0})[0], 1.0 / 3, 1e-15);
    const auto nested = nestedLogitShares({-5.0, -5.0, -5.0}, {-1, 0, 0}, {phi});
    within(nested[0], 1 / (1 + std::exp(phi * std::log(2.0))), 1e-12);
    within(nested[0], 0.5, 1e-3);
}

TEST(nestedlogit, nl5_codec) { // NL5
    const auto good = nestedJson();
    const auto set = parseModeChoiceSet(good, "s", true);
    CHECK(set.purposes[0].nests.size() == 1 && set.purposes[0].nests[0].phi == 0.5); // forcing: the nest was read
    CHECK(modeChoiceSetJson(set) == good);
    CHECK(!modeChoiceSetJson(parseModeChoiceSet(readJson(tdData() / "mode-choice/hand.json"), "s", true))["purposes"][0].contains("nests"));
    const auto bad = [&](const std::function<void(Json&)>& change, const std::string& path, const std::string& code = "MODE_CHOICE_INVALID_SET") {
        auto j = good; change(j);
        CHECK(has(refuses([&] { parseModeChoiceSet(j, "s", true); }), code, path));
    };
    const std::string nest = "s.purposes[0].nests[0]";
    bad([](Json& j) { j["purposes"][0]["nests"][0]["phi"] = 0; }, nest + ".phi");
    bad([](Json& j) { j["purposes"][0]["nests"][0]["phi"] = 1.2; }, nest + ".phi");
    bad([](Json& j) { j["purposes"][0]["nests"][0]["phi"] = -0.5; }, nest + ".phi");
    bad([](Json& j) { j["purposes"][0]["nests"][0]["phi"] = "half"; }, nest + ".phi");
    bad([](Json& j) { j["purposes"][0]["nests"][0]["alternatives"] = Json::array(); }, nest + ".alternatives");
    bad([](Json& j) { j["purposes"][0]["nests"][0]["alternatives"][1] = "bus"; }, nest + ".alternatives[1]");
    bad([](Json& j) { j["purposes"][0]["nests"] = Json::array(); }, "s.purposes[0].nests");
    bad([](Json& j) { j["purposes"][0]["nests"][0]["extra"] = 1; }, nest);
    bad([](Json& j) { j["purposes"][0]["nests"].push_back({{"id", "foot"}, {"phi", 1}, {"alternatives", {"car"}}}); },
        "s.purposes[0].nests[1].id", "DUPLICATE_ID");
    bad([](Json& j) { j["purposes"][0]["nests"].push_back({{"id", "two"}, {"phi", 1}, {"alternatives", {"walk"}}}); },
        "s.purposes[0].nests[1].alternatives[0]", "DUPLICATE_ID");
    bad([](Json& j) { j["purposes"][0]["nests"][0]["id"] = " "; }, nest + ".id", "INVALID_ID");
    auto one = good; one["purposes"][0]["nests"][0]["phi"] = 1;
    CHECK(parseModeChoiceSet(one, "s", true).purposes[0].nests[0].phi == 1); // the closed end is allowed
}

TEST(nestedlogit, nl6_schema_71) { // NL6
    auto owned = nestedJson(); owned["id"] = "nested-owned";
    auto j = fixtureJson(); j["schemaVersion"] = 71; j["modeChoiceSets"] = Json::array({owned});
    const auto d = parseDocument(j);
    CHECK(documentSchemaVersion(d) == 71); // forcing: nests select 71
    const auto saved = documentJson(d);
    CHECK(saved["schemaVersion"] == 71 && saved["modeChoiceSets"][0] == owned && documentJson(parseDocument(saved)) == saved);
    auto older = saved; older["schemaVersion"] = 70;
    CHECK(has(refuses([&] { parseDocument(older); }), "EDIT_UNSUPPORTED_FIELD", "modeChoiceSets[0].purposes[0].nests"));
    auto flat = d; flat.modeChoiceSets[0].purposes[0].nests.clear();
    CHECK(documentSchemaVersion(flat) == 70 && !documentJson(flat)["modeChoiceSets"][0]["purposes"][0].contains("nests"));
}

TEST(nestedlogit, nl7_run_and_output) { // NL7
    const auto d = parseDocument(fixtureJson());
    const auto r = runModeChoice(d, tdData(), "one-way", "hand", "", "", "nested", {});
    const auto& pa = r.purposes[0];
    const auto& trips = r.distribution.purposes[0].result.trips;
    CHECK(pa.alternatives.size() == 3 && trips[0][1] > 0); // forcing: three alternatives, trips to split
    const auto v = [&](std::size_t k) { const auto& a = pa.alternatives[k]; return a.alternative.constant + a.alternative.timeCoefficient * a.costs[0][1]; };
    const auto expected = nestedLogitShares({v(0), v(1), v(2)}, {-1, 0, 0}, {0.5});
    const auto flat = logitShares({v(0), v(1), v(2)});
    CHECK(std::abs(expected[0] - flat[0]) > 1e-3); // the nest changes the split
    for (std::size_t k = 0; k < 3; ++k) within(pa.alternatives[k].persons[0][1], expected[k] * trips[0][1], 1e-9);
    for (std::size_t i = 0; i < trips.size(); ++i)
        for (std::size_t k = 0; k < trips.size(); ++k)
            within(pa.alternatives[0].persons[i][k] + pa.alternatives[1].persons[i][k] + pa.alternatives[2].persons[i][k], trips[i][k], 1e-9);
    const auto j = modeChoiceJson(r);
    CHECK(j["validated"] == false && j["purposes"][0]["nests"] == nestedJson()["purposes"][0]["nests"]);
    const auto mnl = modeChoiceJson(runModeChoice(d, tdData(), "one-way", "hand", "", "", "hand", {}));
    CHECK(!mnl["purposes"][0].contains("nests"));
}
