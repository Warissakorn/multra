#pragma once
#include "demand.hpp"
#include "link_model.hpp"
#include <functional>

// M11.3c loading engine (docs/reference/MESO_NETWORK.md §4-§6, D298): Yperman's (2007) LTM solution
// algorithm (§3.2) with the Tampère et al. (2011) node model (D297). Per step, from the state at t
// only: every link's sending and receiving volume (§4.6); each in-link's sending volume split by the
// routes of the vehicles at its exit (Eq. 5.2); origins as point queues (Eq. 5.4) and destinations
// receiving without limit (Eq. 5.5); mesoNodeFlows at every node; then the counts (Step 3). A
// commodity is one class's path, counted in PCU. Signals are averaged green, internal node supplies
// (Tampère §5.2, D299), one per in-link and program. The step must
// meet mesoStepLimit; the compile refuses it otherwise. Deterministic: there is no seed.
namespace multra {
// One class's path through the run, vehicles at k * step.
struct MesoCommodityRun {
    std::size_t cls{}, path{};
    std::vector<double> demand, entered, arrived;
};
struct MesoLoading {
    double step{};
    // Per link, at k * step (PCU): the cumulative counts at its entrance and exit, and the sending and
    // receiving volumes of step [k, k + 1] (the boundary interface of §9; the last entry is 0).
    std::vector<MesoCurve> up, down;
    std::vector<std::vector<double>> sending, receiving;
    std::vector<std::vector<double>> turned; // per turn, cumulative PCU at k * step
    std::vector<MesoCommodityRun> commodities;
    std::vector<std::pair<std::size_t, std::size_t>> refused; // (class, path) not loaded: no PCU, a link twice, or turns not joining
    bool cancelled{};                    // progress returned false; the curves end at the last whole step
};
// Loads the classes' paths over [0, horizon], s, in steps of `step` s. `progress(k, steps)` is called
// before each step k (M11.4, D300); false stops the run there.
MesoLoading loadMeso(const MesoGraph&, const std::vector<MesoClass>&, double step, double horizon,
                     const std::function<bool(std::size_t, std::size_t)>& progress = {});
}
