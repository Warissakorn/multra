#include "test.hpp"
#include "../src/commands/behaviour_commands.hpp"
#include "../src/commands/catalog_commands.hpp"
#include "../src/commands/connector_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/commands/network_commands.hpp"
#include "../src/core/lateral.hpp"
#include "../src/core/validate.hpp"
#include "../src/core/w74.hpp"
#include "../src/project/demand_catalog.hpp"
#include "../src/project/run.hpp"
#include <cmath>
#include <fstream>
#include <functional>
#include <map>
using namespace trafficsim;
// M3.7 slice 1 (D187): rows BA45 and BA39 of docs/plans/DRIVING_BEHAVIOUR.md -- the lateral keys of
// docs/reference/LATERAL.md §9 are stored, validated and round-tripped, and §3's cross-sections and
// driving side are compiled only with the switch on (BA45); with every key absent nothing changes (BA39).
namespace {
const W74Parameters kP{.ax = 2, .bxAdd = 2, .bxMult = 1, .exAdd = 2, .exMult = 0.5, .cxAdd = 16,
    .cxMult = 4, .opdvAdd = 0.5, .opdvMult = 0.5, .dMax = 20, .bMaxAdd = 1, .bMaxMult = 1,
    .bMaxSpeedRoot = 3, .bNullAdd = 0.25, .bNullMult = 0.5, .bMinAdd = -1,
    .leaderAccelerationWeight = 0.5, .emergencyLeaderWeight = 0.25};
LateralParameters full() {
    LateralParameters p;
    p.lateralMinDistanceStanding = 0.5; p.lateralMinDistanceAt50 = 1.0; p.maxLateralSpeed = 1.2;
    return p;
}
struct Fixture { ProjectDocument d; std::string a, b, connector; };
// Owned catalogs from data/ (amber removed, so lateral alone decides the schema); a two-lane Link
// of 3.5 m and 3.0 m lanes, a Connector from its first lane onto a one-lane 4.0 m Link, and an input.
Fixture owned(DrivingSide side = DrivingSide::left) {
    Fixture f;
    auto& d = f.d;
    changeDrivingSide(d, side);
    f.a = addLink(d, {{0, 0}, {300, 0}}, 2, 3.5);
    changeLanes(d, f.a, {3.5, 3.0});
    f.b = addLink(d, {{340, 0}, {600, 0}}, 1, 4.0);
    const auto laneOf = [&](const std::string& link) {
        for (const auto& l : d.network.links) if (l.id == link) return l.lanes[0].id;
        throw std::runtime_error("no link");
    };
    f.connector = addConnector(d, {f.a, laneOf(f.a)}, {f.b, laneOf(f.b)});
    const auto route = putRoute(d, {"route", {f.a}}); changeRunSettings(d, 60, .1);
    auto catalog = resolveDemandCatalog(AuthoringDefinition{}, test::root() / "data");
    for (auto& b : catalog.behaviours) b.amberDeceleration.reset();
    putDemandCatalog(d, catalog);
    putInput(d, {"in", route, "car", 600, 0, 60});
    validateDocument(d);
    return f;
}
// The same, with every key on the prototype behaviour and an unused w74 behaviour carrying them.
Fixture withLateral(DrivingSide side = DrivingSide::left) {
    auto f = owned(side);
    f.d.definition->behaviours[0].lateral = full();
    DriverBehaviour w74{"urban-w74"}; w74.w74 = kP; w74.lateral = full();
    putBehaviour(f.d, w74, "Urban W74");
    validateDocument(f.d);
    return f;
}
std::size_t indexOf(const Json& j, const std::string& id) {
    const auto& list = j["definition"]["behaviours"];
    for (std::size_t i = 0; i < list.size(); ++i) if (list[i]["id"] == id) return i;
    throw std::runtime_error("no behaviour " + id);
}
bool hasIssue(const std::function<void()>& f, const std::string& code, const std::string& suffix) {
    try { f(); } catch (const ValidationError& e) {
        return std::any_of(e.issues.begin(), e.issues.end(), [&](const auto& i) {
            return i.code == code && i.path.size() >= suffix.size() &&
                   i.path.compare(i.path.size() - suffix.size(), suffix.size(), suffix) == 0;
        });
    }
    return false;
}
const Link& link(const Network& n, const std::string& id) {
    for (const auto& l : n.links) if (l.id == id) return l;
    throw std::runtime_error("no link");
}
std::map<std::string, SegmentCrossSection> byId(const Scenario& s) {
    std::map<std::string, SegmentCrossSection> result;
    for (std::size_t i = 0; i < s.segments.size(); ++i) result[s.segments[i].id] = s.segmentCrossSections.at(i);
    return result;
}
}

