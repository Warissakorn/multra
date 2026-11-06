// M10.4 E1 rows MC1-MC9 (docs/reference/MODE_CHOICE.md §12, D478): the multinomial logit split against
// Ortuzar and Willumsen Example 6.2 (book p. 214, read from the page image,
// docs/evidence/ortuzar-ch6-7-mode-choice.md), availability, the directional split, occupancy, the
// mode-choice codec and period factors at schema 69. Each row first checks its forcing.
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
constexpr double inf = std::numeric_limits<double>::infinity();
const double kNaN = std::numeric_limits<double>::quiet_NaN();
std::filesystem::path tdData() { return test::root() / "tests/data/trip-distribution"; }
Json readJson(const std::filesystem::path& p) { std::ifstream f(p); Json j; f >> j; return j; }
Json fixtureJson() { return readJson(tdData() / "mode-choice.traffic.json"); }
ProjectDocument fixture() { return parseDocument(fixtureJson()); }
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
double total(const DistributionMatrix& m) { double s = 0; for (const auto& r : m) for (const double x : r) s += x; return s; }
ModeChoiceRun run(const ProjectDocument& d, const std::vector<std::string>& periods = {}) {
    return runModeChoice(d, tdData(), "one-way", "hand", "", "", "hand", periods);
}
}

TEST(modechoice, mc1_example_6_2_red_bus_blue_bus) { // MC1
    const auto two = logitShares({-0.1 * 50, -0.1 * 50});
    within(two[0], 0.5, 1e-15); // forcing: car and bus at equal cost share half each
    const auto three = logitShares({-0.1 * 50, -0.1 * 50, -0.1 * 50});
    for (const double s : three) within(s, 1.0 / 3, 1e-15); // printed P_C = 0.33
}

TEST(modechoice, mc2_shares_by_hand_and_iia) { // MC2
    const std::vector<double> v{-1.0, -2.5};
    const auto s = logitShares(v);
    within(s[0], 1 / (1 + std::exp(-1.5)), 1e-12); within(s[0] + s[1], 1, 1e-15);
    const auto t = logitShares({-1.0, -2.5, -0.3});
    const double e = std::exp(-1.0) + std::exp(-2.5) + std::exp(-0.3);
    within(t[2], std::exp(-0.3) / e, 1e-12);
    within(t[0] / t[1], s[0] / s[1], 1e-12); // IIA: the ratio ignores the third alternative
    const auto shifted = logitShares({4.0, 2.5, 4.7});
    for (std::size_t k = 0; k < 3; ++k) within(shifted[k], t[k], 1e-12); // only differences matter
}

TEST(modechoice, mc3_availability) { // MC3
    const auto s = logitShares({-1.0, kNaN, -inf, -1.0});
    CHECK(s[1] == 0 && s[2] == 0); within(s[0], 0.5, 1e-15);
    CHECK(logitShares({kNaN, -inf}) == std::vector<double>({0, 0}));
    const DistributionMatrix trips{{0, 10}, {0, 0}};
    const auto ok = splitModes(trips, {{0, -0.1, {{inf, 5}, {inf, inf}}}, {0, -0.1, {{inf, inf}, {inf, inf}}}});
    CHECK(ok.issues.empty() && ok.persons[0][0][1] == 10 && ok.persons[1][0][1] == 0); // forcing: one alternative available
    const auto none = splitModes(trips, {{0, -0.1, {{inf, inf}, {inf, inf}}}});
    CHECK(none.issues.size() == 1 && none.issues[0].origin == 0 && none.issues[0].destination == 1 && none.persons.empty());
    // Through the run: a walk-only purpose where walk has no matrix cell refuses at the origin zone.
    auto j = fixtureJson(); j["costMatrices"][0]["rows"][0][1] = nullptr;
    auto d = parseDocument(j);
    CHECK(run(d).purposes[0].alternatives[1].persons[0][1] == 0); // car remains available
    const auto data = std::filesystem::temp_directory_path() / "multra-mc-walk-only";
    std::filesystem::remove_all(data);
    std::filesystem::copy(tdData(), data, std::filesystem::copy_options::recursive);
    auto walkOnly = readJson(tdData() / "mode-choice/hand.json");
    walkOnly["purposes"][0]["alternatives"].erase(0); walkOnly["purposes"][0]["alternatives"][0]["constant"] = 0;
    std::ofstream(data / "mode-choice/hand.json") << walkOnly.dump();
    CHECK(has(refuses([&] { runModeChoice(d, data, "one-way", "hand", "", "", "hand", {}); }), "MODE_CHOICE_NO_ALTERNATIVE", "zones[0].all"));
}

TEST(modechoice, mc4_large_costs_stay_finite) { // MC4
    CHECK(std::exp(-0.1 * 1e4) == 0); // forcing: the naive numerator underflows
    const auto s = logitShares({-0.1 * 1e4, -0.1 * (1e4 + 10)});
    CHECK(std::isfinite(s[0]) && std::isfinite(s[1]));
    within(s[0], 1 / (1 + std::exp(-1.0)), 1e-12);
}

