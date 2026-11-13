// Procedure feedback rows FB1-FB8 (docs/reference/PROCEDURE_FEEDBACK.md §5, D547): the codec at schema 75, the step
// shape, the plain first pass, the congested skim, the averaging by hand, Ortuzar Example 11.3's fixed point under
// CW and MSA, naive feedback failing, the limit and Cancel. Each row first checks its forcing.
#include "test.hpp"
#include "../src/project/document.hpp"
#include "../src/project/document_schema.hpp"
#include "../src/project/procedure_output.hpp"
#include "../src/project/procedure_run.hpp"
#include "../src/project/zone_skim.hpp"
#include <cmath>
#include <fstream>
#include <iostream>
using namespace multra;
namespace {
std::filesystem::path fbData() { return test::root() / "tests/data/procedure-feedback"; }
Json fixtureJson() { std::ifstream f(fbData() / "example-11-3.traffic.json"); Json j; f >> j; return j; }
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
const ModeChoicePeriodAlternative& alternative(const ModeChoiceRun& r, const std::string& id) {
    for (const auto& a : r.periods.at(0).alternatives) if (a.id == id) return a;
    throw std::runtime_error("no alternative " + id);
}
double persons(const ModeChoiceRun& r, const std::string& id) { return alternative(r, id).persons.at(0).at(1); }
const ModeChoiceRun& modeChoiceOf(const ProcedureRun& r) { return *r.steps.at(2).modeChoice; }
ProcedureRun run(const ProjectDocument& d, const std::string& id, const ProcedureObserver* o = nullptr) {
    return runProcedure(d, fbData(), id, nullptr, o);
}
ModeChoiceRun auxiliary(const ProjectDocument& d, const std::string& set, const std::vector<double>& times) {
    return runModeChoice(d, fbData(), "example-11-3", "example-11-3", "", "", set, {"period-am"}, nullptr, {}, &times);
}
}
TEST(procfeedback, fb1_codec_and_schema_75) { // FB1
    const auto j = fixtureJson();
    const auto d = parseDocument(j);
    CHECK(d.procedures.size() == 8 && d.procedures[1].feedback && !d.procedures[0].feedback); // forcing: feedback was read
    CHECK(documentSchemaVersion(d) == 75);
    const auto saved = documentJson(d);
    CHECK(saved["procedures"] == j["procedures"] && documentJson(parseDocument(saved)) == saved);
    auto older = saved; older["schemaVersion"] = 74;
    CHECK(has(refuses([&] { parseDocument(older); }), "EDIT_UNSUPPORTED_FIELD", "procedures[1].feedback"));
    auto none = d;
    for (auto& p : none.procedures) p.feedback.reset();
    CHECK(documentSchemaVersion(none) == 73 && !documentJson(none)["procedures"][1].contains("feedback"));
    const auto bad = [&](const std::function<void(Json&)>& change, const std::string& path) {
        auto k = j; change(k);
        CHECK(has(refuses([&] { parseDocument(k); }), "INVALID_PROCEDURE", path));
    };
    const std::string p = "procedures[1].feedback";
    bad([](Json& k) { k["procedures"][1]["feedback"]["method"] = "naive"; }, p + ".method");
    bad([](Json& k) { k["procedures"][1]["feedback"]["weight"] = 1.0; }, p + ".weight");
    bad([](Json& k) { k["procedures"][1]["feedback"]["weight"] = -0.1; }, p + ".weight");
    bad([](Json& k) { k["procedures"][1]["feedback"].erase("weight"); }, p + ".weight");
    bad([](Json& k) { k["procedures"][2]["feedback"]["weight"] = 0.5; }, "procedures[2].feedback.weight");
    bad([](Json& k) { k["procedures"][1]["feedback"]["maxIterations"] = 1; }, p + ".maxIterations");
    bad([](Json& k) { k["procedures"][1]["feedback"]["maxIterations"] = 2.5; }, p + ".maxIterations");
    bad([](Json& k) { k["procedures"][1]["feedback"]["tolerance"] = -1; }, p + ".tolerance");
    bad([](Json& k) { k["procedures"][1]["feedback"].erase("tolerance"); }, p + ".tolerance");
    bad([](Json& k) { k["procedures"][1]["feedback"]["extra"] = 1; }, p);
    auto blank = d; blank.procedures[1].feedback->weight.reset();
    CHECK(has(procedureIssues(blank.procedures), "INVALID_PROCEDURE", p + ".weight"));
}
TEST(procfeedback, fb2_steps_shape) { // FB2
    const auto d = fixture();
    CHECK(run(d, "cw-005").feedback.has_value()); // forcing: the shape is accepted as shipped
    const auto refused = [&](const std::function<void(Procedure&)>& change) {
        auto k = d; change(k.procedures[1]);
        return has(refuses([&] { run(k, "cw-005"); }), "FEEDBACK_STEPS_INVALID", "procedures[1].feedback");
    };
    CHECK(refused([](Procedure& p) { p.steps.erase(p.steps.begin() + 2); }));                       // no mode choice
    CHECK(refused([](Procedure& p) { p.steps.insert(p.steps.begin() + 1, p.steps[1]); }));          // two distributions
    CHECK(refused([](Procedure& p) { p.steps.push_back(p.steps[0]); }));                            // assignment not last
    CHECK(refused([](Procedure& p) { std::swap(p.steps[2], p.steps[3]); }));                        // misordered
    CHECK(refused([](Procedure& p) { p.steps[2].active = false; }));                                // inactive mode choice
    auto ok = d; ok.procedures[1].steps.push_back(ok.procedures[1].steps[0]); ok.procedures[1].steps.back().active = false;
    CHECK(run(ok, "cw-005").feedback.has_value());                                                   // an inactive tail is fine
}
TEST(procfeedback, fb3_first_pass_is_the_plain_run) { // FB3
    const auto d = fixture();
    int calls = 0;
    ProcedureObserver first;
    first.step = [&](std::size_t) { return ++calls <= 4; }; // the four steps, then refuse iteration 2
    const auto r = run(d, "cw-005", &first);
    CHECK(calls == 5 && r.cancelled && r.feedback && r.feedback->iterations.size() == 1); // forcing
    const auto plain = run(d, "plain-005");
    CHECK(plain.completed() && !plain.feedback && !procedureJson(plain).contains("feedback"));
    CHECK(procedureJson(r)["steps"] == procedureJson(plain)["steps"]);
    const auto& it = r.feedback->iterations[0];
    CHECK(it.n == 1 && !it.weight && !it.tmf && !it.rse && it.relativeGap == plain.steps[3].macro->assignment.gap);
}
TEST(procfeedback, fb4_congested_skim) { // FB4
    const auto d = fixture();
    const auto plain = run(d, "plain-005");
    const auto& macro = *plain.steps[3].macro;
    CHECK(macro.assignment.time.size() == macro.graph.links.size() && macro.assignment.load.size() == macro.graph.links.size());
    const auto e = auxiliary(d, "lambda-005", macro.assignment.time);
    const auto& car = e.purposes.at(0).alternatives.at(0);
    const auto& rail = e.purposes.at(0).alternatives.at(1);
    const auto skim = zoneSkim(d, macro.graph, {"A", "B"}, "car", &macro.assignment.time);
    const auto free = zoneSkim(d, macro.graph, {"A", "B"}, "car");
    CHECK(skim[0][1] > free[0][1] + 1); // forcing: congestion is in the skim
    test::near(car.costs.at(0).at(1), skim[0][1]);
    test::near(rail.costs.at(0).at(1), 18); // the owned matrix still wins
    test::near(free[0][1], 10 + 0.006849, 1e-5); // the hand constants of §5: the town centre at free flow
}
TEST(procfeedback, fb5_averaging_by_hand) { // FB5
    auto d = fixture();
    for (const auto* id : {"cw-005", "msa-005"}) {
        auto k = d;
        for (auto& p : k.procedures) if (p.id == id) { p.feedback->maxIterations = 2; p.feedback->tolerance = 0; }
        const auto plain = run(k, "plain-005");
        const auto e = auxiliary(k, "lambda-005", plain.steps[3].macro->assignment.time);
        const auto r = run(k, id);
        const double w = std::string(id) == "cw-005" ? 0.25 : 0.5;
        CHECK(r.feedback && r.feedback->iterations.size() == 2 && !r.feedback->converged); // forcing: one averaging step
        const double t1 = persons(modeChoiceOf(plain), "car"), e2 = persons(e, "car");
        CHECK(std::abs(t1 - e2) > 1); // forcing: congestion moved demand
        test::near(persons(modeChoiceOf(r), "car"), w * t1 + (1 - w) * e2, 1e-9);
        test::near(alternative(modeChoiceOf(r), "car").vehicles->at(0).at(1), persons(modeChoiceOf(r), "car"), 1e-9); // occupancy 1
        const double tr = persons(modeChoiceOf(plain), "rail"), er = persons(e, "rail");
        test::near(*r.feedback->iterations[1].tmf, std::abs(t1 - e2) + std::abs(tr - er), 1e-9);
        test::near(*r.feedback->iterations[1].rse, std::sqrt((t1 - e2) * (t1 - e2) + (tr - er) * (tr - er)), 1e-9);
        test::near(*r.feedback->iterations[1].weight, w);
    }
}
TEST(procfeedback, fb6_example_11_3_lambda_005) { // FB6: hand values of §5
    const auto d = fixture();
    for (const auto* id : {"cw-005", "msa-005"}) {
        const auto r = run(d, id);
        CHECK(r.completed() && r.feedback && r.feedback->converged && r.feedback->iterations.size() >= 2);
        std::cerr << "  " << id << ": " << r.feedback->iterations.size() << " iterations, car " << persons(modeChoiceOf(r), "car") << '\n';
        test::near(persons(modeChoiceOf(r), "car"), 999.793, 1.0);
        test::near(persons(modeChoiceOf(r), "rail"), 1000.207, 1.0);
    }
}
TEST(procfeedback, fb7_example_11_3_lambda_034_and_naive) { // FB7
    const auto d = fixture();
    const auto naive = run(d, "naive-060"); // amended (§5): naive feedback's factor is about 2 lambda, 1.2 here
    CHECK(naive.completed() && naive.feedback && !naive.feedback->converged && naive.feedback->iterations.size() == 100); // forcing
    const auto averaged = run(d, "cw-060");
    CHECK(averaged.feedback && averaged.feedback->converged);
    for (const auto* id : {"cw-034", "msa-034"}) {
        const auto r = run(d, id);
        CHECK(r.feedback && r.feedback->converged);
        std::cerr << "  " << id << ": " << r.feedback->iterations.size() << " iterations, car " << persons(modeChoiceOf(r), "car") << '\n';
        test::near(persons(modeChoiceOf(r), "car"), 999.078, 1.0);
    }
}
TEST(procfeedback, fb8_limit_and_cancel) { // FB8
    auto d = fixture();
    d.procedures[1].feedback->maxIterations = 3; d.procedures[1].feedback->tolerance = 0;
    const auto limited = run(d, "cw-005");
    CHECK(limited.completed() && limited.feedback && !limited.feedback->converged && limited.feedback->iterations.size() == 3);
    CHECK(procedureJson(limited)["feedback"]["converged"] == false && procedureJson(limited)["feedback"]["iterations"].size() == 3);
    bool second = false; int calls = 0;
    ProcedureObserver inside;
    inside.step = [&](std::size_t) { if (++calls == 5) second = true; return true; };
    inside.macro.iteration = [&](const MacroIteration&) { return !second; };
    const auto cancelled = run(d, "cw-005", &inside);
    CHECK(second && cancelled.cancelled && cancelled.feedback && cancelled.feedback->iterations.size() == 1);
    CHECK(cancelled.steps[3].macro.has_value() && cancelled.steps[3].status == ProcedureStepStatus::done);
}
