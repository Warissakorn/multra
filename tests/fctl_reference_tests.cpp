#include "test.hpp"
#include "../src/eval/fctl_reference.hpp"
#include <cmath>
#include <cstdint>
#include <deque>
#include <stdexcept>

using namespace multra;
// D529 (docs/reference/FCTL_REFERENCE.md): van Leeuwaarden (2006)'s FCTL procedure, rows FC1-FC5.
// The check values are the paper's Tables 1 and 2 (author's preprint, PDF pp. 12 and 14).
namespace {
bool near(double a, double b, double tolerance) { return std::abs(a - b) <= tolerance; }
template <class E> bool throws(const FctlInput& in) {
    try { fctlPoisson(in); } catch (const E&) { return true; }
    return false;
}
// A deterministic uniform draw (splitmix64) and Knuth's Poisson sampler, independent of the
// procedure under test.
struct Draw {
    std::uint64_t state;
    double uniform() {
        std::uint64_t z = (state += 0x9E3779B97F4A7C15ULL);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return static_cast<double>((z ^ (z >> 31)) >> 11) * 0x1.0p-53;
    }
    int poisson(double mu) {
        const double limit = std::exp(-mu);
        int k = 0;
        for (double p = uniform(); p > limit; p *= uniform()) ++k;
        return k;
    }
};
} // namespace

TEST(fctl, fc1_mean_overflow_matches_table_1) {
    const double mu[] = {0.30, 0.40, 0.45, 0.49}, expected[] = {0.1800, 1.0971, 3.3998, 23.2249};
    for (int i = 0; i < 4; ++i) CHECK(near(fctlPoisson({5, 5, mu[i]}).meanOverflow, expected[i], 5e-4));
}
TEST(fctl, fc2_mean_delay_matches_table_2) {
    const double mu[] = {0.30, 0.40, 0.45, 0.49}, expected[] = {2.7245, 5.0634, 9.9675, 49.8805};
    for (int i = 0; i < 4; ++i) CHECK(near(fctlPoisson({5, 5, mu[i]}).meanDelay, expected[i], 5e-4));
}
TEST(fctl, fc3_slot_simulation_agrees) {
    // Recursion 3.2 slot by slot, FIFO, with Assumption 3 (arrivals into an empty queue during
    // green pass undelayed) and Definition 1 (delay = departure slot - arrival slot).
    const int g = 5, r = 5, c = g + r, warmup = 1000, batches = 1000, perBatch = 1000;
    const double mu = 0.40;
    Draw draw{20261112};
    std::deque<long> queue; // arrival slot of each waiting vehicle
    long slot = 0;
    double sum = 0, sumSq = 0;
    for (int b = -1; b < batches; ++b) {
        double delay = 0, vehicles = 0;
        const int cycles = b < 0 ? warmup : perBatch;
        for (int n = 0; n < cycles; ++n) {
            for (int k = 0; k < c; ++k, ++slot) {
                const int arrivals = draw.poisson(mu);
                vehicles += arrivals;
                if (k < g && queue.empty()) continue; // they pass undelayed
                if (k < g) { delay += static_cast<double>(slot - queue.front()); queue.pop_front(); }
                for (int a = 0; a < arrivals; ++a) queue.push_back(slot);
            }
        }
        if (b < 0) continue;
        const double mean = delay / vehicles;
        sum += mean; sumSq += mean * mean;
    }
    const double mean = sum / batches, se = std::sqrt((sumSq / batches - mean * mean) / (batches - 1));
    CHECK(near(mean, fctlPoisson({g, r, mu}).meanDelay, 3 * se));
}
TEST(fctl, fc4_refusals) {
    CHECK(throws<std::invalid_argument>({5, 5, 0.5}));
    CHECK(throws<std::invalid_argument>({5, 5, 0.6}));
    CHECK(throws<std::invalid_argument>({0, 5, 0.1}));
    CHECK(throws<std::invalid_argument>({5, -1, 0.1}));
    CHECK(throws<std::invalid_argument>({5, 5, 0.0}));
}
TEST(fctl, fc5_sixteen_slots_converge) {
    double previous = 0;
    for (const double mu : {0.25, 0.35, 0.425}) {
        const auto result = fctlPoisson({16, 16, mu});
        CHECK(result.q.size() == 16);
        for (const double q : result.q) CHECK(q >= 0 && q <= 1);
        CHECK(result.meanDelay > previous);
        previous = result.meanDelay;
    }
}
