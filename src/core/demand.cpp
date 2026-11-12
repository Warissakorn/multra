#include "detail.hpp"
#include "routes.hpp"
#include "speed_distribution.hpp"
#include "simulation.hpp"
#include "w74.hpp"
#include <cmath>
#include <numbers>

namespace multra::detail {
double random(std::uint32_t& state) {
    // Unsigned shifts and overflow intentionally match xorshift32 in the TS baseline.
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return (static_cast<double>(state) + 0.5) / 4294967296.0;
}
// INPUT_STREAMS §4: the run stream's seeding (D159, the same gamma), mixed again with the object's key.
std::uint32_t streamState(std::uint32_t seed, std::uint64_t key) {
    const auto state = static_cast<std::uint32_t>(splitmix64Mix(splitmix64Mix(seed + 0x9E3779B97F4A7C15ULL) ^ key) >> 32);
    return state == 0 ? 0x6d2b79f5U : state; // xorshift32 never leaves zero
}
double hashedUniform(std::uint32_t seed, std::uint64_t origin, std::uint32_t ordinal, std::uint64_t decision) {
    std::uint64_t h = splitmix64Mix(seed + 0x9E3779B97F4A7C15ULL);
    h = splitmix64Mix(h ^ origin);
    h = splitmix64Mix(h ^ ordinal);
    h = splitmix64Mix(h ^ decision);
    return (static_cast<double>(h >> 11) + 0.5) / 9007199254740992.0; // 53 bits, never 0 or 1
}
std::uint64_t traitIdentity(std::uint64_t vehicleId, std::uint64_t origin, std::uint32_t ordinal) {
    return origin == 0 ? vehicleId : splitmix64Mix(splitmix64Mix(origin) ^ ordinal);
}
// One new vehicle: its desired speed and driver factor drawn in this order, then (only with a w74
// behaviour) its hashed traits. Arrivals and a lot's initial occupants (M8.4a) both come from here.
// `origin`/`ordinal` are set only under perInput (zero otherwise) and go in before the trait hash (D512).
PendingVehicle samplePending(SimState& state, std::uint32_t& rng, std::uint32_t input, std::uint32_t route,
                             std::uint32_t typeIndex, double scheduledTime, std::uint64_t origin, std::uint32_t ordinal) {
    const auto& type = state.scenario->vehicleTypes[typeIndex];
    const double desiredSpeed = speedAt(type.desiredSpeed, random(rng)); // B-5 (D229)
    // Separate statements specify draw order; C++ operand evaluation order must
    // never choose which draw goes to the logarithm versus cosine.
    const double radiusDraw = random(rng);
    const double angleDraw = random(rng);
    const double gaussian = std::sqrt(-2 * std::log(radiusDraw)) *
                            std::cos(2 * std::numbers::pi * angleDraw);
    // B-4 (D236): the spread of the type's own behaviour, never the road's; 0.15 without the key.
    const auto& spread = state.scenario->behaviours[state.index->behaviourOfType[typeIndex]].driverFactorSpread;
    PendingVehicle v{state.nextVehicleId++, input, route, typeIndex, scheduledTime, desiredSpeed,
                     std::clamp(0.5 + spread.value_or(0.15) * gaussian, 0.0, 1.0)};
    v.streamKey = origin; v.ordinal = ordinal;
    // Hashed, not drawn: randomState's sequence is the same with or without W74 (D129).
    if (state.index->w74) v.w74Traits = w74Traits(state.seed, traitIdentity(v.id, origin, ordinal), v.driverFactor);
    return v;
}
void initializeInputs(SimState& state) {
    const bool perInput = state.scenario->randomStreams == RandomStreams::perInput;
    for (std::size_t i = 0; i < state.scenario->inputs.size(); ++i) {
        const auto& input = state.scenario->inputs[i];
        InputState current{std::nullopt, {}};
        if (perInput) current.stream = streamState(state.seed, state.index->keyOfInput[i]);
        auto& rng = perInput ? current.stream : state.randomState;
        if (input.vehiclesPerHour != 0) {
            const double arrival = input.startTime - std::log(random(rng)) * 3600 / input.vehiclesPerHour;
            if (arrival < input.endTime) current.nextArrival = arrival;
        }
        state.inputs.push_back(std::move(current));
    }
}
void generateArrivals(SimState& state) {
    // state.inputs is parallel to scenario.inputs, so the input is the one at this position --
    // no id lookup, and the slot a released vehicle carries is this index.
    for (std::size_t i = 0; i < state.inputs.size(); ++i) {
        auto& current = state.inputs[i];
        // Most inputs are not due on most ticks: settle that before reading anything else.
        if (!current.nextArrival || *current.nextArrival > state.time) continue;
        const auto& input = state.scenario->inputs[i];
        // Resolved once per scenario in the index, not once per input per tick.
        const auto routeIndex = state.index->routeOfInput[i];
        const auto typeIndex = state.index->typeOfInput[i];
        const bool perInput = state.scenario->randomStreams == RandomStreams::perInput;
        auto& rng = perInput ? current.stream : state.randomState;
        while (current.nextArrival && *current.nextArrival <= state.time) {
            auto v = perInput ? samplePending(state, rng, static_cast<std::uint32_t>(i), routeIndex, typeIndex,
                                              *current.nextArrival, state.index->keyOfInput[i], current.generated++)
                              : samplePending(state, rng, static_cast<std::uint32_t>(i), routeIndex, typeIndex, *current.nextArrival);
            current.queue.push_back(std::move(v));
            const double arrival = *current.nextArrival - std::log(random(rng)) * 3600 / input.vehiclesPerHour;
            current.nextArrival = arrival < input.endTime ? std::optional(arrival) : std::nullopt;
        }
    }
}
}
namespace multra {
std::vector<PendingVehicle> upcomingArrivals(const SimState& state) {
    // Mirrors stepSimulation's preconditions and its first action, on a scratch state, so the
    // caller's inputs, RNG and id counter are never touched. Most ticks generate nothing.
    if (!state.scenario || state.inputs.size() != state.scenario->inputs.size() ||
        state.tick >= totalTicks(*state.scenario) ||
        std::none_of(state.inputs.begin(), state.inputs.end(),
                     [&](const auto& i) { return i.nextArrival && *i.nextArrival <= state.time; }))
        return {};
    SimState scratch;
    scratch.scenario = state.scenario;
    scratch.index = state.index ? state.index
                                : std::make_shared<const ScenarioIndex>(buildScenarioIndex(*state.scenario));
    scratch.seed = state.seed; scratch.randomState = state.randomState; scratch.nextVehicleId = state.nextVehicleId;
    scratch.time = state.time; scratch.inputs = state.inputs;
    detail::generateArrivals(scratch);
    std::vector<PendingVehicle> result;
    for (const auto& input : scratch.inputs)
        for (const auto& v : input.queue) if (v.id >= state.nextVehicleId) result.push_back(v);
    std::sort(result.begin(), result.end(), [](const auto& a, const auto& b) { return a.id < b.id; });
    return result;
}
}
