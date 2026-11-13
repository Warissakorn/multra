// Logsum composite cost rows LS1-LS8 (docs/reference/MODE_CHOICE_LOGSUM.md §6, D548): the codec at schema 76,
// the coefficient rules, the composite against Ortuzar Example 6.3 and the hand table, Eq. 6.11, availability,
// congested link times and byte-identity. Each row first checks its forcing.
#include "test.hpp"
#include "../src/macro/mode_choice.hpp"
#include "../src/project/document.hpp"
#include "../src/project/document_schema.hpp"
#include "../src/project/mode_choice_output.hpp"
#include "../src/project/mode_choice_run.hpp"
#include "../src/project/procedure_run.hpp"
#include "../src/project/trip_distribution_output.hpp"
#include "../src/project/zone_skim.hpp"
#include <cmath>
#include <fstream>
#include <iostream>
using namespace multra;
namespace {
std::filesystem::path lsData() { return test::root() / "tests/data/mode-choice-logsum"; }
Json readJson(const std::filesystem::path& p) { std::ifstream f(p); Json j; f >> j; return j; }
ProjectDocument fixture() { return parseDocument(readJson(lsData() / "logsum.traffic.json")); }
template<class F> std::vector<ValidationIssue> refuses(F f) {
    try { f(); } catch (const ValidationError& e) { return e.issues; }
    return {};
}
bool has(const std::vector<ValidationIssue>& issues, const std::string& code, const std::string& path) {
    const bool found = std::any_of(issues.begin(), issues.end(), [&](const auto& i) { return i.code == code && i.path == path; });
    if (!found) for (const auto& i : issues) std::cerr << "  got " << i.code << " at " << i.path << '\n';
    return found;
}
ModeChoiceRun run(const ProjectDocument& d, const std::string& set, const std::string& deterrence = "origin-005") {
    return runModeChoice(d, lsData(), "hand", deterrence, "", "car-times", set, {"period-am"});
}
// Zones by id: generation orders them its own way. j: 0 = O, 1 = D1, 2 = D2.
std::size_t zone(const ModeChoiceRun& r, std::size_t j) {
    const auto& ids = r.distribution.generation.zoneIds;
    const char* names[] = {"O", "D1", "D2"};
    return static_cast<std::size_t>(std::find(ids.begin(), ids.end(), names[j]) - ids.begin());
}
double trips(const ModeChoiceRun& r, std::size_t j) { return r.distribution.purposes.at(0).result.trips.at(zone(r, 0)).at(zone(r, j)); }
double persons(const ModeChoiceRun& r, std::size_t k, std::size_t j) { return r.purposes.at(0).alternatives.at(k).persons.at(zone(r, 0)).at(zone(r, j)); }
}
TEST(modechoicelogsum, ls1_codec_and_schema_76) { // LS1
    auto j = readJson(lsData() / "logsum.traffic.json");
    const auto plain = parseDocument(j);
    CHECK(documentSchemaVersion(plain) == 73 && !documentJson(plain).contains("modeChoiceSets")); // forcing: no owned set yet
    j["modeChoiceSets"] = Json::array({readJson(lsData() / "mode-choice/logsum-hand.json")});
    j["modeChoiceSets"][0]["id"] = "owned-logsum";
    j["schemaVersion"] = 76;
    const auto d = parseDocument(j);
    CHECK(d.modeChoiceSets.size() == 1 && d.modeChoiceSets[0].purposes[0].distributionCost == std::optional<std::string>("logsum"));
    CHECK(documentSchemaVersion(d) == 76);
    const auto saved = documentJson(d);
    CHECK(saved["modeChoiceSets"][0]["purposes"][0]["distributionCost"] == "logsum" && documentJson(parseDocument(saved)) == saved);
    auto older = saved; older["schemaVersion"] = 75;
    CHECK(has(refuses([&] { parseDocument(older); }), "EDIT_UNSUPPORTED_FIELD", "modeChoiceSets[0].purposes[0].distributionCost"));
    auto none = d; none.modeChoiceSets[0].purposes[0].distributionCost.reset();
    CHECK(documentSchemaVersion(none) == 73 && !documentJson(none)["modeChoiceSets"][0]["purposes"][0].contains("distributionCost"));
    auto bad = saved; bad["modeChoiceSets"][0]["purposes"][0]["distributionCost"] = "skim";
    CHECK(has(refuses([&] { parseDocument(bad); }), "MODE_CHOICE_INVALID_SET", "modeChoiceSets[0].purposes[0].distributionCost"));
}
TEST(modechoicelogsum, ls2_one_negative_coefficient) { // LS2
    auto j = readJson(lsData() / "logsum.traffic.json");
    j["schemaVersion"] = 76;
    j["modeChoiceSets"] = Json::array({readJson(lsData() / "mode-choice/logsum-hand.json")});
    CHECK(parseDocument(j).modeChoiceSets.size() == 1); // forcing: the shared coefficient is accepted
    const std::string at = "modeChoiceSets[0].purposes[0].distributionCost";
    auto differ = j; differ["modeChoiceSets"][0]["purposes"][0]["alternatives"][1]["timeCoefficient"] = -0.2;
    CHECK(has(refuses([&] { parseDocument(differ); }), "MODE_CHOICE_INVALID_SET", at));
    auto zero = j;
    for (auto& a : zero["modeChoiceSets"][0]["purposes"][0]["alternatives"]) a["timeCoefficient"] = 0;
    CHECK(has(refuses([&] { parseDocument(zero); }), "MODE_CHOICE_INVALID_SET", at));
}
TEST(modechoicelogsum, ls3_composite_by_hand) { // LS3
    // (a) Ortuzar Example 6.3: the bus nest at 50 min, lambda2 = 0.9 -> C_B = 50 - ln 2 / 0.9.
    const DistributionMatrix fifty{{50.0}};
    const auto bus = compositeCost({{0, -0.1, fifty}, {0, -0.1, fifty}}, {{0, 0}, {0.1 / 0.9}}, 0.1);
    test::near(bus[0][0], 49.2298, 1e-4);
    test::near(bus[0][0], 50 - std::log(2.0) / 0.9, 1e-12);
    const auto viaRun = run(fixture(), "bus-6-3");
    CHECK(viaRun.distribution.purposes[0].costSource == "logsum"); // forcing: the composite reached the run
    test::near(viaRun.distribution.purposes[0].meanCost, 50 - std::log(2.0) / 0.9, 1e-9);
    // (b) the hand table's K, as the distribution's mean cost per destination.
    const DistributionMatrix car{{INFINITY, 10, 20}, {INFINITY, INFINITY, INFINITY}, {INFINITY, INFINITY, INFINITY}};
    const DistributionMatrix walk{{INFINITY, 15, 60}, {INFINITY, INFINITY, INFINITY}, {INFINITY, INFINITY, INFINITY}};
    const auto k = compositeCost({{0, -0.1, car}, {-0.5, -0.1, walk}}, {}, 0.1);
    test::near(k[0][1], 6.867383, 1e-6);
    test::near(k[0][2], 19.889523, 1e-6);
    CHECK(std::isinf(k[1][2]) && std::isinf(k[0][0]));
}
TEST(modechoicelogsum, ls4_distribution_and_split_by_hand) { // LS4
    const auto d = fixture();
    const auto skim = run(d, "skim-hand");
    test::near(trips(skim, 1), 62.245933, 1e-6); // forcing: today's form, the car skim
    CHECK(skim.distribution.purposes[0].costSource.empty());
    const auto r = run(d, "logsum-hand");
    test::near(trips(r, 1), 65.725987, 1e-6);
    test::near(trips(r, 2), 34.274013, 1e-6);
    test::near(persons(r, 0, 1), 48.049547, 1e-6);
    test::near(persons(r, 1, 1), 17.676440, 1e-6);
    test::near(persons(r, 0, 2), 33.897446, 1e-6);
    test::near(persons(r, 1, 2), 0.376567, 1e-6);
}
TEST(modechoicelogsum, ls5_eq_6_11) { // LS5
    const auto d = fixture();
    CHECK(run(d, "logsum-hand", "origin-010").distribution.purposes[0].costSource == "logsum"); // beta = lambda passes
    CHECK(has(refuses([&] { run(d, "logsum-hand", "origin-020"); }), "MODE_CHOICE_STRUCTURE_INVALID", "catalog/mode-choice/logsum-hand.json.purposes[0]"));
    CHECK(run(d, "skim-hand", "origin-020").distribution.purposes[0].costSource.empty()); // no logsum, no check
}
TEST(modechoicelogsum, ls6_availability_and_phi_one) { // LS6
    const auto d = fixture();
    const auto r = run(d, "logsum-hand");
    CHECK(trips(r, 0) == 0); // O to O: no alternative has the diagonal, K = infinity, no trips
    const auto nested = run(d, "logsum-nest-1");
    test::near(trips(nested, 1), trips(r, 1), 1e-12);
    test::near(trips(nested, 2), trips(r, 2), 1e-12);
    const std::vector<double> v{-1.0, -2.0, std::numeric_limits<double>::quiet_NaN()};
    test::near(logsum(v, {}, {}), std::log(std::exp(-1.0) + std::exp(-2.0)), 1e-15);
    CHECK(std::isinf(logsum({std::numeric_limits<double>::quiet_NaN()}, {}, {})));
}
TEST(modechoicelogsum, ls7_congested_link_times) { // LS7
    const auto fb = test::root() / "tests/data/procedure-feedback";
    const auto d = parseDocument(readJson(fb / "example-11-3.traffic.json"));
    const auto plain = runProcedure(d, fb, "plain-005");
    const auto& macro = *plain.steps[3].macro;
    const auto r = runModeChoice(d, fb, "example-11-3", "example-11-3", "", "", "lambda-034-logsum", {"period-am"}, nullptr, {}, &macro.assignment.time);
    const double car = zoneSkim(d, macro.graph, {"A", "B"}, "car", &macro.assignment.time)[0][1];
    const double free = zoneSkim(d, macro.graph, {"A", "B"}, "car")[0][1];
    CHECK(car > free + 1 && r.distribution.purposes[0].costSource == "logsum"); // forcing: congestion and the composite
    test::near(r.distribution.purposes[0].meanCost, -std::log(std::exp(-0.34 * car) + std::exp(-0.34 * 18)) / 0.34, 1e-9);
}
TEST(modechoicelogsum, ls8_only_when_used) { // LS8
    const auto d = fixture();
    const auto skim = run(d, "skim-hand");
    CHECK(!tripDistributionJson(skim.distribution)["purposes"][0].contains("costSource"));
    CHECK(!modeChoiceJson(skim).dump().empty() && modeChoiceJson(skim).dump().find("distributionCost") == std::string::npos);
    const auto r = run(d, "logsum-hand");
    CHECK(tripDistributionJson(r.distribution)["purposes"][0]["costSource"] == "logsum");
    CHECK(modeChoiceJson(r).dump().find("\"distributionCost\":\"logsum\"") != std::string::npos);
}
