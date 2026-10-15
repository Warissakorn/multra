#include "test.hpp"
#include "../src/commands/behaviour_commands.hpp"
#include "../src/commands/catalog_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/core/perception.hpp"
#include "../src/core/w74.hpp"
#include "../src/project/demand_catalog.hpp"
#include "../src/project/run.hpp"
#include <cmath>
#include <fstream>
#include <functional>
using namespace trafficsim;
// M3.5 slice 1 (D181): rows BA37 and BA30 of docs/plans/DRIVING_BEHAVIOUR.md -- the perception
// keys of docs/reference/PERCEPTION.md §6 are stored, validated and round-tripped, and read by no
// motion yet, so a run with them is the run without them.
namespace {
const W74Parameters kP{.ax = 2, .bxAdd = 2, .bxMult = 1, .exAdd = 2, .exMult = 0.5, .cxAdd = 16,
    .cxMult = 4, .opdvAdd = 0.5, .opdvMult = 0.5, .dMax = 20, .bMaxAdd = 1, .bMaxMult = 1,
    .bMaxSpeedRoot = 3, .bNullAdd = 0.25, .bNullMult = 0.5, .bMinAdd = -1,
    .leaderAccelerationWeight = 0.5, .emergencyLeaderWeight = 0.25};
PerceptionParameters full() {
    PerceptionParameters p;
    p.lookAheadDistance = 120; p.observedVehiclesAhead = 2; p.lookBackDistance = 80;
    p.rearTimeToCollision = 1.5; p.reactionTime = 0.8;
    return p;
}
// Owned catalogs from data/ (amber removed, so the schema is decided by perception alone), one
// Link and one input of cars.
ProjectDocument owned() {
    ProjectDocument d; const auto link = addLink(d, {{0, 0}, {300, 0}}, 1, 3.5);
    const auto route = putRoute(d, {"route", {link}}); changeRunSettings(d, 60, .1);
    auto catalog = resolveDemandCatalog(AuthoringDefinition{}, test::root() / "data");
    for (auto& b : catalog.behaviours) b.amberDeceleration.reset();
    putDemandCatalog(d, catalog);
    putInput(d, {"in", route, "car", 600, 0, 60});
    validateDocument(d);
    return d;
}
// The same, with every key on the prototype behaviour and an unused w74 behaviour carrying them.
ProjectDocument withPerception() {
    auto d = owned();
    d.definition->behaviours[0].perception = full();
    DriverBehaviour w74{"urban-w74"}; w74.w74 = kP; w74.perception = full();
    putBehaviour(d, w74, "Urban W74");
    validateDocument(d);
    return d;
}
std::size_t indexOf(const Json& j, const std::string& id) {
    const auto& list = j["definition"]["behaviours"];
    for (std::size_t i = 0; i < list.size(); ++i) if (list[i]["id"] == id) return i;
    throw std::runtime_error("no behaviour " + id);
}
bool issueEndsWith(const std::function<void()>& f, const std::string& code, const std::string& suffix) {
    try { f(); } catch (const ValidationError& e) {
        return std::any_of(e.issues.begin(), e.issues.end(), [&](const auto& i) {
            return i.code == code && i.path.size() >= suffix.size() &&
                   i.path.compare(i.path.size() - suffix.size(), suffix.size(), suffix) == 0;
        });
    }
    return false;
}
bool hasIssue(const std::function<void()>& f, const std::string& code, const std::string& path) {
    try { f(); } catch (const ValidationError& e) {
        return std::any_of(e.issues.begin(), e.issues.end(), [&](const auto& i) { return i.code == code && i.path == path; });
    }
    return false;
}
}

TEST(perceptioncodec, schema32_round_trips_only_when_an_owned_behaviour_carries_a_key) { // BA37
    // Without any key the file keeps its earlier schema and bytes.
    const auto plain = owned();
    const auto plainJson = documentJson(plain);
    CHECK(plainJson["schemaVersion"] < 32);
    CHECK(documentJson(parseDocument(Json::parse(plainJson.dump()))) == plainJson);
    // With keys on either model: schema 32, every key written, the integer one as an integer.
    const auto d = withPerception();
    const auto j = documentJson(d);
    CHECK(j["schemaVersion"] == 32);
    for (const auto* id : {"wiedemann-inspired-prototype", "urban-w74"}) {
        const auto& item = j["definition"]["behaviours"][indexOf(j, id)];
        for (const auto& key : perceptionParameterKeys()) CHECK(item.contains(key.name));
        CHECK(item["observedVehiclesAhead"].is_number_integer());
        CHECK(item["observedVehiclesAhead"] == 2);
    }
    const auto reopened = parseDocument(Json::parse(j.dump()));
    CHECK(reopened == d);
    for (const auto& b : reopened.definition->behaviours) CHECK(b.perception == full());
    CHECK(documentJson(reopened) == j);
}