TEST(lateralcodec, schema33_round_trips_only_when_an_owned_behaviour_carries_a_key) { // BA45
    const auto plain = owned().d;
    const auto plainJson = documentJson(plain);
    CHECK(plainJson["schemaVersion"] < 33);
    CHECK(documentJson(parseDocument(Json::parse(plainJson.dump()))) == plainJson);
    const auto d = withLateral().d;
    const auto j = documentJson(d);
    CHECK(j["schemaVersion"] == 33);
    for (const auto* id : {"wiedemann-inspired-prototype", "urban-w74"}) {
        const auto& item = j["definition"]["behaviours"][indexOf(j, id)];
        for (const auto& key : lateralParameterKeys()) CHECK(item.contains(key.name));
    }
    const auto reopened = parseDocument(Json::parse(j.dump()));
    CHECK(reopened == d);
    for (const auto& b : reopened.definition->behaviours) CHECK(b.lateral == full());
    CHECK(documentJson(reopened) == j);
}

TEST(lateralcodec, a_key_below_schema_33_or_with_a_wrong_type_is_refused_with_its_path) { // BA45
    const auto j = documentJson(withLateral().d);
    const auto i = indexOf(j, "wiedemann-inspired-prototype");
    const auto at = "behaviours[" + std::to_string(i) + "]";
    auto older = j; older["schemaVersion"] = 32;
    CHECK(hasIssue([&] { parseDocument(older); }, "EDIT_UNSUPPORTED_FIELD", at + ".lateralMinDistanceStanding"));
    auto text = j; text["definition"]["behaviours"][i]["maxLateralSpeed"] = "1";
    CHECK(hasIssue([&] { parseDocument(text); }, "INVALID_BEHAVIOUR_PARAMETER", at + ".maxLateralSpeed"));
    auto unknown = j; unknown["definition"]["behaviours"][i]["lateralDrift"] = 1;
    test::throws([&] { parseDocument(unknown); }, "EDIT_UNSUPPORTED_FIELD");
}

TEST(lateralcodec, partial_sets_and_out_of_range_values_are_refused_on_used_and_unused_behaviours) { // BA45
    const auto data = test::root() / "data";
    using P = LateralParameters;
    const std::vector<std::optional<double> P::*> members{
        &P::lateralMinDistanceStanding, &P::lateralMinDistanceAt50, &P::maxLateralSpeed};
    const auto& keys = lateralParameterKeys();
    // Each one of the three missing: refused at the first missing key, on either behaviour.
    for (std::size_t missing = 0; missing < members.size(); ++missing)
        for (const std::size_t slot : {std::size_t{0}, std::size_t{1}}) {
            auto d = withLateral().d;
            d.definition->behaviours[slot].lateral.*members[missing] = std::nullopt;
            CHECK(hasIssue([&] { compileDocument(d, data); }, "INVALID_BEHAVIOUR_PARAMETER", std::string(".") + keys[missing].name));
        }
    // Two missing: the path names the first of them in table order.
    auto two = full(); two.lateralMinDistanceStanding.reset(); two.maxLateralSpeed.reset();
    const auto issues = lateralParameterIssues(two, "p");
    CHECK(issues.size() == 1); CHECK(issues.front().path == "p.lateralMinDistanceStanding");
    const std::vector<std::pair<std::size_t, std::vector<double>>> bad{
        {0, {-0.1, std::nan(""), INFINITY}}, {1, {-1, std::nan(""), -INFINITY}}, {2, {0, -1, 3.01, std::nan("")}}};
    for (const auto& [k, values] : bad)
        for (const double v : values)
            for (const std::size_t slot : {std::size_t{0}, std::size_t{1}}) {
                auto d = withLateral().d;
                d.definition->behaviours[slot].lateral.*members[k] = v;
                CHECK(hasIssue([&] { compileDocument(d, data); }, "INVALID_BEHAVIOUR_PARAMETER", std::string(".") + keys[k].name));
            }
    // The bounds are inside: zero distances and a 3 m/s rate.
    auto edges = withLateral().d;
    edges.definition->behaviours[0].lateral = {0.0, 0.0, 3.0};
    compileDocument(edges, data);
    CHECK(lateralParameterIssues(full(), "p").empty());
    CHECK(lateralParameterIssues(LateralParameters{}, "p").empty());
}

TEST(lateralcodec, external_catalog_entries_and_the_w74_branch_keep_the_keys) { // BA45
    Json prototype{{"id", "p"}, {"standstillDistance", 2},
        {"additiveSafetyDistance", 2}, {"multiplicativeSafetyDistance", 3}, {"followingTime", 1.5},
        {"speedThreshold", .2}, {"lateralMinDistanceStanding", 0.5}, {"lateralMinDistanceAt50", 1.0},
        {"maxLateralSpeed", 1.2}};
    CHECK(parseBehaviour(prototype, "b").lateral == full());
    const auto j = documentJson(withLateral().d);
    const auto parsed = parseBehaviour(j["definition"]["behaviours"][indexOf(j, "urban-w74")], "b");
    CHECK(parsed.w74.has_value()); CHECK(parsed.lateral == full());
}

