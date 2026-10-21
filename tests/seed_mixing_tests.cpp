// B11 (D159): the seed is mixed (splitmix64) before the first xorshift32 draw, so small seeds such as
// the D88 set 42-81 no longer start the sequence near zero. SeedStart::raw keeps the old mapping for the
// frozen TypeScript fixtures only (tests/reference).
#include "test.hpp"
#include <cmath>
using namespace multra;
namespace {
Scenario oneInput(double vehiclesPerHour, double duration) {
    auto s = test::straight();
    s.duration = duration;
    s.inputs = {{"input", "route", "car", vehiclesPerHour, 0, duration}};
    return s;
}
std::uint32_t initialState(std::uint32_t seed, SeedStart start = SeedStart::mixed) {
    auto s = test::straight(); s.inputs.clear(); // no input, no lot: nothing draws before we look
    return createSimulation(s, seed, start).randomState;
}
}
TEST(seedmixing, sm1_first_draw_is_uniform_for_small_seeds) {
    // At 3600 veh/h from t = 0 the first arrival is -ln(u) s, so u is recovered exactly enough.
    const auto s = oneInput(3600, 120);
    double sum = 0; int low = 0; const int n = 1000;
    for (std::uint32_t seed = 42; seed < 42 + n; ++seed) {
        const auto state = createSimulation(s, seed);
        CHECK(state.inputs[0].nextArrival.has_value());
        const double u = std::exp(-*state.inputs[0].nextArrival);
        sum += u; if (u < 0.05) ++low;
    }
    CHECK(std::abs(sum / n - 0.5) < 0.03);
    CHECK(low < n / 20);
}
TEST(seedmixing, sm2_same_seed_replays_and_keeps_the_user_seed) {
    const auto s = test::straight();
    const auto a = runSimulation(s, 42), b = runSimulation(s, 42);
    CHECK(a.seed == 42); CHECK(a.randomState == b.randomState);
    CHECK(a.completed == b.completed); CHECK(a.nextVehicleId == b.nextVehicleId);
    CHECK(initialState(42) != 42); // forcing: the mixed state is not the seed itself
}
TEST(seedmixing, sm3_edge_seeds_and_the_raw_mapping) {
    for (const std::uint32_t seed : {0u, 1u, 4294967295u}) CHECK(initialState(seed) != 0);
    CHECK(initialState(0, SeedStart::raw) == 0x6d2b79f5U);
    CHECK(initialState(42, SeedStart::raw) == 42);
    CHECK(initialState(4294967295u, SeedStart::raw) == 4294967295u);
}
TEST(seedmixing, sm4_first_arrival_is_not_late_over_the_d88_seeds) {
    // 27 veh/h: mean headway 133.3 s; over 40 seeds the mean first arrival has SE 21.1 s.
    const auto s = oneInput(27, 3600);
    double sum = 0;
    for (std::uint32_t seed = 42; seed <= 81; ++seed) {
        const auto state = createSimulation(s, seed);
        CHECK(state.inputs[0].nextArrival.has_value());
        sum += *state.inputs[0].nextArrival;
    }
    const double mean = sum / 40, headway = 3600.0 / 27;
    CHECK(std::abs(mean - headway) <= 1.96 * headway / std::sqrt(40.0));
}
