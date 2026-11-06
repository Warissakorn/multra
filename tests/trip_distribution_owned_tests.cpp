// M10.3 D2 row TD12 and the override rows (docs/reference/TRIP_DISTRIBUTION.md §5-§6, §13, D474):
// project-owned deterrence sets and cost matrices at schema 67, their commands, how a run finds them,
// and a matrix overriding the skim. Each row first checks its forcing.
#include "test.hpp"
#include "../src/commands/history.hpp"
#include "../src/commands/trip_rate_commands.hpp"
#include "../src/project/document.hpp"
#include "../src/project/trip_distribution_output.hpp"
#include "../src/project/trip_distribution_run.hpp"
#include <cmath>
#include <fstream>
using namespace multra;
namespace {
Json readJson(const std::filesystem::path& p) { std::ifstream f(p); Json j; f >> j; return j; }
std::filesystem::path tdData() { return test::root() / "tests/data/trip-distribution"; }
ProjectDocument fixture() { return parseDocument(readJson(test::root() / "tests/trip-generation.traffic.json")); }
DeterrenceSet hand() { return parseDeterrenceSet(readJson(tdData() / "deterrence/hand.json"), "d", true); }
CostMatrix matrix(const std::string& id, const Json& zones, const Json& rows) {
    return parseCostMatrix({{"id", id}, {"name", id}, {"unit", "min"}, {"zoneIds", zones}, {"rows", rows}}, "m");
}
template<class F> std::vector<ValidationIssue> refuses(F f) {
    try { f(); } catch (const ValidationError& e) { return e.issues; }
    return {};
}
bool has(const std::vector<ValidationIssue>& issues, const std::string& code, const std::string& path) {
    return std::any_of(issues.begin(), issues.end(), [&](const auto& i) { return i.code == code && i.path == path; });
}
// A data root without the catalog's `hand` set, so the project's own is the only one.
std::filesystem::path dataWithoutCatalog() {
    const auto root = std::filesystem::temp_directory_path() / "multra-td-owned";
    std::filesystem::remove_all(root);
    std::filesystem::copy(tdData(), root, std::filesystem::copy_options::recursive);
    std::filesystem::remove_all(root / "deterrence");
    return root;
}
}

TEST(tripdistributionowned, td12_schema_67_round_trip_and_old_bytes) { // TD12
    auto d = fixture();
    const auto old = documentJson(d);
    CHECK(!old.contains("deterrenceSets") && !old.contains("costMatrices") && old["schemaVersion"] < 67);
    d.deterrenceSets = {hand()};
    d.costMatrices = {matrix("m1", {"site", "town"}, Json::array({Json::array({2, nullptr}), Json::array({5.5, 2})}))};
    const auto saved = documentJson(d);
    CHECK(saved["schemaVersion"] == 67 && saved["deterrenceSets"].size() == 1 && saved["costMatrices"].size() == 1);
    CHECK(saved["deterrenceSets"][0] == readJson(tdData() / "deterrence/hand.json"));
    CHECK(saved["costMatrices"][0]["rows"][0][1].is_null());
    CHECK(parseDocument(saved) == parseDocument(documentJson(parseDocument(saved))) && documentJson(parseDocument(saved)) == saved);
    for (const auto* key : {"deterrenceSets", "costMatrices"}) {
        auto older = saved; older["schemaVersion"] = 66;
        if (std::string(key) == "deterrenceSets") older.erase("costMatrices"); else older.erase("deterrenceSets");
        CHECK(older.contains(key));
        CHECK(has(refuses([&] { parseDocument(older); }), "EDIT_UNSUPPORTED_FIELD", key));
    }
    d.deterrenceSets.clear(); d.costMatrices.clear();
    CHECK(documentJson(d) == old);
    auto bad = saved; bad["deterrenceSets"][0]["purposes"][0]["beta"] = -1;
    CHECK(has(refuses([&] { parseDocument(bad); }), "INVALID_DETERRENCE_SET", "deterrenceSets[0].purposes[0].beta"));
    bad = saved; bad["costMatrices"][0]["rows"][1][0] = -3;
    CHECK(has(refuses([&] { parseDocument(bad); }), "INVALID_COST_MATRIX", "costMatrices[0].rows[1][0]"));
    bad = saved; bad["costMatrices"][0]["rows"][1] = Json::array({1});
    CHECK(has(refuses([&] { parseDocument(bad); }), "INVALID_COST_MATRIX", "costMatrices[0].rows[1]"));
    bad = saved; bad["costMatrices"][0]["zoneIds"][1] = "site";
    CHECK(has(refuses([&] { parseDocument(bad); }), "INVALID_COST_MATRIX", "costMatrices[0].zoneIds[1]"));
    bad = saved; bad["costMatrices"][0]["unit"] = "s";
    CHECK(has(refuses([&] { parseDocument(bad); }), "INVALID_COST_MATRIX", "costMatrices[0].unit"));
    bad = saved; bad["costMatrices"] = Json::array();
    CHECK(has(refuses([&] { parseDocument(bad); }), "INVALID_COST_MATRIX", "costMatrices"));
    bad = saved; bad["deterrenceSets"][0]["source"].erase("citation");
    CHECK(parseDocument(bad).deterrenceSets[0].citation.empty()); // a project set may omit its citation
}

