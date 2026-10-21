#include "compare.hpp"
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <stdexcept>

namespace multra {
namespace {
// `make(base row, alternative row)` builds the compared row of a name found once on each side.
template<class Row, class Make>
auto match(const std::vector<Row>& base, const std::vector<Row>& alternative, Make make, Unmatched& unmatched) {
    std::map<std::string, std::size_t> inBase, inAlternative;
    for (const auto& row : base) ++inBase[row.name];
    for (const auto& row : alternative) ++inAlternative[row.name];
    std::set<std::string> listed; // an ambiguous name is listed once
    const auto ambiguous = [&](const std::string& name) {
        if (listed.insert(name).second) unmatched.ambiguous.push_back(name);
    };
    std::vector<decltype(make(base.front(), base.front()))> rows;
    for (const auto& row : base) {
        const auto other = inAlternative[row.name];
        if (inBase[row.name] > 1 || other > 1) ambiguous(row.name);
        else if (other == 0) unmatched.baseOnly.push_back(row.name);
        else {
            const auto& paired = *std::find_if(alternative.begin(), alternative.end(),
                                               [&](const Row& r) { return r.name == row.name; });
            rows.push_back(make(row, paired));
        }
    }
    for (const auto& row : alternative) {
        if (inBase[row.name] > 0) continue; // compared, or already listed from the base
        if (inAlternative[row.name] > 1) ambiguous(row.name);
        else unmatched.alternativeOnly.push_back(row.name);
    }
    return rows;
}
// One value per row, as every block before S0.8a.
template<class Row, class Value>
std::vector<ComparisonRow> matchValue(const std::vector<Row>& base, const std::vector<Row>& alternative, Value value,
                                      Unmatched& unmatched) {
    return match(base, alternative, [&](const Row& b, const Row& a) { return ComparisonRow{b.name, welch(value(b), value(a))}; },
                 unmatched);
}
ComparisonNodeRow compareNode(const BatchNodeRow& base, const BatchNodeRow& alternative) {
    ComparisonNodeRow row{base.name, welch(base.delay, alternative.delay), {}, {}, {}, {}};
    row.legs = match(base.legs, alternative.legs, [](const BatchNodeLegRow& b, const BatchNodeLegRow& a) {
        return ComparisonLegRow{b.name, welch(b.delay, a.delay), welch(b.queueMean, a.queueMean)};
    }, row.unmatchedLegs);
    row.turns = matchValue(base.turns, alternative.turns, [](const BatchMovementRow& r) { return r.meanDelay; },
                           row.unmatchedTurns);
    return row;
}
}
Difference welch(const Estimate& base, const Estimate& alternative) {
    Difference d{base.n, alternative.n, base.mean, alternative.mean, {}, {}, {}};
    if (base.mean && alternative.mean) d.difference = *alternative.mean - *base.mean;
    if (!d.difference || !base.sd || !alternative.sd) return d;
    const double vb = *base.sd * *base.sd / static_cast<double>(base.n);
    const double va = *alternative.sd * *alternative.sd / static_cast<double>(alternative.n);
    const double variance = vb + va;
    if (variance == 0) { d.halfWidth95 = 0.0; return d; }
    const double nu = variance * variance /
                      (vb * vb / static_cast<double>(base.n - 1) + va * va / static_cast<double>(alternative.n - 1));
    d.degreesOfFreedom = nu;
    // nu >= min(n) - 1 >= 1 in exact arithmetic; the epsilon keeps a rounded 4.9999... at 5.
    const auto whole = std::max<std::size_t>(1, static_cast<std::size_t>(std::floor(nu + 1e-9)));
    d.halfWidth95 = tQuantile975(whole) * std::sqrt(variance);
    return d;
}
Comparison compareBatches(const BatchReport& base, const BatchReport& alternative) {
    if (base.seeds != alternative.seeds) throw std::invalid_argument("compare: the batches ran different seed lists");
    Comparison c;
    c.seeds = base.seeds;
    c.movements = matchValue(base.movements, alternative.movements, [](const BatchMovementRow& r) { return r.meanDelay; },
                             c.unmatchedMovements);
    c.sections = matchValue(base.sections, alternative.sections, [](const BatchSectionRow& r) { return r.meanDelay; },
                            c.unmatchedSections);
    c.queues = matchValue(base.queues, alternative.queues, [](const BatchQueueRow& r) { return r.meanLength; },
                          c.unmatchedQueues);
    c.network = welch(base.meanDelay, alternative.meanDelay);
    c.nodes = match(base.nodes, alternative.nodes, compareNode, c.unmatchedNodes);
    return c;
}
}
