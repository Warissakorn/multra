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
struct PriorityLineRow {
    std::string zoneId; ZoneControl control{};
    double window{};
    std::uint64_t minorCrossed{}, majorEntered{}, followUps{}, excluded{};
    double minorPerHour{}, majorPerHour{};
    std::optional<double> followUpMean, followUpSd; // n >= 1, n >= 2
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