TEST(modechoice, mc5_directional_split_and_factors) { // MC5
    const DistributionMatrix t{{1, 10}, {3, 0}};
    const auto od = directionalOd(t, 0.4, 0.1);
    within(od[0][1], 0.4 * 10 + 0.1 * 3, 1e-15); within(od[1][0], 0.4 * 3 + 0.1 * 10, 1e-15);
    within(od[0][0], 0.5, 1e-15);
    CHECK(od[0][1] != 0.4 * 10 + 0.1 * 10); // forcing: the return share uses the transpose
    auto j = fixtureJson();
    j["periodFactors"][0]["productionToAttraction"] = 0.95;
    CHECK(has(refuses([&] { parseDocument(j); }), "PERIOD_FACTOR_INVALID", "periodFactors[0]"));
    j = fixtureJson(); j["periodFactors"][0]["purposeId"] = "other";
    const auto missing = run(parseDocument(j), {"period-am"});
    CHECK(missing.advisories.size() == 1 && missing.advisories[0].code == "MODE_PERIOD_FACTOR_MISSING");
    CHECK(missing.periods[0].alternatives.empty() && missing.periods[0].modes.empty());
    CHECK(has(refuses([&] { run(fixture(), {"nope"}); }), "UNKNOWN_PERIOD", "studyPeriods"));
}

TEST(modechoice, mc6_occupancy_and_mc7_conservation) { // MC6, MC7
    const auto r = run(fixture());
    CHECK(r.periods.size() == 1 && r.periods[0].id == "period-am");
    const auto& pa = r.purposes[0];
    const auto& trips = r.distribution.purposes[0].result.trips;
    CHECK(total(trips) > 0); // forcing: there are trips to split
    // MC7: the alternatives' persons add to the P-A table, cell by cell.
    for (std::size_t i = 0; i < trips.size(); ++i)
        for (std::size_t k = 0; k < trips.size(); ++k) within(pa.alternatives[0].persons[i][k] + pa.alternatives[1].persons[i][k], trips[i][k], 1e-9);
    // The share follows the hand utilities on the run's own costs.
    const double car = pa.alternatives[0].costs[0][1];
    CHECK(std::isfinite(car) && pa.alternatives[1].costs[0][1] == 20 && std::isinf(pa.alternatives[1].costs[1][0]));
    within(pa.alternatives[0].persons[0][1], trips[0][1] * logitShares({-0.05 * car, -1 - 0.1 * 20})[0], 1e-9);
    // MC6: vehicles = persons / occupancy; walk has none; the car mode carries the car vehicles.
    const auto& p = r.periods[0];
    CHECK(p.alternatives.size() == 2 && p.alternatives[0].vehicles && !p.alternatives[1].vehicles);
    within(p.alternatives[0].persons[0][1], 0.4 * pa.alternatives[0].persons[0][1], 1e-9);
    within(p.alternatives[0].persons[1][0], 0.1 * pa.alternatives[0].persons[0][1], 1e-9);
    within((*p.alternatives[0].vehicles)[0][1], p.alternatives[0].persons[0][1] / 1.25, 1e-9);
    CHECK(p.modes.size() == 1 && p.modes[0].modeId == "car" && p.modes[0].vehicles == *p.alternatives[0].vehicles);
    // Two alternatives on one mode add their vehicles.
    const auto data = std::filesystem::temp_directory_path() / "multra-mc-two-car";
    std::filesystem::remove_all(data);
    std::filesystem::copy(tdData(), data, std::filesystem::copy_options::recursive);
    auto set = readJson(tdData() / "mode-choice/hand.json");
    set["purposes"][0]["alternatives"][1] = {{"id", "passenger"}, {"modeId", "car"}, {"constant", -1}, {"timeCoefficient", -0.05}, {"occupancy", 2}};
    std::ofstream(data / "mode-choice/hand.json") << set.dump();
    const auto two = runModeChoice(fixture(), data, "one-way", "hand", "", "", "hand", {});
    const auto& q = two.periods[0];
    within(q.modes[0].vehicles[0][1], (*q.alternatives[0].vehicles)[0][1] + (*q.alternatives[1].vehicles)[0][1], 1e-12);
    within((*q.alternatives[1].vehicles)[0][1], q.alternatives[1].persons[0][1] / 2, 1e-12);
    // Outputs carry the marker and the unit conditional.
    const auto j = modeChoiceJson(r);
    CHECK(j["validated"] == false && j["periods"][0]["modes"][0]["modeId"] == "car" && j["periods"][0]["alternatives"][1]["vehicles"].is_null());
    const auto csv = modeChoiceCsv(r);
    CHECK(csv.rfind("# MULTRA - not yet validated.", 0) == 0 && csv.find("only if the rate set counts daily person trips") != std::string::npos);
    CHECK(csv.find("\"period-am\",\"walk\",totals,,") != std::string::npos);
}

