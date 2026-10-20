#pragma once
// S0.3 (docs/reference/NODES.md §4, D264): Micro node evaluation. A Node's turns are ordinary
// travel-time sections and its legs ordinary queue counters; this only groups their rows. Simulated
// section delay, NOT HCM control delay, not validated (rule 4).
#include "sections.hpp"

namespace trafficsim {
struct NodeConflictRow {
    std::string conflictArea, first, second, kind, priority; // turn names; control.hpp's enum names
    bool operator==(const NodeConflictRow&) const = default;
};
struct NodeLegSpec { std::string name; std::vector<CounterLine> queueLines; }; // lines at the in-leg crossing
struct NodeTurnSpec { SectionSpec section; std::size_t leg{}; };            // leg: index into legs
struct NodeSpec {
    std::string name; std::optional<std::string> controlType; // "signalised" / "unsignalised"
    std::vector<NodeLegSpec> legs; std::vector<NodeTurnSpec> turns;
    std::vector<NodeConflictRow> conflicts; std::vector<std::string> issues;
};
struct NodeTurnRow {
    std::string name; std::uint64_t vehicles{};
    std::optional<double> delay, travelTime; std::uint64_t unfinished{};
    bool operator==(const NodeTurnRow&) const = default;
};
struct NodeLegRow {
    std::string name; std::uint64_t vehicles{};
    std::optional<double> delay; std::uint64_t unfinished{};
    double queueMean{}, queueMax{};
    bool operator==(const NodeLegRow&) const = default;
};
struct NodeRow {
    std::string name; std::optional<std::string> controlType;
    std::uint64_t vehicles{}; std::optional<double> delay; std::uint64_t unfinished{};
    std::vector<NodeLegRow> legs; std::vector<NodeTurnRow> turns;
    std::vector<NodeConflictRow> conflicts; std::vector<std::string> issues;
    bool operator==(const NodeRow&) const = default;
};
// Vehicle-weighted mean of the delays given, Σ vᵢdᵢ / Σ vᵢ; none without a vehicle.
std::optional<double> weightedDelay(const std::vector<std::pair<std::uint64_t, std::optional<double>>>&);
// `turns` holds one SectionRow per NodeTurnSpec and `queues` one {mean, max} per NodeLegSpec, both
// in spec order across every Node.
std::vector<NodeRow> nodeRows(const std::vector<NodeSpec>&, const std::vector<SectionRow>& turns,
                              const std::vector<std::pair<double, double>>& queues);
}
