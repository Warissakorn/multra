// M9 route-costs file (docs/reference/DYNAMIC_ASSIGNMENT.md §5, §11, D549): "MultraRouteCosts" version 1, written by
// the dynamic-assignment loop and read back by --route-costs.
#pragma once
#include "dynamic_assignment_run.hpp"
#include <nlohmann/json.hpp>
#include <cstdint>

namespace multra {
struct RouteCostsProvenance { std::string project; std::uint32_t seed{}; DaSettings settings; std::string tool; };
struct RouteCostsCell {
    std::optional<std::size_t> interval; // none: "default"
    double start{}, end{};               // the interval's bounds, the staleness key (interval cells only)
    std::vector<double> weight, cost, count;
    std::size_t censored{};
    std::vector<std::size_t> freeFlowFilled; // entry indices
};
struct RouteCostsDecision { std::string id; std::vector<RouteCostsCell> cells; };
struct RouteCosts {
    RouteCostsProvenance provenance;
    DaLevel level{DaLevel::micro}; // written as "level": "meso" only for Meso, which has no seed (D550)
    bool converged{};
    bool cancelled{}; // D552: the loop was stopped; never written to the file
    std::vector<double> deltas; // per iteration, n = 1 first
    std::vector<RouteCostsDecision> decisions;
    std::vector<std::string> skipped;
};
// The file of one run's weights and its measurement (deltas and converged are the caller's).
RouteCosts routeCostsFor(const AuthoringDefinition&, const DaPlan&, const std::vector<std::vector<double>>& weights,
                         const DaMeasurement&, const RouteCostsProvenance&);
nlohmann::json routeCostsJson(const RouteCosts&);
// ROUTE_COSTS_INVALID at the faulty path.
RouteCosts parseRouteCosts(const nlohmann::json&);
// Each listed cell's weights replace its flows; ROUTE_COSTS_STALE at decisions[d] when the decision is missing or its
// entry count or interval bounds differ.
// ROUTE_COSTS_STALE at `level` when the file's level is not the run's.
ProjectDocument applyRouteCosts(ProjectDocument, const RouteCosts&, DaLevel run = DaLevel::micro);
}
