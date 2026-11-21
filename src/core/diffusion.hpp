#pragma once
#include "types.hpp"

namespace multra {
// DIFFUSION (D613, as Vissim's waiting time before diffusion). A vehicle whose behaviour has
// diffusionTime and that has waited at its stub's dead end (waitingAtDeadEnd) for that long, in whole
// ticks, is removed from the run with a DiffusedEvent. A tick out of the wait -- a lane change this
// tick, a lateral move, another route than the snapshot's -- restarts the count. Runs after this
// tick's lane changes and missed exits; returns whether any vehicle was removed. Never called unless
// index.diffusions.
bool applyDiffusion(const Scenario&, const ScenarioIndex&, std::vector<Vehicle>&, const std::vector<VehicleRefs>&,
                    std::uint64_t tick, double time, std::vector<SimEvent>&);
// (0, 3600] s when present.
std::vector<ValidationIssue> diffusionIssues(const DriverBehaviour&, const std::string& path);
}
