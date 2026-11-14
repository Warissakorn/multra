#pragma once
#include "movement.hpp"
#include <map>
#include <optional>
#include <set>

namespace multra {
// D528 (docs/reference/PRIORITY_LINES.md): the counter at each conflict area's waiting line, for
// M6's U rows. Minor vehicles whose front passes the waiting line, major vehicles whose front enters
// the area, both interpolated in time and counted in the evaluation window, and the follow-up
// headway between consecutive minor crossings whose follower was queued at the leader's crossing
// with no major entry between them. A measured count, not an HCM capacity; not validated.
// D534 (PRIORITY_LINES §5): Siegloch's regression for the effective critical headway. A point is
// the mean of the saturated major gaps that `vehicles` minor vehicles entered.
struct SieglochPoint {
    std::uint64_t vehicles{}, gaps{}; double meanGap{};
    bool operator==(const SieglochPoint&) const = default;
};
struct SieglochFit {
    double t0{}, tf{}, tc{}; // intercept, slope, t0 + tf / 2 (TFT8 Eq. 8.1)
    bool operator==(const SieglochFit&) const = default;
};
// Unweighted least squares of meanGap on vehicles; nothing with fewer than two distinct vehicles.
std::optional<SieglochFit> sieglochFit(const std::vector<SieglochPoint>& points);
struct PriorityLineRow {
    std::string zoneId; ZoneControl control{};
    double window{};
    std::uint64_t minorCrossed{}, majorEntered{}, followUps{}, excluded{};
    double minorPerHour{}, majorPerHour{};
    std::optional<double> followUpMean, followUpSd; // n >= 1, n >= 2
    std::uint64_t sieglochGaps{};              // saturated major gaps, n = 0 included
    std::vector<SieglochPoint> sieglochPoints; // n >= 1, ascending
    std::optional<SieglochFit> siegloch;
    bool operator==(const PriorityLineRow&) const = default;
};
struct PriorityLineReport { std::vector<PriorityLineRow> rows; }; // in Scenario::conflictZones order

class PriorityLineAccumulator {
public:
    explicit PriorityLineAccumulator(const EvaluationSpec& spec);
    // Once per state: after createSimulation and after every stepSimulation.
    void observe(const SimState& state);
    PriorityLineReport report() const;
private:
    struct Crossing { double time{}; std::optional<double> queuedSince; };
    // The id and control are copied at binding: the report may outlive the run's scenario.
    struct Zone { std::string id; ZoneControl control{}; std::vector<Crossing> minor; std::vector<double> major; std::uint64_t excluded{}; };
    struct Position { std::uint32_t route{}; double distance{}; };
    double beginSpeed_{}, maxGap_{}, warmup_{};
    std::optional<double> end_;
    double duration_{};
    const Scenario* bound_{}; // compared only, never read after the run
    std::vector<Zone> zones_;
    std::map<std::uint64_t, Position> previous_;
    std::set<std::tuple<std::uint64_t, std::size_t, int>> done_; // (vehicle, zone, role) already passed
    std::map<std::pair<std::uint64_t, std::size_t>, double> queuedSince_; // (vehicle, zone) -> first stood
};
}
