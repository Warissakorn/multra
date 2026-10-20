// S1.1 rows SP1-SP6 and SP8 (docs/reference/STUDY_PERIODS.md §3-§5, D268): study periods at schema
// 49, the run's clockStart, and an OD matrix placed by its period. Each row first checks its forcing.
#include "test.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/commands/zone_commands.hpp"
#include "../src/project/evaluation.hpp"
#include "../src/project/od_demand.hpp"
#include "../src/project/run.hpp"
#include "../src/project/zone_codec.hpp"
#include "../src/runner/batch.hpp"
#include <filesystem>
#include <fstream>
using namespace trafficsim;
namespace {
std::string laneOf(const ProjectDocument& d, const std::string& link) {
    for (const auto& l : d.network.links) if (l.id == link) return l.lanes.front().id;
    throw std::runtime_error("no link " + link);
}
// Two entry Links feeding one terminal Link, a zone at each end, and one matrix over run seconds
// [600, 1800) -- the run-second twin of a period 07:10-07:30 under clockStart 07:00.
ProjectDocument twin() {
    ProjectDocument d;
    const auto a1 = addLink(d, {{0, 0}, {100, 0}}, 1, 3.5), a2 = addLink(d, {{0, 40}, {100, 40}}, 1, 3.5);
    const auto t = addLink(d, {{130, 20}, {230, 20}}, 1, 3.5);
    addConnector(d, {a1, laneOf(d, a1), 100}, {t, laneOf(d, t), 0});
    addConnector(d, {a2, laneOf(d, a2), 100}, {t, laneOf(d, t), 0});
    auto& def = demand(d); def.duration = 2400; def.timeStep = 0.5;
    putZone(d, {"site", "", {{-10, -10}, {-10, 50}, {-30, 20}}, {}});
    putZone(d, {"town", "", {{240, 0}, {260, 40}, {260, 0}}, {}});
    putZoneConnector(d, {"s1", "site", a1, ZoneDirection::origin, 2});
    putZoneConnector(d, {"s2", "site", a2, ZoneDirection::origin, 1});
    putZoneConnector(d, {"t1", "town", t, ZoneDirection::destination, 1});
    putOdMatrix(d, {"am", 600, 1800, "car-only", {"site", "town"}, {{0, 60}, {0, 0}}});
    return d;
}
constexpr double kSeven = 7 * 3600;
ProjectDocument placed() {
    auto d = twin();
    putStudyPeriod(d, {"period-am", "AM", kSeven + 600, kSeven + 1800});
    d.definition->clockStart = kSeven;
    auto m = d.zones.odMatrices.front(); m.startTime = 0; m.endTime = 0; m.periodId = "period-am";
    putOdMatrix(d, m);
    return d;
}
Json reread(const ProjectDocument& d) { return Json::parse(documentJson(d).dump()); }
bool refusedAt(const Json& j, const std::string& code, const std::string& path) {
    try { parseDocument(j); } catch (const ValidationError& e) {
        return std::any_of(e.issues.begin(), e.issues.end(), [&](const auto& i) { return i.code == code && i.path == path; });
    } catch (...) {}
    return false;
}
bool issueAt(const std::vector<ValidationIssue>& issues, const std::string& code, const std::string& path) {
    return std::any_of(issues.begin(), issues.end(), [&](const auto& i) { return i.code == code && i.path == path; });
}
}
TEST(studyperiods, sp1_files_without_a_period_keep_their_bytes) { // SP1
    std::vector<std::filesystem::path> files;
    for (const auto& entry : std::filesystem::directory_iterator(test::root() / "data/projects"))
        if (entry.path().string().ends_with(".traffic.json")) files.push_back(entry.path());
    CHECK(files.size() == 4);
    for (const auto& file : files) {
        std::ifstream in(file); const auto j = Json::parse(in);
        CHECK(documentJson(parseDocument(j)) == j);
    }
    // A schema-31 OD project: no period, its own times, saved at 31 with the same bytes.
    const auto saved = reread(twin());
    CHECK(saved["schemaVersion"] == 31 && !saved.contains("studyPeriods"));
    CHECK(saved["odMatrices"][0].contains("startTime") && !saved["odMatrices"][0].contains("periodId"));
    CHECK(documentJson(parseDocument(saved)) == saved);
}
TEST(studyperiods, sp2_a_period_round_trips_at_49_and_is_refused_before) { // SP2
    const auto d = placed();
    const auto j = reread(d);
    CHECK(j["schemaVersion"] == 49);
    CHECK(j["studyPeriods"] == Json::parse(R"([{"id":"period-am","name":"AM","start":25800.0,"end":27000.0}])"));
    CHECK(j["definition"]["clockStart"] == kSeven);
    CHECK(j["odMatrices"][0]["periodId"] == "period-am" && !j["odMatrices"][0].contains("startTime"));
    CHECK(parseDocument(j) == d);
    auto older = j; older["schemaVersion"] = 48;
    CHECK(refusedAt(older, "EDIT_UNSUPPORTED_FIELD", "studyPeriods"));
    older.erase("studyPeriods");
    CHECK(refusedAt(older, "EDIT_UNSUPPORTED_FIELD", "definition.clockStart"));
    older["definition"].erase("clockStart");
    CHECK(refusedAt(older, "EDIT_UNSUPPORTED_FIELD", "odMatrices[0].periodId"));
    // Bad fields, each at its path.
    auto bad = d;
    bad.zones.studyPeriods.push_back({"period-am", "", -1, 30});
    bad.zones.studyPeriods.push_back({"period-x", "", 100, 50});
    const auto issues = zoneLayerIssues(bad);
    CHECK(issueAt(issues, "PERIOD_INVALID", "studyPeriods[1].id") && issueAt(issues, "PERIOD_INVALID", "studyPeriods[1].start"));
    CHECK(issueAt(issues, "PERIOD_INVALID", "studyPeriods[2].end") && !issueAt(issues, "PERIOD_INVALID", "studyPeriods[0].end"));
    auto late = d; late.zones.studyPeriods[0].end = 172801;
    CHECK(issueAt(zoneLayerIssues(late), "PERIOD_INVALID", "studyPeriods[0].end"));
}
TEST(studyperiods, sp3_a_placed_matrix_expands_like_its_run_second_twin) { // SP3
    const auto a = withOdDemand(placed()), b = withOdDemand(twin());
    CHECK(!a.inputs.empty());
    CHECK(a.inputs == b.inputs && a.routes == b.routes);
    CHECK(a.inputs.front().startTime == 600 && a.inputs.front().endTime == 1800);
}
TEST(studyperiods, sp4_clock_start_shifts_the_window_and_the_run_bounds_it) { // SP4
    auto d = placed();
    d.definition->clockStart = kSeven + 300;
    CHECK(odMatrixWindow(d, d.zones.odMatrices[0]) == std::optional(std::pair{300.0, 1500.0}));
    CHECK(withOdDemand(d).inputs.front().startTime == 300);
    const auto data = test::root() / "data";
    CHECK(!issueAt(odDemandIssues(d, data), "OD_PERIOD_INVALID", "odMatrices[0]"));
    d.definition->clockStart = kSeven + 900; // the period would start before run time 0
    CHECK(issueAt(odDemandIssues(d, data), "OD_PERIOD_INVALID", "odMatrices[0]"));
    d.definition->clockStart = kSeven - 900; // ... or end after the run
    CHECK(issueAt(odDemandIssues(d, data), "OD_PERIOD_INVALID", "odMatrices[0]"));
    for (const double clock : {-1.0, 172801.0}) {
        auto j = reread(placed()); j["definition"]["clockStart"] = clock;
        CHECK(refusedAt(j, "INVALID_NUMBER", "definition.clockStart"));
    }
    auto text = reread(placed()); text["definition"]["clockStart"] = "07:00";
    CHECK(refusedAt(text, "INVALID_NUMBER", "definition.clockStart"));
}
TEST(studyperiods, sp5_one_form_of_period_and_a_known_one) { // SP5
    const auto j = reread(placed());
    auto both = j; both["odMatrices"][0]["startTime"] = 0.0; both["odMatrices"][0]["endTime"] = 60.0;
    CHECK(refusedAt(both, "OD_PERIOD_AMBIGUOUS", "odMatrices[0]"));
    auto neither = j; neither["odMatrices"][0].erase("periodId");
    CHECK(refusedAt(neither, "OD_PERIOD_AMBIGUOUS", "odMatrices[0]"));
    auto unknown = j; unknown["odMatrices"][0]["periodId"] = "period-pm";
    CHECK(refusedAt(unknown, "UNKNOWN_PERIOD", "odMatrices[0].periodId"));
}
TEST(studyperiods, sp6_a_period_in_use_is_not_deleted) { // SP6
    auto d = placed();
    const auto before = d;
    test::throws([&] { deleteStudyPeriod(d, "period-am"); }, "EDIT_PERIOD_IN_USE");
    CHECK(d == before);
    const auto spare = putStudyPeriod(d, {"", "PM", 61200, 64800});
    CHECK(spare.rfind("period-", 0) == 0);
    deleteStudyPeriod(d, spare);
    CHECK(d.zones.studyPeriods.size() == 1);
    test::throws([&] { deleteStudyPeriod(d, spare); }, "EDIT_UNKNOWN_OBJECT");
}
TEST(studyperiods, sp8_outputs_equal_the_run_second_twin) { // SP8
    const auto data = test::root() / "data";
    const auto run = [&](const ProjectDocument& d) {
        const auto snapshot = compileDocument(d, data);
        return runSeeds(snapshot.scenario, evaluationSpec(d, snapshot, data), {42, 43});
    };
    const auto a = run(placed()), b = run(twin());
    CHECK(a.front().report.completed > 0);
    CHECK(movementJson(a.front().report) == movementJson(b.front().report));
    CHECK(movementCsv(a.front().report) == movementCsv(b.front().report));
    CHECK(aggregate(a) == aggregate(b));
}
