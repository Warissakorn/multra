// M9 dynamic assignment over a project (docs/reference/DYNAMIC_ASSIGNMENT.md §2-§3, §11, D549): the cells of a
// project's unplaced routing decisions, their weights, and their costs measured from one Micro run.
#pragma once
#include "dynamic_assignment.hpp"
#include "document.hpp"
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace multra {
// The level a loop measures at: Micro runs (D549) or Meso runs (MESO_ASSIGNMENT, D550).
enum class DaLevel { micro, meso };
// One decision x one of its intervals; no interval is the decision's whole-period ("default") cell.
struct DaCellRef { std::size_t decision{}; std::optional<std::size_t> interval; };
struct DaPlan {
    std::vector<std::size_t> decisions; // indices into routingDecisions, in authored order
    std::vector<std::string> skipped;   // ids of decisions with fewer than two entries
    std::vector<DaCellRef> cells;       // per decision: its intervals in order, then default
};
// Refuses (ValidationError): no decision at all or none with two entries, a decision with type rules or an
// all-zero default (DYNAMIC_ASSIGNMENT_INVALID), a placed decision (DYNAMIC_ASSIGNMENT_UNSUPPORTED, §11).
DaPlan planDynamicAssignment(const AuthoringDefinition&);
// Each cell's entry flows normalised to shares; an all-zero interval takes the default shares.
std::vector<std::vector<double>> decisionWeights(const AuthoringDefinition&, const DaPlan&);
// Writes the shares back: the default cell to relativeFlow, interval k to intervalFlows[k].
void applyDecisionWeights(AuthoringDefinition&, const DaPlan&, const std::vector<std::vector<double>>& weights);
struct DaMeasurement {
    std::vector<DaCellCosts> cells;   // in plan.cells order
    std::vector<std::size_t> censored; // per cell, vehicles still running at the end
};
// One run of the compiled document at `seed`: each vehicle of a decision costs its departure-to-arrival time (or
// its elapsed time if still running), in the cell of its scheduled time; empty choices are free-flow-filled.
DaMeasurement measureDecisionCosts(const ProjectDocument&, const std::filesystem::path& data, const DaPlan&, std::uint32_t seed);
}
