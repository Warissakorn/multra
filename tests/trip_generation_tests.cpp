// M10.2 G1 rows TG1-TG10, TG4b, TG12 (docs/reference/TRIP_GENERATION.md §13, D470): rate sets, the two
// methods, balancing by Eq. (4.14), refusals and outputs. Each row first checks its forcing.
#include "test.hpp"
#include "../src/macro/trip_generation.hpp"
#include "../src/project/trip_generation_output.hpp"
#include "../src/project/trip_generation_run.hpp"
#include "../src/project/trip_rate_codec.hpp"
#include "../src/project/macro_output.hpp"
#include "../src/project/document.hpp"
#include <fstream>
using namespace multra;
namespace {
std::filesystem::path tgData() { return test::root() / "tests/data/trip-generation"; }
Json readJson(const std::filesystem::path& p) { std::ifstream f(p); Json j; f >> j; return j; }
ProjectDocument fixture() { return parseDocument(readJson(test::root() / "tests/trip-generation.traffic.json")); }
template<class F> std::vector<ValidationIssue> refuses(F f) {
    try { f(); } catch (const ValidationError& e) { return e.issues; }
    return {};
}
bool has(const std::vector<ValidationIssue>& issues, const std::string& code, const std::string& path) {
    return std::any_of(issues.begin(), issues.end(), [&](const auto& i) { return i.code == code && i.path == path; });
}
// A test-only data root holding exactly the given sets.
std::filesystem::path dataWith(const std::string& name, const std::vector<Json>& sets) {
    const auto root = std::filesystem::temp_directory_path() / ("multra-tg-" + name);
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root / "trip-rates");
    for (std::size_t k = 0; k < sets.size(); ++k) std::ofstream(root / "trip-rates" / ("s" + std::to_string(k) + ".json")) << sets[k].dump();
    return root;
}
Json side(const std::string& method, const Json& items, double constant = 0) {
    if (method == "crossClassification") return {{"method", method}, {"categories", items}};
    return {{"method", method}, {"constant", constant}, {"terms", items}};
}
Json set(const std::string& id, const Json& productions, const Json& attractions) {
    return {{"id", id}, {"name", id}, {"source", {{"citation", "hand"}}}, {"unit", "trips"},
            {"purposes", Json::array({{{"id", "HBW"}, {"productions", productions}, {"attractions", attractions}}})}};
}
Json jobs(double coefficient = 1, double constant = 0) { return side("regression", Json::array({{{"attribute", "jobs"}, {"coefficient", coefficient}}}), constant); }
}

TEST(tripgeneration, tg1_projects_unchanged_without_a_set) { // TG1
    // No document field is read or written: shipped and Macro fixtures keep their bytes and schema.
    for (const auto& file : std::filesystem::directory_iterator(test::root() / "data/projects")) {
        if (file.path().extension() != ".json") continue;
        const auto j = readJson(file.path());
        CHECK(documentJson(parseDocument(j))["schemaVersion"] == j["schemaVersion"]);
    }
    const auto two = parseDocument(readJson(test::root() / "tests/macro-two-route.traffic.json"));
    CHECK(macroJson(runMacro(two, test::root() / "data", "matrix:am")).dump() ==
          macroJson(runMacro(parseDocument(documentJson(two)), test::root() / "data", "matrix:am")).dump());
    CHECK(resolveTripRateSets(test::root() / "data").empty()); // choice 8a: no shipped set
}

