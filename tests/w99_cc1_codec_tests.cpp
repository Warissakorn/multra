#include "test.hpp"
#include <functional>
#include <limits>
#include "../src/project/latest_schema.hpp"
#include "../src/commands/behaviour_commands.hpp"
#include "../src/commands/catalog_commands.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/core/w99.hpp"
#include "../src/project/behaviour_library.hpp"
#include "../src/project/run.hpp"
using namespace multra;
// W99 §5a (D646): row BA233 of docs/plans/DRIVING_BEHAVIOUR.md -- CC1 as a cumulative curve, the
// codec and the schema floor 86. D647 composed the draw, lifting the interim Run refusal.
namespace {
W99Parameters curved() {
    W99Parameters p{1.5, 0, 4, -12, -0.25, 0.35, 6, 0.25, 2, 1.5};
    p.cc1Curve = {{0.6, 0}, {0.9, 0.5}, {1.5, 1}};
    return p;
}
ProjectDocument owned() { // owned catalogs, two Links, one routed input of cars (w99_codec_tests' fixture)
    ProjectDocument d; const auto a = addLink(d, {{0, 0}, {200, 0}}, 1, 3.5), b = addLink(d, {{220, 0}, {400, 0}}, 1, 3.5);
    addConnector(d, {a, d.network.links[0].lanes[0].id, {}}, {b, d.network.links[1].lanes[0].id, {}});
    const auto route = putRoute(d, {"route", {a, d.network.connectors[0].id, b}}); changeRunSettings(d, 60, .1);
    auto catalog = resolveDemandCatalog(AuthoringDefinition{}, test::root() / "data");
    for (auto& x : catalog.behaviours) x.amberDeceleration.reset();
    putDemandCatalog(d, catalog);
    putInput(d, {"in", route, "car", 600, 0, 60});
    validateDocument(d);
    return d;
}
ProjectDocument with(const W99Parameters& p) {
    auto d = owned();
    DriverBehaviour w99{"urban-w99"}; w99.w99 = p;
    putBehaviour(d, w99, "Urban W99");
    validateDocument(d);
    return d;
}
std::size_t index(const Json& j) {
    const auto& list = j["definition"]["behaviours"];
    for (std::size_t i = 0; i < list.size(); ++i) if (list[i]["id"] == "urban-w99") return i;
    throw std::runtime_error("no w99 behaviour");
}
void refused(const Json& j, const std::string& code, const std::string& path) {
    test::throws([&] { parseDocument(j); }, code);
    try { parseDocument(j); } catch (const ValidationError& e) {
        CHECK(std::any_of(e.issues.begin(), e.issues.end(), [&](const auto& i) { return i.code == code && i.path == path; }));
    }
}
}

TEST(w99cc1codec, a_curve_round_trips_at_86_and_a_fixed_cc1_stays_at_85) { // BA233
    W99Parameters fixed{1.5, 1.3, 4, -12, -0.25, 0.35, 6, 0.25, 2, 1.5};
    const auto plain = documentJson(with(fixed));
    CHECK(plain["schemaVersion"] == 85); CHECK(plain.dump().find("cc1Curve") == std::string::npos);
    const auto d = with(curved()); const auto j = documentJson(d);
    CHECK(j["schemaVersion"] == 86); CHECK(kLatestSchema == 86);
    const auto& b = j["definition"]["behaviours"][index(j)];
    CHECK(!b.contains("cc1")); CHECK(b.at("cc1Curve").size() == 3);
    CHECK(b["cc1Curve"][1]["value"] == 0.9); CHECK(b["cc1Curve"][1]["share"] == 0.5);
    const auto back = parseDocument(j);
    CHECK(back.definition->behaviours == d.definition->behaviours);
    CHECK(documentJson(back).dump() == j.dump());
    // The external catalog reader takes the same form.
    Json item{{"id", "x"}, {"model", "w99"}};
    for (const auto& key : w99ParameterKeys()) if (std::string_view(key.name) != "cc1") item[key.name] = curved().*key.member;
    item["cc1Curve"] = b["cc1Curve"];
    CHECK(parseBehaviour(item).w99 == curved());
}