TEST(perceptioncodec, a_key_below_schema_32_or_with_a_wrong_type_is_refused_with_its_path) { // BA37
    const auto j = documentJson(withPerception());
    const auto i = indexOf(j, "wiedemann-inspired-prototype");
    const auto at = "behaviours[" + std::to_string(i) + "]";
    auto older = j; older["schemaVersion"] = 31;
    CHECK(hasIssue([&] { parseDocument(older); }, "EDIT_UNSUPPORTED_FIELD", at + ".lookAheadDistance"));
    // A float where the key is an integer, and a string, are refused at parse with the path.
    auto fraction = j; fraction["definition"]["behaviours"][i]["observedVehiclesAhead"] = 2.0;
    CHECK(fraction["definition"]["behaviours"][i]["observedVehiclesAhead"].is_number_float()); // the forcing
    CHECK(hasIssue([&] { parseDocument(fraction); }, "INVALID_BEHAVIOUR_PARAMETER", "behaviours[" + std::to_string(i) + "].observedVehiclesAhead"));
    auto text = j; text["definition"]["behaviours"][i]["reactionTime"] = "1";
    test::throws([&] { parseDocument(text); }, "INVALID_BEHAVIOUR_PARAMETER");
    // An unknown key next to them is still refused (the known-key list grew, it did not open).
    auto unknown = j; unknown["definition"]["behaviours"][i]["lookSideways"] = 1;
    test::throws([&] { parseDocument(unknown); }, "EDIT_UNSUPPORTED_FIELD");
}

TEST(perceptioncodec, out_of_range_values_are_refused_on_used_and_unused_behaviours) { // BA37
    const auto data = test::root() / "data";
    const std::vector<std::pair<std::optional<double> PerceptionParameters::*, std::vector<double>>> bad{
        {&PerceptionParameters::lookAheadDistance, {0, -1, std::nan(""), INFINITY}},
        {&PerceptionParameters::observedVehiclesAhead, {0, 9, 1.5, std::nan("")}},
        {&PerceptionParameters::lookBackDistance, {0, -5, INFINITY}},
        {&PerceptionParameters::rearTimeToCollision, {0, -0.1, std::nan("")}},
        {&PerceptionParameters::reactionTime, {-0.1, 2.01, std::nan("")}}};
    const auto& keys = perceptionParameterKeys();
    for (const auto& [member, values] : bad) {
        const auto key = std::find_if(keys.begin(), keys.end(), [&](const auto& k) { return k.member == member; });
        CHECK(key != keys.end());
        for (const double v : values) {
            // On the prototype behaviour every vehicle uses, and on the unused w74 one.
            // The path names the key; the behaviour's index is the validator's own order.
            for (const std::size_t slot : {std::size_t{0}, std::size_t{1}}) {
                auto d = withPerception();
                d.definition->behaviours[slot].perception.*member = v;
                CHECK(issueEndsWith([&] { compileDocument(d, data); }, "INVALID_BEHAVIOUR_PARAMETER", std::string(".") + key->name));
            }
        }
    }
    // The bounds themselves are inside: 1 and 8 observed vehicles, a 0 s and a 2 s reaction time.
    auto edges = withPerception();
    edges.definition->behaviours[0].perception.observedVehiclesAhead = 1;
    edges.definition->behaviours[1].perception.observedVehiclesAhead = 8;
    edges.definition->behaviours[0].perception.reactionTime = 0;
    edges.definition->behaviours[1].perception.reactionTime = 2;
    compileDocument(edges, data);
    CHECK(perceptionParameterIssues(full(), "p").empty());
    CHECK(perceptionParameterIssues(PerceptionParameters{}, "p").empty());
}

TEST(perceptioncodec, external_catalog_entries_and_the_w74_branch_keep_the_keys) { // BA37, §6
    Json item{{"id", "x"}, {"standstillDistance", 2}, {"additiveSafetyDistance", 2},
        {"multiplicativeSafetyDistance", 3}, {"followingTime", 1.5}, {"speedThreshold", .2},
        {"lookAheadDistance", 120}, {"observedVehiclesAhead", 2}, {"lookBackDistance", 80},
        {"rearTimeToCollision", 1.5}, {"reactionTime", 0.8}};
    CHECK(parseBehaviour(item).perception == full());
    Json w74{{"id", "w"}, {"model", "w74"}, {"lookBackDistance", 80}};
    for (const auto& key : w74ParameterKeys()) w74[key.name] = kP.*key.member;
    const auto parsed = parseBehaviour(w74);
    CHECK(parsed.w74 == kP); CHECK(*parsed.perception.lookBackDistance == 80);
    CHECK(!parsed.perception.lookAheadDistance);
    // What the Driving behaviours dialog's w74 path does: a fresh behaviour plus the kept keys.
    auto edited = DriverBehaviour{parsed.id}; edited.perception = parsed.perception; edited.w74 = kP;
    CHECK(edited == parsed);
}

TEST(perceptioncodec, ba30_the_keys_change_no_run_yet) { // BA30
    const auto data = test::root() / "data";
    std::ifstream file(test::root() / "data/projects/four-leg-signalised.traffic.json");
    Json project; file >> project;
    const auto plain = compileDocument(parseDocument(project), data).scenario;
    auto keyed = plain;
    for (auto& b : keyed.behaviours) b.perception = full();
    CHECK(keyed.behaviours != plain.behaviours); // the forcing: the keys really are there
    const auto a = runSimulation(plain, 42), b = runSimulation(keyed, 42);
    CHECK(!a.vehicles.empty() || a.nextVehicleId > 1);
    CHECK(a.vehicles == b.vehicles); CHECK(a.events == b.events); CHECK(a.time == b.time);
    CHECK(a.nextVehicleId == b.nextVehicleId);
}