TEST(tripgeneration, tg2_codec_round_trip_and_refusals) { // TG2
    const auto good = readJson(tgData() / "trip-rates/example-4-4.json");
    const auto parsed = parseTripRateSet(good, "s", true);
    CHECK(parsed.purposes.size() == 1 && parsed.purposes[0].productions.terms.size() == 4);
    CHECK(tripRateSetJson(parsed) == good);
    CHECK(parseTripRateSet(tripRateSetJson(parsed), "s", true) == parsed);
    const auto refusedAt = [&](Json j, const std::string& code, const std::string& path, bool citation = true) {
        return has(refuses([&] { parseTripRateSet(j, "s", citation); }), code, path);
    };
    auto j = good; j["extra"] = 1; CHECK(refusedAt(j, "INVALID_TRIP_RATE_SET", "s"));
    j = good; j["id"] = " "; CHECK(refusedAt(j, "INVALID_ID", "s.id"));
    j = good; j["source"]["citation"] = ""; CHECK(refusedAt(j, "INVALID_TRIP_RATE_SET", "s.source.citation"));
    CHECK(parseTripRateSet(j, "s", false).citation.empty()); // a project set may omit it (G2)
    j = good; j["purposes"] = Json::array(); CHECK(refusedAt(j, "INVALID_TRIP_RATE_SET", "s.purposes"));
    j = good; j["purposes"].push_back(good["purposes"][0]); CHECK(refusedAt(j, "DUPLICATE_ID", "s.purposes[1].id"));
    j = good; j["purposes"][0]["productions"]["method"] = "logit"; CHECK(refusedAt(j, "INVALID_TRIP_RATE_SET", "s.purposes[0].productions.method"));
    j = good; j["purposes"][0]["productions"]["categories"] = Json::array();
    CHECK(refusedAt(j, "INVALID_TRIP_RATE_SET", "s.purposes[0].productions")); // regression with categories
    j = good; j["purposes"][0]["productions"]["terms"][1]["attribute"] = "households";
    CHECK(refusedAt(j, "INVALID_TRIP_RATE_SET", "s.purposes[0].productions.terms[1].attribute"));
    j = good; j["purposes"][0]["productions"]["terms"][0]["coefficient"] = "1";
    CHECK(refusedAt(j, "INVALID_TRIP_RATE_SET", "s.purposes[0].productions.terms[0].coefficient"));
    const auto cross = set("x", side("crossClassification", Json::array({{{"attribute", "a"}, {"rate", -1}}})), jobs());
    CHECK(has(refuses([&] { parseTripRateSet(cross, "s", true); }), "INVALID_TRIP_RATE_SET", "s.purposes[0].productions.categories[0].rate"));
    const auto empty = set("x", side("crossClassification", Json::array()), jobs());
    CHECK(has(refuses([&] { parseTripRateSet(empty, "s", true); }), "INVALID_TRIP_RATE_SET", "s.purposes[0].productions"));
    const auto twice = dataWith("dup", {set("a", jobs(), jobs()), set("a", jobs(), jobs())});
    CHECK(has(refuses([&] { resolveTripRateSets(twice); }), "DUPLICATE_ID", "catalog/trip-rates/s1.json.id"));
}

TEST(tripgeneration, tg3_tg4_the_two_methods) { // TG3, TG4
    TripSide cross{TripSide::Method::crossClassification, 0, {{0, 0.5}, {1, 1.25}, {2, 3}}, {}};
    CHECK(tripEnd(cross, {10, 4, 2}) == 10 * 0.5 + 4 * 1.25 + 2 * 3.0);
    CHECK(tripEnd(cross, {0, 0, 0}) == 0);
    TripSide regression{TripSide::Method::regression, 7, {}, {{1, 2}, {0, 0.5}}};
    CHECK(tripEnd(regression, {4, 3, 99}) == 7 + 2 * 3.0 + 0.5 * 4);
    TripSide zero{TripSide::Method::regression, 0, {}, {{0, 2}}};
    CHECK(tripEnd(zero, {0}) == 0);
}

TEST(tripgeneration, tg4b_example_4_4_zonal_form) { // TG4b
    const auto run = runTripGeneration(fixture(), tgData(), "example-4-4");
    CHECK(run.zoneIds == std::vector<std::string>({"site", "town"}));
    test::near(run.purposes[0].productions[0], 0.84 * 100 + 1.41 * 150 + 0.75 * 40 + 3.14 * 10, 1e-9);
    test::near(run.purposes[0].productions[0], 356.9, 1e-9);
    CHECK(run.purposes[0].productions[1] == 0);
}

TEST(tripgeneration, tg5_tg6_balancing) { // TG5, TG6
    const TripPurpose p{"HBW", {TripSide::Method::regression, 0, {}, {{0, 1}}}, {TripSide::Method::regression, 0, {}, {{1, 1}}}};
    const auto r = generateTrips({{30, 10}, {70, 30}}, {p});
    CHECK(r.issues.empty() && r.purposes[0].totalProductions == 100 && r.purposes[0].totalAttractionsRaw == 40);
    test::near(*r.purposes[0].factor, 2.5);
    test::near(r.purposes[0].totalAttractions, 100, 1e-12 * 100);
    CHECK(r.purposes[0].attractionsRaw == std::vector<double>({10, 30}));
    test::near(r.purposes[0].attractions[0], 25); test::near(r.purposes[0].attractions[1], 75);
    const auto impossible = generateTrips({{30, 0}}, {p});
    CHECK(impossible.purposes.empty() && impossible.issues.size() == 1 && impossible.issues[0].code == TripIssue::Code::balanceImpossible);
    const auto none = generateTrips({{0, 0}, {0, 0}}, {p});
    CHECK(none.issues.empty() && !none.purposes[0].factor && none.purposes[0].attractions == std::vector<double>({0, 0}));
    const auto d = dataWith("balance", {set("x", jobs(0), jobs())});
    auto doc = fixture();
    CHECK(!has(refuses([&] { runTripGeneration(doc, d, "x"); }), "TRIP_BALANCE_IMPOSSIBLE", "catalog/trip-rates/s0.json.purposes[0]")); // P = 0, A > 0
    const auto e = dataWith("balance2", {set("x", jobs(), jobs(0))});
    CHECK(has(refuses([&] { runTripGeneration(doc, e, "x"); }), "TRIP_BALANCE_IMPOSSIBLE", "catalog/trip-rates/s0.json.purposes[0]"));
}

