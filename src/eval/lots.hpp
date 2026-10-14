#pragma once
#include "../core/types.hpp"
#include <map>
#include <optional>

namespace trafficsim {
// M8.5 (D160, docs/reference/ZONES_AND_PARKING.md §5): each parking lot's occupancy, flows and gate
// waits, from SimState snapshots alone -- the engine emits nothing new for it. NOT validated.
// - An admission is the ArrivedEvent on one of the lot's gate routes; its gate wait is the time from
//   the end of gate service (arrival at the line + gateServiceTime) to admission, floored at 0; a
//   vehicle that drove in without stopping at the line waits 0.
// - A departure ("out") is a vehicle leaving the lot: its id drops out of the lot's parked and leaving
//   lists together, i.e. it was inserted from the exit queue (possibly in the step that released it).
// - Window: flows and waits count when they happen in [warmup, end]; the occupancy mean and peak are
//   over the snapshots in it, and occupancyEnd is the last snapshot at or before `end`.
struct LotSpec { std::string name, id; bool operator==(const LotSpec&) const = default; };
struct LotRow {
    std::string name;
    std::uint32_t capacity{};
    std::uint64_t occupancyEnd{}, occupancyPeak{};
    double occupancyMean{};
    std::uint64_t admitted{}, departed{}, waitedOverTick{};
    std::optional<double> gateWaitMean, gateWaitMax; // none without an admission in the window
    std::uint64_t endParked{}, endAtGate{}, endInExitQueue{}; // at the report, whatever the window
    bool operator==(const LotRow&) const = default;
};
class LotAccumulator {
public:
    LotAccumulator(std::vector<LotSpec> lots, double warmup, std::optional<double> end);
    void observe(const SimState& state);
    std::vector<LotRow> report(const SimState& end) const;
private:
    void bind(const SimState& state);
    bool inPeriod(double time) const;
    std::vector<LotSpec> lots_;
    double warmup_{}; std::optional<double> periodEnd_;
    const Scenario* bound_{};
    std::vector<std::size_t> slot_; // spec -> scenario.parkingLots position (npos: not in the run)
    std::map<std::string, std::size_t> gateOf_; // gate route id -> spec
    std::vector<std::map<std::uint64_t, std::uint64_t>> gateSince_; // per spec: last snapshot's gate
    std::vector<std::vector<std::uint64_t>> leaving_;               // per spec: last snapshot's held ids, sorted
    std::vector<LotRow> rows_;
    std::vector<double> occupancySum_, waitSum_;
    std::vector<std::uint64_t> samples_;
};
// Vehicles held in lots and not yet released to an exit queue (those are already pending).
std::size_t parkedCount(const SimState& state);
}
