#pragma once
#include "batch.hpp"

namespace multra {
// Scenario comparison (M5.8a, D148, BATCH §7): two batches over one seed list, alternative minus
// base, row by row. Under shared, equal seeds are not common random numbers -- each run's stream
// diverges once the projects differ -- so the interval is Welch's for independent samples; under
// perInput the rows pair by seed (D526). NOT HCM control delay, NOT LOS, not validated (rule 4).
struct Difference {
    std::size_t nBase{}, nAlternative{};
    std::optional<double> base, alternative;              // the two means
    std::optional<double> difference;                     // alternative - base
    std::optional<double> halfWidth95, degreesOfFreedom;  // empty with n < 2 on a side
    std::optional<std::size_t> pairs;                     // D526: set only by `paired`
    bool operator==(const Difference&) const = default;
};
// d = mean_a - mean_b, SE = sqrt(s_b^2/n_b + s_a^2/n_a), Welch-Satterthwaite nu, half-width
// tQuantile975(floor(nu)) * SE. Both SDs zero: half-width 0 and no nu.
Difference welch(const Estimate& base, const Estimate& alternative);
// D526 (BATCH §7): under perInput a seed's two runs share every unedited input's draws, so rows
// pair by seed. The m seeds with a value on both sides give d_i = a_i - b_i; d = mean(d_i), the
// half-width tQuantile975(m - 1) * s_d / sqrt(m), df m - 1. The shown means stay each side's own.
// m < 2: no interval (no d with m = 0). Both estimates must carry perSeed over the same seeds.
Difference paired(const Estimate& base, const Estimate& alternative);

struct ComparisonRow {
    std::string name; Difference value;
    bool operator==(const ComparisonRow&) const = default;
};
// The names a block did not compare: in one batch only, or more than once in either.
struct Unmatched {
    std::vector<std::string> baseOnly, alternativeOnly, ambiguous;
    bool empty() const { return baseOnly.empty() && alternativeOnly.empty() && ambiguous.empty(); }
    bool operator==(const Unmatched&) const = default;
};
// S0.8a (D289, NODES §17): a Node's rows, matched by Node name, then legs and turns by name within
// the Node. Delay in s for the Node, its in-legs and turns; mean queue in m for the legs.
struct ComparisonLegRow {
    std::string name; Difference delay, queueMean;
    bool operator==(const ComparisonLegRow&) const = default;
};
struct ComparisonNodeRow {
    std::string name; Difference delay;
    std::vector<ComparisonLegRow> legs; std::vector<ComparisonRow> turns;
    Unmatched unmatchedLegs, unmatchedTurns;
    bool operator==(const ComparisonNodeRow&) const = default;
};
struct Comparison {
    std::vector<std::uint32_t> seeds;
    std::vector<ComparisonRow> movements, sections; // mean delay, s
    std::vector<ComparisonRow> queues;              // mean queue length, m
    Difference network;                             // network mean delay, s
    Unmatched unmatchedMovements, unmatchedSections, unmatchedQueues;
    std::vector<ComparisonNodeRow> nodes; Unmatched unmatchedNodes; // S0.8a; empty without a Node
    RandomStreams randomStreams{}; // INPUT_STREAMS §5 (IS8): both batches' mapping
    bool operator==(const Comparison&) const = default;
};
// Rows matched by name in the base's order; paired by seed under perInput (D526), Welch under
// shared. Throws when the batches ran different seed lists or different random-stream mappings
// (IS8): the same seed then means different draws.
Comparison compareBatches(const BatchReport& base, const BatchReport& alternative);
}
