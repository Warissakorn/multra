#pragma once
#include "graph.hpp"
#include <functional>

// S2.2 (docs/reference/MACRO_NETWORK.md §7, D274): static user-equilibrium assignment by
// multi-class Frank-Wolfe. Pure: values in, values out, fixed index order, no clock, no seed.
namespace multra {
// t0 for an uncapacitated arc; t0 (1 + alpha (load / capacity)^beta) otherwise, load in pcu/h.
double macroTime(const MacroLink&, double load);
// Dijkstra from `origin` over the arcs `mode` may use. On equal distance the lower link index is the
// predecessor. `via[n]` is the arc reaching node n (graph.links.size() when none).
struct MacroTree { std::vector<double> distance; std::vector<std::size_t> via; };
MacroTree shortestTree(const MacroGraph&, std::size_t origin, const std::string& mode, const std::vector<double>& times);
// One class loaded on its shortest paths: veh/h per link, and sum of demand x shortest time.
struct MacroLoading { std::vector<double> flows; double shortestCost{}; };
MacroLoading allOrNothing(const MacroGraph&, const MacroClass&, const std::vector<double>& times);
// One recorded gap (S2.4, §19 a): the iteration it was computed at and the step then taken (0 at the end).
struct MacroIteration { int iteration{}; double gap{}, step{}; bool operator==(const MacroIteration&) const = default; };
struct MacroAssignment {
    std::vector<std::vector<double>> classFlows; // [class][link], veh/h
    std::vector<double> load, time;              // [link]: pcu/h, and the time at that load
    // (sum_m sum_a v_ma t_a - sum_m sum_rs d_rs kappa_rs) / sum_m sum_a v_ma t_a, at the final load.
    double gap{}; int iterations{}; bool converged{};
    std::vector<MacroIteration> convergence; // one per gap computed: iterations + 1 entries
    bool cancelled{};                        // the observer stopped the run (never converged)
    bool operator==(const MacroAssignment&) const = default;
};
// Starts from all-or-nothing at free-flow times. Each iteration: times at the current load, the
// all-or-nothing target, the gap; stop at gap <= target (converged) or at maxIterations steps (not
// converged); else one step size, by 60 halvings of [0, 1] on the Beckmann derivative, moves every
// class toward its target. `observer`, when set, sees each entry as it is recorded; false stops the
// run there with `cancelled`.
MacroAssignment assignMacro(const MacroGraph&, const std::vector<MacroClass>&, const MacroSettings&,
                            const std::function<bool(const MacroIteration&)>& observer = {});
}
