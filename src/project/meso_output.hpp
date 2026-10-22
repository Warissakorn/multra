#pragma once
#include "meso_run.hpp"
#include <optional>

namespace multra {
// M11.4 (docs/reference/MESO_NETWORK.md §7, D300): a Meso run's outputs, in PCU (capacity is shared
// in PCU, D272), times in seconds, by the run's reporting slice.
// Per link and slice [t0, t1): inflow and outflow (pcu/h); mean density over the slice's steps
// (pcu/km); the mean travel time of the PCU entering in the slice, from the cumulative curves'
// horizontal distance (null when none entered or not all have left by the end); the mean vertical
// queue up(t - L/v) - down(t); spill-back when the receiving volume was 0 in a step of the slice.
// Per turn and slice: the PCU transferred. Turns have no delay: the LTM's nodes have no length.
// Totals over the PCU that completed each link: VKT (links only), VHT with the turns' free time
// (D301) and delay against free flow; PCU stored on links and inside nodes at the end; vehicles per
// class demanded, entered, arrived and waiting at their origins.
struct MesoLinkSlice {
    double start{}, end{}, inflow{}, outflow{}, density{}, queue{};
    std::optional<double> travelTime;
    bool spillBack{};
};
std::vector<MesoLinkSlice> mesoLinkSlices(const MesoRun&, std::size_t link);
// The mean time PCU numbered a to b spend between the entrance and exit curves, s; null if not all left.
std::optional<double> mesoTravelTime(const MesoCurve& up, const MesoCurve& down, double a, double b);
Json mesoJson(const MesoRun&);
std::string mesoCsv(const MesoRun&);
}