TEST(tripgeneration, tg7_negative_is_refused_never_clamped) { // TG7
    const auto d = dataWith("negative", {set("x", jobs(), jobs(1, -200))});
    auto doc = fixture();
    CHECK(doc.zones.zones[0].attributes.at("jobs") == 100); // site: 100 - 200 < 0; town: 300 - 200 > 0
    std::vector<TripGenerationStage> stages;
    const auto issues = refuses([&] { runTripGeneration(doc, d, "x", nullptr, [&](TripGenerationStage s) { stages.push_back(s); }); });
    CHECK(issues.size() == 1 && has(issues, "TRIP_GENERATION_NEGATIVE", "zones[0].HBW.attractions"));
    CHECK(stages.back() == TripGenerationStage::generation);
}

TEST(tripgeneration, tg8_missing_attributes_listed_and_no_result) { // TG8
    auto doc = fixture();
    doc.zones.zones[1].attributes.erase("workers");
    doc.zones.zones[0].attributes.erase("households");
    CHECK(!doc.zones.zones[1].attributes.contains("workers"));
    const auto issues = refuses([&] { runTripGeneration(doc, tgData(), "example-4-4"); });
    CHECK(issues.size() == 2 && has(issues, "TRIP_RATE_ATTRIBUTE_MISSING", "zones[0].attributes.households") &&
          has(issues, "TRIP_RATE_ATTRIBUTE_MISSING", "zones[1].attributes.workers"));
    doc.zones.zones.clear();
    CHECK(has(refuses([&] { runTripGeneration(doc, tgData(), "example-4-4"); }), "TRIP_GENERATION_NO_ZONES", "zones"));
}

TEST(tripgeneration, tg9_same_bytes_and_permutation) { // TG9
    const auto doc = fixture();
    const auto once = tripGenerationJson(runTripGeneration(doc, tgData(), "example-4-1")).dump();
    CHECK(once == tripGenerationJson(runTripGeneration(doc, tgData(), "example-4-1")).dump());
    auto swapped = doc;
    std::swap(swapped.zones.zones[0], swapped.zones.zones[1]);
    CHECK(swapped.zones.zones[0].id == "town");
    CHECK(tripGenerationJson(runTripGeneration(swapped, tgData(), "example-4-1")).dump() == once);
    CHECK(tripGenerationCsv(runTripGeneration(swapped, tgData(), "example-4-1")) == tripGenerationCsv(runTripGeneration(doc, tgData(), "example-4-1")));
}

TEST(tripgeneration, tg10_data_swap_marker_and_unknown) { // TG10
    const auto doc = fixture();
    const auto a = tripGenerationJson(runTripGeneration(doc, tgData(), "example-4-1"));
    const auto b = tripGenerationJson(runTripGeneration(doc, tgData(), "example-4-4"));
    CHECK(a["purposes"] != b["purposes"]);
    CHECK(a["validated"] == false && a["rateSet"]["unit"] == "trips per day" && a["rateSet"]["source"]["citation"].get<std::string>().find("Example 4.1") != std::string::npos);
    const auto csv = tripGenerationCsv(runTripGeneration(doc, tgData(), "example-4-1"));
    CHECK(csv.rfind("# MULTRA - not yet validated.", 0) == 0);
    CHECK(csv.find("\"all\",totals,5125.00,400.00,5125.00,12.81\n") != std::string::npos);
    CHECK(has(refuses([&] { runTripGeneration(doc, tgData(), "none"); }), "TRIP_RATE_SET_UNKNOWN", "tripRateSets"));
}

TEST(tripgeneration, tg12_example_4_1) { // TG12: 2125 and 3000 trips/day, as printed (book pp. 143-144)
    const auto run = runTripGeneration(fixture(), tgData(), "example-4-1");
    CHECK(run.zoneIds[0] == "site" && run.zoneIds[1] == "town");
    CHECK(run.purposes[0].productions[0] == 2125); // 250 households at 2.5, 250 at 6.0
    CHECK(run.purposes[0].productions[1] == 3000); // 500 households at 6.0
    test::near(*run.purposes[0].factor, 5125.0 / 400.0);
}