TEST(tripdistributionowned, td12_commands_are_one_undo_step_and_refuse_duplicates) { // TD12
    History history; history.reset(fixture());
    CHECK(history.execute("set", [](ProjectDocument& d) { setDeterrenceSets(d, {hand()}); }));
    CHECK(history.execute("matrix", [](ProjectDocument& d) { setCostMatrices(d, {matrix("m1", {"site"}, Json::array({Json::array({1})}))}); }));
    const auto revision = history.revision();
    CHECK(has(refuses([&] { history.execute("dup", [](ProjectDocument& d) { setDeterrenceSets(d, {hand(), hand()}); }); }),
              "DUPLICATE_ID", "deterrenceSets[1].id"));
    CHECK(has(refuses([&] { history.execute("dup", [](ProjectDocument& d) {
        const auto m = matrix("m1", {"site"}, Json::array({Json::array({1})})); setCostMatrices(d, {m, m}); }); }), "DUPLICATE_ID", "costMatrices[1].id"));
    CHECK(history.revision() == revision && history.document().deterrenceSets.size() == 1 && history.document().costMatrices.size() == 1);
    history.undo();
    CHECK(history.document().costMatrices.empty() && history.document().deterrenceSets.size() == 1);
    history.undo();
    CHECK(history.document().deterrenceSets.empty());
    history.redo(); history.redo();
    CHECK(history.document().deterrenceSets.size() == 1 && history.document().costMatrices.size() == 1);
}

TEST(tripdistributionowned, td12_owned_sets_run_ahead_of_the_catalog) { // TD12
    auto d = fixture();
    const auto bare = dataWithoutCatalog();
    CHECK(has(refuses([&] { runTripDistribution(d, bare, "one-way", "hand", ""); }), "DISTRIBUTION_SET_UNKNOWN", "deterrenceSets"));
    d.deterrenceSets = {hand()};
    const auto run = runTripDistribution(d, bare, "one-way", "hand", "");
    CHECK(run.deterrencePath == "deterrenceSets[0]" && std::abs(run.purposes[0].result.trips[0][1] - 150) < 1e-9);
    // The same id in the catalog and the project is refused at the project's row, never silently overridden.
    CHECK(has(refuses([&] { runTripDistribution(d, tdData(), "one-way", "hand", ""); }), "DUPLICATE_ID", "deterrenceSets[0].id"));
}

TEST(tripdistributionowned, cost_matrix_overrides_the_skim) { // TD12 override rows (§5)
    auto d = fixture();
    const auto skimOnly = runTripDistribution(d, tdData(), "one-way", "hand", "");
    CHECK(std::isinf(skimOnly.costs[0][0]) && std::isinf(skimOnly.costs[1][0])); // the skim gives no diagonal
    d.costMatrices = {matrix("site-town", {"site", "town"}, Json::array({Json::array({nullptr, 7}), Json::array({nullptr, nullptr})}))};
    const auto one = runTripDistribution(d, tdData(), "one-way", "hand", "", "site-town");
    CHECK(one.costs[0][1] == 7 && std::isinf(one.costs[1][0]) && one.costMatrix == "site-town");
    CHECK(std::abs(one.purposes[0].result.trips[0][1] - 150) < 1e-9 && one.purposes[0].meanCost == 7);
    CHECK(tripDistributionJson(one)["skim"]["costMatrix"] == "site-town");
    // D1 refused Example 4.1 here (town reaches nothing); a full matrix with a diagonal makes it distributable.
    CHECK(has(refuses([&] { runTripDistribution(d, tdData(), "example-4-1", "hand", ""); }), "DISTRIBUTION_ROW_IMPOSSIBLE", "zones[1].all.productions"));
    d.costMatrices.push_back(matrix("full", {"town", "site"}, Json::array({Json::array({2, 5}), Json::array({5, 2})})));
    const auto full = runTripDistribution(d, tdData(), "example-4-1", "hand", "", "full");
    const auto& r = full.purposes[0].result;
    const auto& ends = full.generation.purposes[0];
    CHECK(r.converged && r.trips[0][0] > 0 && r.trips[1][1] > 0);
    for (std::size_t i = 0; i < 2; ++i) {
        CHECK(std::abs(r.trips[i][0] + r.trips[i][1] - ends.productions[i]) <= 1e-3 * ends.productions[i]);
        CHECK(std::abs(r.trips[0][i] + r.trips[1][i] - ends.attractions[i]) <= 1e-3 * ends.attractions[i]);
    }
    // With an intrazonal share the matrix diagonal leaves the gravity model (choice 2a).
    auto shared = hand(); shared.id = "shared"; shared.purposes[0].intrazonalShare = 0.5;
    d.deterrenceSets = {shared};
    DistributionSettings few; few.fixedPasses = 3;
    const auto withShare = runTripDistribution(d, tdData(), "example-4-1", "shared", "", "full", nullptr, {}, few);
    const auto& s = withShare.purposes[0].result;
    for (std::size_t i = 0; i < 2; ++i) CHECK(std::abs(s.trips[i][i] - s.intrazonal[i]) < 1e-9 && s.intrazonal[i] > 0);
    CHECK(has(refuses([&] { runTripDistribution(d, tdData(), "one-way", "hand", "", "none"); }), "COST_MATRIX_UNKNOWN", "costMatrices"));
    d.costMatrices.push_back(matrix("stale", {"site", "gone"}, Json::array({Json::array({1, 1}), Json::array({1, 1})})));
    CHECK(has(refuses([&] { runTripDistribution(d, tdData(), "one-way", "hand", "", "stale"); }), "COST_MATRIX_ZONE_UNKNOWN", "costMatrices[2].zoneIds[1]"));
}
