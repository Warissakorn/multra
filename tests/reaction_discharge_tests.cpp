#include "test.hpp"
#include "../tools/reaction_discharge.hpp"
#include "../tools/reaction_discharge_summary.hpp"
using namespace trafficsim;
namespace {
DriverBehaviour model(bool w74) {
    if (!w74) return {"driver", 2, 2, 3, 1.5, .2};
    auto b = w74fixture::behaviour(test::root() / "data/driver-behaviour/presets/w74-sumo-defaults.json");
    b.id = "driver"; return b;
}
Scenario road(bool w74, std::optional<double> reaction) {
    Scenario s; s.duration = 30; s.timeStep = .1;
    s.segments = {{"lane", 300, {}}}; s.routes = {{"route", {"lane"}}};
    auto b = model(w74); b.perception.reactionTime = reaction; s.behaviours = {b};
    s.vehicleTypes = {{"car", 4, 2, {10, 10}, 2, 3, 6, "driver"}};
    s.inputs = {{"input", "route", "car", 1800, 0, 30}};
    s.signalPrograms = {{"p", 0, {{10, SignalColor::red}, {10, SignalColor::green}, {10, SignalColor::red}}}};
    s.signalHeads = {{"head", "lane", 100, "p"}};
    return s;
}
}
TEST(reactiondischarge, variants_preserve_every_non_reaction_setting_within_each_model) {
    const auto data = test::root() / "data";
    const auto base = w74fixture::base(data / "projects/four-leg-signalised.traffic.json", data);
    const auto w74 = model(true);
    for (const auto* selected : {static_cast<const DriverBehaviour*>(nullptr), &w74}) {
        const auto original = reactiondischarge::variant(base, selected, {});
        const auto plain = documentJson(original);
        for (const double tau : {0., .5, 1.}) {
            const auto d = reactiondischarge::variant(base, selected, tau);
            const auto snapshot = compileDocument(d, data);
            CHECK(snapshot.scenario.timeStep == .1);
            for (const auto& b : snapshot.scenario.behaviours) CHECK(b.perception.reactionTime == tau);
            CHECK(parseDocument(documentJson(d)) == d);
            auto restored = documentJson(d);
            CHECK(restored["schemaVersion"] == 32);
            for (auto& b : restored["definition"]["behaviours"]) {
                CHECK(b["reactionTime"] == tau); b.erase("reactionTime");
            }
            restored["schemaVersion"] = plain["schemaVersion"];
            CHECK(restored == plain);
        }
    }
}
TEST(reactiondischarge, zero_is_identical_to_absent_and_live_fifo_replay_is_exact_for_both_models) {
    for (const bool w74 : {false, true}) {
        const auto absent = road(w74, {});
        const auto zero = reactiondischarge::measure(road(w74, 0), {.5, 1, 10}, 42, &absent);
        CHECK(zero["generated"] > 0); CHECK(zero["zeroAbsentTicksChecked"] == 300);
        CHECK(zero["copiedReplayTicks"] == 32); CHECK(!zero["copiedReplayWithHistory"].get<bool>());
        const auto delayed = reactiondischarge::measure(road(w74, .5), {.5, 1, 10}, 42);
        CHECK(delayed["generated"] > 0); CHECK(delayed["copiedReplayWithHistory"].get<bool>());
        CHECK(delayed["copiedReplayTicks"] == 32);
        CHECK(delayed == reactiondischarge::measure(road(w74, .5), {.5, 1, 10}, 42));
        CHECK(delayed["completed"].get<std::size_t>() + delayed["active"].get<std::size_t>() +
              delayed["pending"].get<std::size_t>() == delayed["generated"].get<std::size_t>());
        CHECK(delayed["discharge"]["label"].get<std::string>().find("not yet validated") != std::string::npos);
        CHECK(!delayed["discharge"]["cycles"].empty());
        for (const auto& c : delayed["discharge"]["cycles"]) {
            if (c["unavailableReason"] != "") CHECK(c["meanHeadwaySeconds"].is_null());
            if (c["startupUnavailableReason"] != "") CHECK(c["startupLostTimeSeconds"].is_null());
        }
    }
}
TEST(reactiondischarge, a_false_zero_identity_claim_is_detected) {
    const auto zero = road(false, 0);
    auto changed = road(false, {}); changed.vehicleTypes[0].maxAcceleration = .1;
    CHECK(changed.vehicleTypes[0].maxAcceleration != zero.vehicleTypes[0].maxAcceleration);
    test::throws([&] { reactiondischarge::measure(zero, {.5, 1, 10}, 42, &changed); }, "trajectory mismatch");
}
TEST(reactiondischarge, comparison_includes_fifo_contents_and_preserves_the_original_copy) {
    auto state = placeVehicle(createSimulation(road(false, 1), 42), {"route", "car", 50, 0});
    auto copy = state;
    CHECK(!copy.vehicles[0].reactionHistory.empty());
    copy.vehicles[0].reactionHistory[0].acceleration = 1;
    CHECK(state.vehicles[0].reactionHistory[0].acceleration == 0);
    CHECK(!reactiondischarge::sameState(state, copy));
}

TEST(reactiondischarge, summaries_weight_seeds_equally_and_retain_nulls_and_reasons) {
    reactiondischarge::Summary summary;
    const auto cycle = [](Json h, Json startup, const std::string& reason) {
        return Json{{"headId", "h"}, {"laneId", "lane"}, {"meanHeadwaySeconds", h},
            {"startupLostTimeSeconds", startup}, {"unavailableReason", reason},
            {"startupUnavailableReason", reason}};
    };
    const auto run = [](int seed, const Json& cycles) {
        return Json{{"variant", "prototype-0"}, {"seed", seed}, {"completed", 10},
            {"active", 1}, {"pending", 2}, {"safetyClamps", 3}, {"discharge", {{"cycles", cycles}}}};
    };
    summary.add(run(42, Json::array({cycle(2, -1, ""), cycle(2, -1, "")})));
    summary.add(run(43, Json::array({cycle(4, 3, "")})));
    summary.add(run(44, Json::array({cycle(nullptr, nullptr, "queue_not_sustained")})));
    const auto result = summary.json(); const auto& lane = result["lanes"][0];
    CHECK(lane["headway"]["n"] == 2); CHECK(lane["headway"]["mean"] == 3); // not cycle-weighted 8/3
    CHECK(lane["startupLostTime"]["mean"] == 1); CHECK(lane["observedSeeds"] == 3);
    CHECK(lane["cycles"] == 4); CHECK(lane["headwayUnavailableReasons"]["queue_not_sustained"] == 1);
    CHECK(result["seedMeans"][2]["headway"]["mean"].is_null());
    CHECK(result["runs"][0]["safetyClamps"]["n"] == 3);
    CHECK(reactiondischarge::Moments{}.json()["mean"].is_null());
}
