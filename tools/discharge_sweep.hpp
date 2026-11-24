// The queue-discharge timestep sweep shared by the W74 (BA27) and W99 (BA231) records: the four-leg
// project at dt 0.1 / 0.25 / 0.5, the prototype arm and one model arm, seeds 42-81. Sensitivity is
// recorded, never bounded; nothing here is calibration (rule 4). The discharge spec is the CLI default.
#pragma once
#include "discharge_options.hpp"
#include "w74_fixture_document.hpp"
#include "../src/project/evaluation.hpp"
#include "../src/core/simulation.hpp"
#include "../src/project/run.hpp"
#include <iomanip>
#include <iostream>
#include <sstream>
namespace multra::sweep {
inline std::string figure(const std::optional<double>& v) {
    if (!v) return "";
    std::ostringstream out; out << std::setprecision(10) << *v; return out.str();
}
// Arms "prototype" (no behaviour added) and `model` (`behaviour` added, every vehicle type on it).
inline void discharge(const ProjectDocument& base, const std::filesystem::path& data, const std::string& model,
                      const DriverBehaviour& behaviour, const std::string& name, std::ostream& csv) {
    csv << "model,dt,seed,head,lane,go,complete,reason,samples,meanHeadway,vehiclesPerHour,startupLostTime,startupReason,runClamps\n";
    for (const std::string& arm : {std::string("prototype"), model})
        for (const double dt : {0.1, 0.25, 0.5}) {
            const auto d = w74fixture::arm(base, arm == model ? &behaviour : nullptr, dt, name);
            const auto snapshot = compileDocument(d, data);
            const auto spec = evaluationSpec(d, snapshot, data);
            const auto options = DischargeOptions{}.forDuration(snapshot.scenario.duration);
            for (std::uint32_t seed = 42; seed <= 81; ++seed) {
                DischargeAccumulator release(options, spec.queue);
                auto state = createSimulation(snapshot.scenario, seed);
                release.observe(state);
                std::size_t clamps = 0;
                while (state.tick < totalTicks(snapshot.scenario)) {
                    state = stepSimulation(std::move(state));
                    release.observe(state);
                    for (const auto& e : state.events) clamps += std::holds_alternative<SafetyClampEvent>(e);
                }
                for (const auto& cycle : release.report()) {
                    const auto e = estimateDischarge(cycle, options);
                    csv << arm << ',' << dt << ',' << seed << ',' << cycle.headId << ',' << cycle.laneId << ','
                        << std::setprecision(10) << cycle.go << ',' << cycle.complete << ',' << e.reason << ','
                        << e.samples << ',' << figure(e.meanHeadway) << ',' << figure(e.dischargeVehiclesPerHour) << ','
                        << figure(e.startupLostTime) << ',' << e.startupUnavailableReason << ',' << clamps << '\n';
                }
                std::cerr << arm << " dt " << dt << " seed " << seed << " clamps " << clamps << '\n';
            }
        }
}
}
