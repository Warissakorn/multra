#include "test.hpp"
#include "../src/core/random_streams.hpp"
#include "../src/core/w74.hpp"
#include "../src/core/w99.hpp"
#include <algorithm>
using namespace multra;
// M3.3.3b (D641): row BA229 of docs/plans/DRIVING_BEHAVIOUR.md -- the hashed driver trait zW of
// docs/reference/W99.md §5. Nothing reads it yet; composition (BA230) will.
namespace {
const W99Parameters kW99{1.5, 1.3, 4, -12, -0.25, 0.35, 6, 0.25, 2, 1.5};
const W74Parameters kW74{2, 2, 1, 2, .5, 16, 4, .5, .5, 20, 1, 1, 3, .25, .5, -1, .5, .25};
// test::straight with unused behaviours: the run is the prototype's, vehicles carry traits.
Scenario with(bool w74, bool w99) {
    auto s = test::straight();
    if (w74) { DriverBehaviour b{"w74"}; b.w74 = kW74; s.behaviours.push_back(b); }
    if (w99) { DriverBehaviour b{"w99"}; b.w99 = kW99; s.behaviours.push_back(b); }
    return s;
}
template<class V> std::vector<V> stripped(std::vector<V> list) {
    for (auto& v : list) v.w99Trait.reset();
    return list;
}
SimState after(const Scenario& s, std::uint32_t seed, int ticks) { // vehicles still on the road
    auto state = createSimulation(s, seed);
    for (int i = 0; i < ticks; ++i) state = stepSimulation(state);
    return state;
}
template<class V> double expected(std::uint32_t seed, const V& v) {
    return w99Trait(seed, detail::traitIdentity(v.id, v.streamKey, v.ordinal));
}
}

TEST(w99traits, the_hash_matches_an_independent_implementation_bit_for_bit) { // BA229
    // Golden values from a separate Python implementation of W74 §5's u(15) (docs/evidence/w99-traits.md).
    struct Case { std::uint32_t seed; std::uint64_t id; double expected; };
    const Case cases[] = {{42, 1, 0x1.19f600c4886eep-1}, {0, 7, 0x1.4aa7714cb5597p-1},
                          {0xffffffffU, 1ULL << 40, 0x1.9047ca15f75a6p-1}};
    for (const auto& c : cases) CHECK(w99Trait(c.seed, c.id) == c.expected);
    CHECK(kW99TraitIndex == 15);
    CHECK(w99Trait(0, 7) != w99Trait(0x6d2b79f5U, 7)); // seed 0 is hashed as given, not remapped
    for (std::uint64_t id = 1; id <= 2000; ++id) {
        const double z = w99Trait(42, id);
        CHECK(z >= 0 && z < 1); CHECK(z == w99Trait(42, id));
        for (std::uint64_t i = 0; i <= 14; ++i) CHECK(z != w74Uniform(42, id, i)); // independent of W74's draws
    }
    CHECK(w99Trait(42, 1) != w99Trait(42, 2)); CHECK(w99Trait(42, 1) != w99Trait(43, 1));
    // The factored hash leaves W74's traits as they were (BA26's golden row).
    CHECK(w74Traits(42, 1, 0.5).zEx == w74Uniform(42, 1, 0)); CHECK(w74Uniform(42, 1, 0) == 0x1.713d41f672900p-7);
}

TEST(w99traits, a_mixed_scenario_keeps_the_prototype_draws_and_every_vehicle_its_trait) { // BA229
    auto plain = createSimulation(test::straight(), 42), w99 = createSimulation(with(false, true), 42);
    std::size_t seen = 0;
    while (plain.tick < totalTicks(*plain.scenario)) {
        const auto upcoming = upcomingArrivals(w99);
        plain = stepSimulation(plain); w99 = stepSimulation(w99);
        CHECK(plain.randomState == w99.randomState); CHECK(plain.nextVehicleId == w99.nextVehicleId);
        CHECK(plain.events == w99.events);
        CHECK(stripped(plain.vehicles) == stripped(w99.vehicles));
        for (std::size_t i = 0; i < plain.inputs.size(); ++i) CHECK(stripped(plain.inputs[i].queue) == stripped(w99.inputs[i].queue));
        for (const auto& v : plain.vehicles) CHECK(!v.w99Trait && !v.w74Traits);
        for (const auto& v : w99.vehicles) { CHECK(v.w99Trait == expected(42, v)); CHECK(!v.w74Traits); ++seen; }
        for (const auto& p : upcoming) CHECK(p.w99Trait == expected(42, p));
    }
    CHECK(seen > 0);
}

TEST(w99traits, prototype_w74_and_w99_together_keep_both_traits_independent) { // BA229
    const auto w74 = after(with(true, false), 42, 600), both = after(with(true, true), 42, 600);
    CHECK(!both.vehicles.empty()); CHECK(w74.randomState == both.randomState); CHECK(w74.events == both.events);
    CHECK(stripped(both.vehicles) == w74.vehicles); // w74Traits exactly the W74-only run's
    for (const auto& v : both.vehicles) {
        CHECK(v.w74Traits.has_value()); CHECK(v.w99Trait == expected(42, v));
        CHECK(*v.w99Trait != v.w74Traits->zEx && *v.w99Trait != v.w74Traits->zCx && *v.w99Trait != v.w74Traits->zOsc);
    }
}

TEST(w99traits, traits_survive_a_copied_state_and_replay_exactly) { // BA229
    auto state = createSimulation(with(false, true), 7);
    for (int i = 0; i < 300; ++i) state = stepSimulation(state);
    const auto copy = state;
    auto a = state, b = copy;
    for (int i = 0; i < 300; ++i) { a = stepSimulation(a); b = stepSimulation(b); }
    CHECK(a.vehicles == b.vehicles);
    CHECK(test::finish(createSimulation(with(false, true), 7)).vehicles == test::finish(createSimulation(with(false, true), 7)).vehicles);
}

TEST(w99traits, per_input_streams_hash_the_stream_identity) { // BA229, INPUT_STREAMS §4
    auto s = with(false, true); s.randomStreams = RandomStreams::perInput;
    const auto run = after(s, 42, 600);
    CHECK(!run.vehicles.empty());
    std::size_t keyed = 0;
    for (const auto& v : run.vehicles) {
        CHECK(v.w99Trait == expected(42, v));
        if (v.streamKey != 0) { ++keyed; CHECK(v.w99Trait != w99Trait(42, v.id)); }
    }
    CHECK(keyed > 0);
}

TEST(w99traits, checkpoints_carry_the_trait_only_when_present) { // W74 §9's rule
    auto plain = createSimulation(with(true, false), 42), w99 = createSimulation(with(false, true), 42);
    while (plain.vehicles.empty() && plain.tick < totalTicks(*plain.scenario)) {
        plain = stepSimulation(plain); w99 = stepSimulation(w99);
    }
    CHECK(!plain.vehicles.empty()); CHECK(!w99.vehicles.empty());
    CHECK(checkpointJson(plain).dump().find("w99Trait") == std::string::npos);
    const auto w = checkpointJson(w99);
    CHECK(w["vehicles"][0]["w99Trait"] == *w99.vehicles.front().w99Trait);
}