TEST(w99cc1codec, both_or_neither_and_each_bad_point_are_refused_at_their_path) { // BA233
    const auto j = documentJson(with(curved())); const auto i = index(j);
    const auto p = "behaviours[" + std::to_string(i) + "]";
    auto both = j; both["definition"]["behaviours"][i]["cc1"] = 0.9;
    refused(both, "INVALID_BEHAVIOUR_PARAMETER", p + ".cc1");
    auto neither = j; neither["definition"]["behaviours"][i].erase("cc1Curve");
    refused(neither, "INVALID_BEHAVIOUR_PARAMETER", p + ".cc1");
    const auto bad = [&](const std::function<void(Json&)>& change, const std::string& path, const std::string& code = "INVALID_BEHAVIOUR_PARAMETER") {
        auto k = j; change(k["definition"]["behaviours"][i]["cc1Curve"]); refused(k, code, p + path);
    };
    bad([](Json& c) { c = Json::array({c[0]}); }, ".cc1Curve");                                  // one point
    bad([](Json& c) { c[1]["value"] = 0.6; }, ".cc1Curve[1].value");                            // not increasing
    bad([](Json& c) { c[0]["value"] = -0.1; }, ".cc1Curve[0].value");                           // negative
    bad([](Json& c) { c[1]["share"] = 0; c[2]["share"] = 1; c[1]["share"] = -0.1; }, ".cc1Curve[1].share");
    bad([](Json& c) { c[0]["share"] = 0.1; }, ".cc1Curve[0].share");                            // not starting at 0
    bad([](Json& c) { c[2]["share"] = 0.9; }, ".cc1Curve[2].share");                            // not ending at 1
    bad([](Json& c) { c[2]["share"] = 1; c[1]["share"] = 0.6; c.push_back({{"value", 2}, {"share", 0.5}}); }, ".cc1Curve[3].share"); // falling
    bad([](Json& c) { c[1]["value"] = "0.9"; }, ".cc1Curve[1].value");                          // not a number
    bad([](Json& c) { c[1] = 7; }, ".cc1Curve[1]");                                              // not an object
    bad([](Json& c) { c = 7; }, ".cc1Curve");                                                    // not an array
    bad([](Json& c) { c[1]["extra"] = 1; }, ".cc1Curve[1].extra", "EDIT_UNSUPPORTED_FIELD");
    // A flat stretch (equal shares) holds no drivers and is allowed.
    auto flat = j; flat["definition"]["behaviours"][i]["cc1Curve"][1]["share"] = 0;
    CHECK(parseDocument(flat).definition->behaviours.size() == j["definition"]["behaviours"].size());
}

TEST(w99cc1codec, the_curve_is_refused_below_86_and_on_other_models) { // BA233
    const auto j = documentJson(with(curved())); const auto i = index(j);
    auto old = j; old["schemaVersion"] = 85;
    refused(old, "EDIT_UNSUPPORTED_FIELD", "behaviours[" + std::to_string(i) + "].cc1Curve");
    const std::size_t prototype = i == 0 ? 1 : 0;
    auto onPrototype = j; onPrototype["definition"]["behaviours"][prototype]["cc1Curve"] = j["definition"]["behaviours"][i]["cc1Curve"];
    refused(onPrototype, "EDIT_UNSUPPORTED_FIELD", "behaviours[" + std::to_string(prototype) + "].cc1Curve");
    Json w74{{"id", "x"}, {"model", "w74"}, {"cc1Curve", Json::array()}};
    test::throws([&] { parseBehaviour(w74); }, "EDIT_UNSUPPORTED_FIELD");
}

TEST(w99cc1codec, a_curved_behaviour_in_use_is_saved_and_runs) { // BA233, BA236 (D647 lifted D646's refusal)
    const auto data = test::root() / "data";
    auto d = with(curved());
    for (auto& t : d.definition->vehicleTypes) if (t.id == "car") t.behaviourId = "urban-w99";
    validateDocument(d);
    CHECK(parseDocument(documentJson(d)).definition->behaviours == d.definition->behaviours);
    const auto run = test::finish(createSimulation(compileDocument(d, data).scenario, 42));
    CHECK(run.completed > 0);
    for (const auto& r : runDiagnostics(d, data)) CHECK(r.code.rfind("UNSUPPORTED", 0) != 0);
}

TEST(w99cc1codec, the_validator_checks_the_curve_in_the_core) { // BA233, W99 §5a
    CHECK(w99ParameterIssues(curved(), "b").empty());
    auto p = curved(); p.cc1 = -5; CHECK(w99ParameterIssues(p, "b").empty()); // cc1 unused beside a curve
    p.cc1Curve[2].value = std::numeric_limits<double>::infinity();
    const auto issues = w99ParameterIssues(p, "b");
    CHECK(issues.size() == 1); CHECK(issues.front().path == "b.cc1Curve[2].value");
}