TEST(modechoice, mc8_codec) { // MC8
    const auto good = readJson(tdData() / "mode-choice/hand.json");
    const auto set = parseModeChoiceSet(good, "s", true);
    CHECK(modeChoiceSetJson(set) == good && set.purposes[0].alternatives[1].costMatrix == std::optional<std::string>("walk-times"));
    const auto bad = [&](const std::function<void(Json&)>& change, const std::string& path, const std::string& code = "MODE_CHOICE_INVALID_SET") {
        auto j = good; change(j);
        CHECK(has(refuses([&] { parseModeChoiceSet(j, "s", true); }), code, path));
    };
    bad([](Json& j) { j["purposes"][0]["alternatives"][0]["timeCoefficient"] = 0.1; }, "s.purposes[0].alternatives[0].timeCoefficient");
    bad([](Json& j) { j["purposes"][0]["alternatives"][0]["constant"] = 0.5; }, "s.purposes[0]");
    bad([](Json& j) { j["purposes"][0]["alternatives"][1].erase("costMatrix"); }, "s.purposes[0].alternatives[1]");
    bad([](Json& j) { j["purposes"][0]["alternatives"][0]["occupancy"] = 0.5; }, "s.purposes[0].alternatives[0].occupancy");
    bad([](Json& j) { j["purposes"][0]["alternatives"][1]["id"] = "car"; }, "s.purposes[0].alternatives[1].id", "DUPLICATE_ID");
    bad([](Json& j) { j["costUnit"] = "s"; }, "s.costUnit");
    bad([](Json& j) { j["source"].erase("citation"); }, "s.source.citation");
    CHECK(has(refuses([&] { runModeChoice(fixture(), tdData(), "one-way", "hand", "", "", "none", {}); }),
              "MODE_CHOICE_SET_UNKNOWN", "modeChoiceSets"));
    const auto data = std::filesystem::temp_directory_path() / "multra-mc-bad-mode";
    std::filesystem::remove_all(data);
    std::filesystem::copy(tdData(), data, std::filesystem::copy_options::recursive);
    auto unknownMode = good; unknownMode["purposes"][0]["alternatives"][0]["modeId"] = "bus";
    std::ofstream(data / "mode-choice/hand.json") << unknownMode.dump();
    CHECK(has(refuses([&] { runModeChoice(fixture(), data, "one-way", "hand", "", "", "hand", {}); }), "UNKNOWN_MODE",
              "catalog/mode-choice/hand.json.purposes[0].alternatives[0].modeId"));
    auto otherPurpose = good; otherPurpose["purposes"][0]["id"] = "HBW";
    std::ofstream(data / "mode-choice/hand.json") << otherPurpose.dump();
    CHECK(has(refuses([&] { runModeChoice(fixture(), data, "one-way", "hand", "", "", "hand", {}); }), "MODE_CHOICE_PURPOSE_MISSING",
              "catalog/mode-choice/hand.json.purposes"));
}

TEST(modechoice, mc9_period_factors_at_schema_69) { // MC9
    const auto j = fixtureJson();
    const auto d = parseDocument(j);
    CHECK(d.periodFactors.size() == 1 && documentSchemaVersion(d) == 69);
    const auto saved = documentJson(d);
    CHECK(saved["schemaVersion"] == 69 && saved["periodFactors"] == j["periodFactors"] && documentJson(parseDocument(saved)) == saved);
    auto older = saved; older["schemaVersion"] = 68;
    CHECK(has(refuses([&] { parseDocument(older); }), "EDIT_UNSUPPORTED_FIELD", "periodFactors"));
    auto none = d; none.periodFactors.clear();
    CHECK(documentSchemaVersion(none) < 69 && !documentJson(none).contains("periodFactors"));
    auto bad = j; bad["periodFactors"][0]["periodId"] = "gone";
    CHECK(has(refuses([&] { validateDocument(parseDocument(bad)); }), "UNKNOWN_PERIOD", "periodFactors[0].periodId"));
    bad = j; bad["periodFactors"].push_back(j["periodFactors"][0]);
    CHECK(has(refuses([&] { validateDocument(parseDocument(bad)); }), "DUPLICATE_ID", "periodFactors[1]"));
    bad = j; bad["periodFactors"] = Json::array();
    CHECK(has(refuses([&] { parseDocument(bad); }), "PERIOD_FACTOR_INVALID", "periodFactors"));
    bad = j; bad["periodFactors"][0]["extra"] = 1;
    CHECK(has(refuses([&] { parseDocument(bad); }), "PERIOD_FACTOR_INVALID", "periodFactors[0]"));
    bad = j; bad["periodFactors"][0]["attractionToProduction"] = -0.1;
    CHECK(has(refuses([&] { parseDocument(bad); }), "PERIOD_FACTOR_INVALID", "periodFactors[0].attractionToProduction"));
}
