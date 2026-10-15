#pragma once
// BA38 experiment seams; tools/tests only. Measurement semantics remain in eval/discharge.
#include "discharge_options.hpp"
#include "w74_fixture_document.hpp"
#include "../src/core/simulation.hpp"
#include "../src/project/evaluation.hpp"
#include <algorithm>
#include <optional>

namespace trafficsim::reactiondischarge {
inline ProjectDocument variant(const ProjectDocument& base, const DriverBehaviour* w74,
                               std::optional<double> reaction) {
    auto document = w74fixture::arm(base, w74, .1);
    for (auto& b : document.definition->behaviours) b.perception.reactionTime = reaction;
    validateDocument(document);
    return document;
}
// Scenario identities/index pointers differ across independently created runs. Every mutable
// state value, including action FIFOs and complete latest-tick events, must agree exactly.
inline bool sameState(const SimState& a, const SimState& b) {
    return a.seed == b.seed && a.randomState == b.randomState && a.tick == b.tick &&
        a.time == b.time && a.nextVehicleId == b.nextVehicleId && a.completed == b.completed &&
        a.inputs == b.inputs && a.vehicles == b.vehicles && a.events == b.events &&
        a.stopService == b.stopService && a.lots == b.lots;
}
// Raw cycles retain null estimates, reasons and crossings. A zero variant is checked against
// the absent-key scenario on EVERY tick. Each run also checks 32 ticks of copied-state replay.
inline Json measure(const Scenario& scenario, const QueueDefinition& queue, std::uint32_t seed,
                    const Scenario* absent = nullptr) {
    if (!scenario.parkingLots.empty()) throw std::invalid_argument("BA38 does not support parking lots");
    const auto spec = DischargeOptions{}.forDuration(scenario.duration);
    DischargeAccumulator discharge(spec, queue);
    SummaryAccumulator summary;
    auto state = createSimulation(scenario, seed);
    std::optional<SimState> baseline, replay;
    if (absent) baseline = createSimulation(*absent, seed);
    discharge.observe(state);
    const auto equal = [&](const SimState& other, const char* label) {
        if (!sameState(state, other)) throw std::runtime_error(label);
    };
    if (baseline) equal(*baseline, "BA38 zero/absent mismatch at creation");
    std::size_t replayTicks = 0;
    bool replayWithHistory = false;
    const auto ticks = totalTicks(scenario);
    while (state.tick < ticks) {
        if (state.tick == ticks / 2) {
            replay = state;
            replayWithHistory = std::any_of(state.vehicles.begin(), state.vehicles.end(),
                [](const auto& v) { return !v.reactionHistory.empty(); });
        }
        state = stepSimulation(std::move(state));
        if (baseline) {
            *baseline = stepSimulation(std::move(*baseline));
            equal(*baseline, "BA38 zero/absent trajectory mismatch");
        }
        if (replay && replayTicks < 32) {
            *replay = stepSimulation(std::move(*replay));
            equal(*replay, "BA38 copied-state replay mismatch");
            ++replayTicks;
        }
        discharge.observe(state);
        for (const auto& event : state.events) summary.add(event);
    }
    const auto pending = pendingCount(state);
    if (state.nextVehicleId - 1 != state.completed + state.vehicles.size() + pending)
        throw std::runtime_error("BA38 vehicle accounting mismatch");
    const auto measured = summary.summary();
    return {{"seed", seed}, {"completed", state.completed}, {"active", state.vehicles.size()},
        {"pending", pending}, {"generated", state.nextVehicleId - 1},
        {"safetyClamps", measured.safetyClamps}, {"time", state.time},
        {"zeroAbsentTicksChecked", baseline ? ticks : 0}, {"copiedReplayTicks", replayTicks},
        {"copiedReplayWithHistory", replayWithHistory},
        {"discharge", dischargeJson(discharge.report(), spec)}};
}
}
