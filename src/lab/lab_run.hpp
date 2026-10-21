#pragma once
// D165 (docs/reference/BEHAVIOUR_LAB.md §2): one lab session's run. It compiles the lab ring, marks
// its routes circuits, places the vehicles at t = 0 and steps; a settings change retunes the run in
// place when the compiled shape is unchanged and restarts it otherwise.
#include "lab_network.hpp"
#include "../core/simulation.hpp"
#include <filesystem>
#include <optional>

namespace multra::lab {
// What the focus vehicle sees this tick. `gap` is front bumper to the leader's rear, metres.
struct FocusTelemetry {
    std::uint64_t id{};
    double time{}, speed{}, acceleration{}, desiredSpeed{}, distance{};
    FollowingMode mode{};
    std::optional<double> gap, leaderSpeed;
    std::uint64_t laps{};
};
class LabRun {
public:
    enum class Applied { unchanged, retuned, restarted };
    LabRun(LabSettings settings, std::filesystem::path dataDirectory, std::uint32_t seed = 42);
    // Retune when only values changed (focus/traffic parameters, signal timings); restart from t = 0
    // when the road, the vehicle count or the heavy share changed. Throws, keeping the current run,
    // when the settings cannot be built or compiled.
    Applied apply(const LabSettings& settings);
    void restart();
    void restart(std::uint32_t seed);
    void step();
    const SimState& state() const { return state_; }
    const Network& network() const { return document_.network; }
    // The ring as a document, for a view that draws roads from one; never in any History.
    const ProjectDocument& document() const { return document_; }
    const LabSettings& settings() const { return settings_; }
    std::uint32_t seed() const { return seed_; }
    std::uint64_t focusId() const { return focusId_; }
    // Bumped whenever the network is replaced, so a view knows to rebuild its geometry.
    std::uint64_t networkRevision() const { return networkRevision_; }
    std::uint64_t safetyClamps() const { return clamps_; }
    std::optional<FocusTelemetry> telemetry() const { return telemetry(focusId_); }
    std::optional<FocusTelemetry> telemetry(std::uint64_t vehicleId) const;
private:
    // `loops`: the authored Route ids, the tuned vehicle's first; `origins`: each compiled chain's loop.
    struct Compiled { ProjectDocument document; Scenario scenario; std::vector<std::string> loops;
                      CompiledRouteOrigins origins; };
    Compiled compile(const LabSettings& settings) const;
    void start(Compiled compiled, const LabSettings& settings);
    LabSettings settings_;
    std::filesystem::path data_;
    std::uint32_t seed_{};
    ProjectDocument document_;
    SimState state_;
    std::uint64_t focusId_{}, networkRevision_{}, clamps_{}, laps_{};
    double lastFocusDistance_{};
};
// The most vehicles (focus included) one lane of this ring holds at t = 0 with a body length and
// the standstill distance between them.
int maxVehicles(const LabSettings& settings);
}
