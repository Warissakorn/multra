#include "test.hpp"
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
// M3.3.3b (D639): row BA228 of docs/plans/DRIVING_BEHAVIOUR.md -- the schema-85 codec of a `w99`
// behaviour (docs/reference/W99.md §5, §9) and the interim Run refusal of one in use.
namespace {
const W99Parameters kP{1.5, 1.3, 4.0, -12.0, -0.25, 0.35, 6.0, 0.25, 2.0, 1.5}; // W99 §5's fixture values
// Owned catalogs from data/, two Links joined by a Connector, one routed input of cars.
ProjectDocument owned() {
    ProjectDocument d; const auto a = addLink(d, {{0, 0}, {200, 0}}, 1, 3.5), b = addLink(d, {{220, 0}, {400, 0}}, 1, 3.5);
    addConnector(d, {a, d.network.links[0].lanes[0].id, {}}, {b, d.network.links[1].lanes[0].id, {}});
    const auto route = putRoute(d, {"route", {a, d.network.connectors[0].id, b}}); changeRunSettings(d, 60, .1);
    auto catalog = resolveDemandCatalog(AuthoringDefinition{}, test::root() / "data");
    for (auto& b : catalog.behaviours) b.amberDeceleration.reset(); // keep the plain file at its old schema
    putDemandCatalog(d, catalog);
    putInput(d, {"in", route, "car", 600, 0, 60});
    validateDocument(d);
    return d;
}
ProjectDocument withW99() { // the same project with an unused w99 behaviour "urban-w99"
    auto d = owned();
    DriverBehaviour w99{"urban-w99"}; w99.w99 = kP; w99.maxDecelerationCooperativeBraking = 3;
    putBehaviour(d, w99, "Urban W99");
    validateDocument(d);
    return d;
}
std::size_t w99Index(const Json& j) {
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

TEST(w99codec, schema85_round_trips_only_when_a_w99_behaviour_is_owned) { // BA228
    const auto plain = documentJson(owned());
    CHECK(plain["schemaVersion"] < 85);
    const auto d = withW99(); const auto j = documentJson(d);
    CHECK(j["schemaVersion"] == 85); CHECK(kLatestSchema == 85);
    const auto& b = j["definition"]["behaviours"][w99Index(j)];
    CHECK(b["model"] == "w99"); CHECK(b["name"] == "Urban W99"); CHECK(b["maxDecelerationCooperativeBraking"] == 3);
    for (const auto& key : w99ParameterKeys()) CHECK(b.at(key.name) == kP.*key.member);
    for (const char* key : {"standstillDistance", "followingTime", "ax", "bxAdd"}) CHECK(!b.contains(key));
    for (const auto& other : j["definition"]["behaviours"]) if (other["id"] != "urban-w99") CHECK(other["model"] == "prototype");
    const auto back = parseDocument(j);
    CHECK(back.definition->behaviours == d.definition->behaviours);
    CHECK(documentJson(back).dump() == j.dump());
    auto removed = d; std::erase_if(removed.definition->behaviours, [](const auto& x) { return x.w99.has_value(); });
    removed.definition->behaviourNames.clear();
    CHECK(documentJson(removed).dump() == plain.dump()); // without it, exactly the old bytes
}

TEST(w99codec, missing_non_numeric_and_out_of_range_keys_are_refused_with_their_path) { // BA228
    const auto j = documentJson(withW99()); const auto i = w99Index(j);
    const auto path = "behaviours[" + std::to_string(i) + "]";
    for (const auto& key : w99ParameterKeys()) {
        auto missing = j; missing["definition"]["behaviours"][i].erase(key.name);
        refused(missing, "INVALID_BEHAVIOUR_PARAMETER", path + "." + key.name);
        auto text = j; text["definition"]["behaviours"][i][key.name] = "1";
        refused(text, "INVALID_BEHAVIOUR_PARAMETER", path + "." + key.name);
    }
    // One value just outside each range; the behaviour is unused, and still refused.
    for (const auto& [key, value] : std::vector<std::pair<std::string, double>>{
             {"cc0", 0}, {"cc1", -0.1}, {"cc2", -0.1}, {"cc3", 0.1}, {"cc4", 0.1}, {"cc5", -0.1},
             {"cc6", -0.1}, {"cc7", 0}, {"cc8", 0}, {"cc9", -0.1}}) {
        auto bad = j; bad["definition"]["behaviours"][i][key] = value;
        refused(bad, "INVALID_BEHAVIOUR_PARAMETER", path + "." + key);
    }
    auto edge = j; auto& e = edge["definition"]["behaviours"][i]; // the closed edges are inside
    for (const char* key : {"cc1", "cc2", "cc3", "cc4", "cc5", "cc6", "cc9"}) e[key] = 0;
    CHECK(parseDocument(edge).definition->behaviours.size() == j["definition"]["behaviours"].size());
}

TEST(w99codec, each_models_keys_are_refused_on_the_others) { // BA228, W99 §5
    const auto j = documentJson(withW99()); const auto i = w99Index(j);
    const auto path = "behaviours[" + std::to_string(i) + "]";
    for (const char* key : {"followingTime", "standstillDistance", "discretionaryLaneChangeThreshold", "ax", "bNullAdd"}) {
        auto extra = j; extra["definition"]["behaviours"][i][key] = 1;
        refused(extra, "EDIT_UNSUPPORTED_FIELD", path + "." + key);
    }
    const std::size_t prototype = i == 0 ? 1 : 0;
    auto onPrototype = j; onPrototype["definition"]["behaviours"][prototype]["cc0"] = 1.5;
    refused(onPrototype, "EDIT_UNSUPPORTED_FIELD", "behaviours[" + std::to_string(prototype) + "].cc0");
    // A W74 entry (external, any version) refuses cc keys too: no key is another model's alias.
    Json w74{{"id", "x"}, {"model", "w74"}};
    for (const auto* key : {"ax", "bxAdd", "bxMult", "exAdd", "exMult", "cxAdd", "cxMult", "opdvAdd", "opdvMult", "dMax",
                            "bMaxAdd", "bMaxMult", "bMaxSpeedRoot", "bNullAdd", "bNullMult", "bMinAdd",
                            "leaderAccelerationWeight", "emergencyLeaderWeight"}) w74[key] = 1;
    w74["cc7"] = 0.25; test::throws([&] { parseBehaviour(w74); }, "EDIT_UNSUPPORTED_FIELD");
    auto unknown = j; unknown["definition"]["behaviours"][i]["model"] = "w100";
    refused(unknown, "UNSUPPORTED_BEHAVIOUR_MODEL", path + ".model");
}

TEST(w99codec, w99_below_schema_85_and_a_future_schema_are_refused) { // BA228
    const auto j = documentJson(withW99()); const auto i = w99Index(j);
    for (const int version : {25, 84}) {
        auto old = j; old["schemaVersion"] = version;
        refused(old, "UNSUPPORTED_BEHAVIOUR_MODEL", "behaviours[" + std::to_string(i) + "].model");
    }
    auto future = j; future["schemaVersion"] = kLatestSchema + 1;
    test::throws([&] { parseDocument(future); }, "EDIT_VERSION");
}

TEST(w99codec, external_catalog_entries_read_the_w99_model) { // W99 §9
    Json item{{"id", "x"}, {"model", "w99"}};
    for (const auto& key : w99ParameterKeys()) item[key.name] = kP.*key.member;
    const auto w99 = parseBehaviour(item);
    CHECK(w99.w99 == kP); CHECK(!w99.w74);
    item.erase("cc4"); test::throws([&] { parseBehaviour(item); }, "INVALID_BEHAVIOUR_PARAMETER");
}

TEST(w99codec, a_w99_behaviour_in_use_is_refused_on_run_and_an_unused_one_changes_nothing) { // BA228
    const auto data = test::root() / "data";
    const auto plain = compileDocument(owned(), data).scenario, unused = compileDocument(withW99(), data).scenario;
    CHECK(unused.behaviours.size() == plain.behaviours.size() + 1);
    const auto a = runSimulation(plain, 42), b = runSimulation(unused, 42);
    CHECK(!a.vehicles.empty()); CHECK(a.vehicles == b.vehicles); CHECK(a.time == b.time); // no trait yet (BA229)
    auto byType = withW99();
    for (auto& t : byType.definition->vehicleTypes) if (t.id == "car") t.behaviourId = "urban-w99";
    auto byRoad = withW99();
    putLinkBehaviourType(byRoad, {"urban", "Urban", "urban-w99", {}});
    assignBehaviourType(byRoad, byRoad.network.links[0].id, "urban");
    for (auto* d : {&byType, &byRoad}) {
        validateDocument(*d); // authoring accepts it
        CHECK(parseDocument(documentJson(*d)).definition->behaviours == d->definition->behaviours);
        // Compiling for a run asserts the scenario (compileScenario), so Run is refused there.
        test::throws([&] { (void)compileDocument(*d, data); }, "UNSUPPORTED_BEHAVIOUR_MODEL_RUN");
        const auto rows = runDiagnostics(*d, data);
        CHECK(std::count_if(rows.begin(), rows.end(), [](const auto& r) { return r.code == "UNSUPPORTED_BEHAVIOUR_MODEL_RUN"; }) == 1);
    }
}