TEST(lateralcodec, cross_sections_and_driving_side_compile_only_with_the_switch_on) { // BA45, BA39
    const auto data = test::root() / "data";
    // Off: nothing compiled, and the definition is the same as before the slice.
    const auto off = compileDocument(owned().d, data).scenario;
    CHECK(off.segmentCrossSections.empty()); CHECK(!off.drivingSide);
    CHECK(validateScenario(off).empty());
    for (const auto side : {DrivingSide::left, DrivingSide::right}) {
        const auto f = withLateral(side);
        const auto s = compileDocument(f.d, data).scenario;
        CHECK(s.drivingSide == side);
        CHECK(s.segmentCrossSections.size() == s.segments.size());
        CHECK(validateScenario(s).empty());
        const auto sections = byId(s);
        const auto& a = link(f.d.network, f.a);
        const auto& b = link(f.d.network, f.b);
        const double sign = side == DrivingSide::left ? 1 : -1;
        // Lane centres of the 3.5 m and 3.0 m lanes: 3.25 m apart, the first one to the left on a
        // left-hand network (whatever laneOffset changeLanes chose).
        const auto& first = sections.at(a.lanes[0].id);
        const auto& second = sections.at(a.lanes[1].id);
        CHECK(first.centre == laneCentreOffset(a, a.lanes[0].id, side));
        CHECK(second.centre == laneCentreOffset(a, a.lanes[1].id, side));
        CHECK(std::abs(first.centre - second.centre - sign * 3.25) < 1e-12);
        // ...and laneGeometry draws exactly there: the left normal of an x-axis link is +y.
        CHECK(laneGeometry(a, a.lanes[0].id, side).front().y == first.centre);
        CHECK(first.startWidth == 3.5); CHECK(first.endWidth == 3.5); CHECK(second.startWidth == 3.0);
        CHECK(first.carriageway == second.carriageway); CHECK(first.linkStationStart == 0);
        CHECK(sections.at(b.lanes[0].id).carriageway != first.carriageway);
        // The Connector path: its own carriageway, centred, 3.5 m widening to 4.0 m.
        std::size_t paths = 0;
        for (const auto& [id, c] : sections)
            if (id != a.lanes[0].id && id != a.lanes[1].id && id != b.lanes[0].id) {
                ++paths;
                CHECK(c.centre == 0); CHECK(c.linkStationStart == 0);
                CHECK(c.startWidth == 3.5); CHECK(c.endWidth == 4.0);
                CHECK(c.carriageway != first.carriageway);
                CHECK(c.carriageway != sections.at(b.lanes[0].id).carriageway);
            }
        CHECK(paths == 1);
        // Canonicalisation re-sorts segments; each cross-section moves with its segment.
        const auto state = createSimulation(s, 42);
        CHECK(byId(*state.scenario) == sections);
    }
    // A malformed compiled table is refused rather than read later.
    auto broken = compileDocument(withLateral().d, data).scenario;
    broken.segmentCrossSections.pop_back();
    CHECK(!validateScenario(broken).empty());
    auto sideless = compileDocument(withLateral().d, data).scenario; sideless.drivingSide.reset();
    CHECK(!validateScenario(sideless).empty());
}

TEST(lateralcodec, ba39_keys_absent_leave_the_four_leg_scenario_and_run_unchanged) { // BA39
    const auto data = test::root() / "data";
    std::ifstream file(test::root() / "data/projects/four-leg-signalised.traffic.json");
    Json project; file >> project;
    const auto scenario = compileDocument(parseDocument(project), data).scenario;
    CHECK(scenario.segmentCrossSections.empty()); CHECK(!scenario.drivingSide);
    for (const auto& b : scenario.behaviours) CHECK(!hasLateral(b.lateral));
    // Slice 1 has no runtime reader: keys present still replay the key-absent run tick by tick.
    // Slice 2 (spans and predicates) replaces this half with BA40-BA42.
    auto keyed = scenario;
    for (auto& b : keyed.behaviours) b.lateral = full();
    auto a = createSimulation(scenario, 42), b = createSimulation(keyed, 42);
    while (a.tick < totalTicks(scenario)) {
        a = stepSimulation(std::move(a)); b = stepSimulation(std::move(b));
        CHECK(a.vehicles == b.vehicles); CHECK(a.events == b.events);
    }
    CHECK(a.nextVehicleId > 1);
}
