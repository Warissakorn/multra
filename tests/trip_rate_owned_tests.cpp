// M10.2 G2 row TG11 and the owned-set run rows (docs/reference/TRIP_GENERATION.md §6.3, §13, D471):
// project-owned trip-rate sets at schema 66, their command, and how a run finds them. Each row first
// checks its forcing.
#include "test.hpp"
#include "../src/commands/history.hpp"
#include "../src/commands/trip_rate_commands.hpp"
#include "../src/project/document.hpp"
#include "../src/project/trip_generation_run.hpp"
#include <fstream>
using namespace multra;
namespace {
Json readJson(const std::filesystem::path& p) { std::ifstream f(p); Json j; f >> j; return j; }
std::filesystem::path tgData() { return test::root() / "tests/data/trip-generation"; }
ProjectDocument fixture() { return parseDocument(readJson(test::root() / "tests/trip-generation.traffic.json")); }
TripRateSet example(const char* file) { return parseTripRateSet(readJson(tgData() / "trip-rates" / file), "s", true); }
template<class F> std::vector<ValidationIssue> refuses(F f) {
    try { f(); } catch (const ValidationError& e) { return e.issues; }
    return {};
}
bool has(const std::vector<ValidationIssue>& issues, const std::string& code, const std::string& path) {
    return std::any_of(issues.begin(), issues.end(), [&](const auto& i) { return i.code == code && i.path == path; });
}
}

TEST(triprateowned, tg11_schema_66_round_trip_and_old_bytes) { // TG11
    auto d = fixture();
    const auto old = documentJson(d);
    CHECK(!old.contains("tripRateSets") && old["schemaVersion"] < 66);
    d.tripRateSets = {example("example-4-1.json"), example("example-4-4.json")};
    const auto saved = documentJson(d);
    CHECK(saved["schemaVersion"] == 66 && saved["tripRateSets"].size() == 2);
    CHECK(saved["tripRateSets"][0] == readJson(tgData() / "trip-rates/example-4-1.json"));
    CHECK(parseDocument(saved).tripRateSets == d.tripRateSets && documentJson(parseDocument(saved)) == saved);
    auto older = saved; older["schemaVersion"] = 65;
    CHECK(older.contains("tripRateSets"));
    CHECK(has(refuses([&] { parseDocument(older); }), "EDIT_UNSUPPORTED_FIELD", "tripRateSets"));
    d.tripRateSets.clear();
    CHECK(documentJson(d) == old);
    auto bad = saved; bad["tripRateSets"] = Json::array();
    CHECK(has(refuses([&] { parseDocument(bad); }), "INVALID_TRIP_RATE_SET", "tripRateSets"));
    bad = saved; bad["tripRateSets"][1]["purposes"][0]["productions"]["method"] = "logit";
    CHECK(has(refuses([&] { parseDocument(bad); }), "INVALID_TRIP_RATE_SET", "tripRateSets[1].purposes[0].productions.method"));
    bad = saved; bad["tripRateSets"][1]["id"] = "example-4-1";
    CHECK(has(refuses([&] { parseDocument(bad); }), "DUPLICATE_ID", "tripRateSets[1].id"));
    bad = saved; bad["tripRateSets"][0]["source"].erase("citation");
    CHECK(parseDocument(bad).tripRateSets[0].citation.empty()); // a project set may omit its citation
}

TEST(triprateowned, tg11_command_is_one_undo_step_and_refuses_duplicates) { // TG11
    History history; history.reset(fixture());
    CHECK(!history.canUndo());
    CHECK(history.execute("set", [](ProjectDocument& d) { setTripRateSets(d, {example("example-4-1.json")}); }));
    CHECK(history.document().tripRateSets.size() == 1);
    const auto revision = history.revision();
    CHECK(has(refuses([&] { history.execute("dup", [](ProjectDocument& d) {
        setTripRateSets(d, {example("example-4-1.json"), example("example-4-1.json")}); }); }), "DUPLICATE_ID", "tripRateSets[1].id"));
    CHECK(history.revision() == revision && history.document().tripRateSets.size() == 1);
    CHECK(!history.execute("same", [](ProjectDocument& d) { setTripRateSets(d, {example("example-4-1.json")}); }));
    history.undo();
    CHECK(history.document().tripRateSets.empty());
    history.redo();
    CHECK(history.document().tripRateSets.size() == 1);
}

TEST(triprateowned, owned_sets_run_ahead_of_the_catalog) {
    auto d = fixture();
    const auto empty = test::root() / "data"; // ships no trip-rates folder (choice 8a)
    CHECK(has(refuses([&] { runTripGeneration(d, empty, "example-4-1"); }), "TRIP_RATE_SET_UNKNOWN", "tripRateSets"));
    d.tripRateSets = {example("example-4-1.json")};
    const auto run = runTripGeneration(d, empty, "example-4-1");
    CHECK(run.setPath == "tripRateSets[0]" && run.purposes[0].productions[0] == 2125);
    // The same id in the catalog and the project is refused at the project's row, never silently overridden.
    CHECK(has(refuses([&] { runTripGeneration(d, tgData(), "example-4-1"); }), "DUPLICATE_ID", "tripRateSets[0].id"));
    // Another catalog set still runs beside an owned one.
    CHECK(runTripGeneration(d, tgData(), "example-4-4").setPath == "catalog/trip-rates/example-4-4.json");
}
