#include "test.hpp"
#include "../src/core/conflicts.hpp"
#include "../src/project/document.hpp"
#include "../src/project/json.hpp"
#include "../src/core/validate.hpp"
#include <fstream>

using namespace multra;
// JOIN_ANTICIPATION §2, §5 (D254/D255): off by default, a vehicle within 50 m of a join follows, as a
// second law, the other stream's vehicle that reaches the join first. Each engine row first runs
// with the switch off and asserts today's tick.
namespace {
bool clamped(const SimState& s, std::uint64_t id) {
    return std::any_of(s.events.begin(), s.events.end(), [&](const auto& e) {
        const auto* c = std::get_if<SafetyClampEvent>(&e); return c && c->vehicleId == id; });
}
const Vehicle& vehicleOf(const SimState& s, std::uint64_t id) {
    return *std::find_if(s.vehicles.begin(), s.vehicles.end(), [&](const auto& v) { return v.id == id; });
}
// A major and a minor joining a shared segment; the minor's line is 1 m short of the join (D50).
Scenario join(bool on) {
    auto s = test::straight();
    s.segments = {{"major", 150, {"shared"}}, {"minor", 100, {"shared"}}, {"shared", 150, {}}};
    s.routes = {{"majorRoute", {"major", "shared"}}, {"minorRoute", {"minor", "shared"}}};
    s.priorityRules = {{"merge", "minor", 99, "major", 150, 3, 10}};
    s.clampRules.joinAnticipation = on;
    return s;
}
SimState placed(bool on, const std::vector<test::Placement>& vehicles) { return test::withVehicles(join(on), vehicles); }
int clampsOf(SimState s, std::uint64_t id, int ticks) {
    int count = 0;
    for (int t = 0; t < ticks; ++t) { s = stepSimulation(std::move(s)); count += clamped(s, id); }
    return count;
}
// One tick with the switch on is today's tick, vehicles and events alike.
bool sameTick(const std::vector<test::Placement>& vehicles) {
    const auto off = stepSimulation(placed(false, vehicles)), on = stepSimulation(placed(true, vehicles));
    return off.vehicles == on.vehicles && off.events == on.events;
}
}
TEST(joinanticipation, ba168_a_major_eases_for_a_minor_that_has_passed_its_line) { // BA168
    // The major is 15 m from the join at 12 m/s; the minor is 0.5 m past its line at 1 m/s.
    const std::vector<test::Placement> start{{1, "majorRoute", 135, 12}, {2, "minorRoute", 99.5, 1}};
    CHECK(clampsOf(placed(false, start), 1, 50) > 0); // forcing: today it meets the minor at the join and clamps
    CHECK(clampsOf(placed(true, start), 1, 50) == 0);
    // It eases from the first tick, before the minor's front is on the shared segment.
    CHECK(vehicleOf(stepSimulation(placed(true, start)), 1).acceleration < vehicleOf(stepSimulation(placed(false, start)), 1).acceleration);
}
TEST(joinanticipation, ba169_a_minor_held_at_its_line_is_no_leader) { // BA169
    // Short of its line and moving, so held by its rule (the major is inside the 3 s gap time).
    const std::vector<test::Placement> start{{1, "majorRoute", 135, 12}, {2, "minorRoute", 98.5, 1}};
    CHECK(!committed(1, 0.5, test::straight().vehicleTypes.front()));
    CHECK(vehicleOf(stepSimulation(placed(false, start)), 2).distance <= 99 + 1e-9); // forcing: its rule holds it
    CHECK(sameTick(start));
    // Standing past its line is no leader either: it is not moving.
    CHECK(sameTick({{1, "majorRoute", 135, 12}, {2, "minorRoute", 99.5, 0}}));
    // Nor is a vehicle whose front is not yet on the join's own predecessor ("near"), 15 m from the join.
    const auto upstream = [](bool on) {
        auto s = join(on);
        s.segments = {{"far", 80, {"near"}}, {"near", 10, {"shared"}}, {"minor", 100, {"shared"}}, {"shared", 150, {}}};
        s.routes = {{"majorRoute", {"far", "near", "shared"}}, {"minorRoute", {"minor", "shared"}}};
        s.priorityRules = {{"merge", "minor", 99, "near", 10, 3, 10}};
        return stepSimulation(test::withVehicles(s, {{1, "majorRoute", 75, 3}, {2, "minorRoute", 70, 10}}));
    };
    CHECK(upstream(true).vehicles == upstream(false).vehicles);
}
TEST(joinanticipation, ba170_the_stricter_law_is_kept) { // BA170
    // The virtual leader (10 m, 12 m/s) is nearer than the real one (20.5 m, 2 m/s) but faster. Replacing
    // the real leader would let the major accelerate into it; the stricter law, the real one, is kept.
    const std::vector<test::Placement> start{{1, "majorRoute", 135, 12}, {3, "majorRoute", 160, 2}, {2, "minorRoute", 99.5, 12}};
    const auto off = stepSimulation(placed(false, start)), on = stepSimulation(placed(true, start));
    const auto alone = stepSimulation(placed(false, {{1, "majorRoute", 135, 12}}));
    CHECK(vehicleOf(off, 1).acceleration < vehicleOf(alone, 1).acceleration); // forcing: the real leader binds
    CHECK(vehicleOf(on, 1).acceleration == vehicleOf(off, 1).acceleration);
    CHECK(on.vehicles == off.vehicles);
}
TEST(joinanticipation, ba171_side_by_side_and_a_tie_lead_nobody) { // BA171
    // The minor's rear projects 2 m past the major's front: alongside, the rules' business.
    CHECK(sameTick({{1, "majorRoute", 147, 12}, {2, "minorRoute", 99.5, 1}}));
    // Equal distances project a gap of minus a length, so neither leads, whichever id is lower.
    CHECK(sameTick({{1, "majorRoute", 149.5, 3}, {2, "minorRoute", 99.5, 3}}));
    CHECK(sameTick({{2, "majorRoute", 149.5, 3}, {1, "minorRoute", 99.5, 3}}));
}
namespace {
ProjectDocument fourLeg() {
    std::ifstream file(test::root() / "data/projects/four-leg-signalised.traffic.json"); Json j; file >> j;
    return parseDocument(j);
}
bool refused(const Json& doc, const std::string& code, const std::string& path) {
    try { parseDocument(doc); } catch (const ValidationError& e) {
        return std::any_of(e.issues.begin(), e.issues.end(), [&](const auto& i) { return i.code == code && i.path == path; });
    }
    return false;
}
}
TEST(joinanticipation, ba172_off_by_default_and_round_trips_at_schema_46) { // BA172
    CHECK(!ClampRules{}.joinAnticipation);
    // Off, a run with vehicles approaching the join keeps today's bytes on every tick.
    auto off = placed(false, {{1, "majorRoute", 135, 12}, {2, "minorRoute", 99.5, 1}});
    auto bare = off; bare.scenario = std::make_shared<const Scenario>([&] { auto s = *off.scenario; s.clampRules = {}; return s; }());
    for (int t = 0; t < 100; ++t) { off = stepSimulation(std::move(off)); bare = stepSimulation(std::move(bare)); CHECK(off.events == bare.events); }
    auto d = fourLeg();
    const auto owned = documentJson(d);
    CHECK(owned["schemaVersion"] < 45);
    d.definition->clampRules.joinAnticipation = true;
    const auto j = documentJson(d);
    CHECK(j["schemaVersion"] == 46);
    CHECK(j["definition"]["clampRules"] == Json{{"joinAnticipation", true}});
    CHECK(parseDocument(Json::parse(j.dump())) == d);
    auto older = j; older["schemaVersion"] = 45;
    CHECK(refused(older, "EDIT_UNSUPPORTED_FIELD", "definition.clampRules.joinAnticipation"));
    auto notBool = j; notBool["definition"]["clampRules"]["joinAnticipation"] = 1;
    CHECK(refused(notBool, "INVALID_CLAMP_RULE", "definition.clampRules.joinAnticipation"));
    // Written false it reads as off, and saving drops it again.
    auto explicitOff = j; explicitOff["definition"]["clampRules"]["joinAnticipation"] = false;
    CHECK(!parseDocument(explicitOff).definition->clampRules.joinAnticipation);
    d.definition->clampRules = {};
    CHECK(documentJson(d) == owned);
}
