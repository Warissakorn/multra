#pragma once
#include "run_identity.hpp"
#include "../core/types.hpp"
#include "../eval/discharge.hpp"
#include "../eval/movement.hpp"
#include "../eval/summary.hpp"
#include "../project/run.hpp"
#include <QElapsedTimer>
#include <QTimer>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace multra {
// The editor's single run (Run/Pause/Step/Reset, src/shell/editor_run.cpp), D517: its state, moved
// out of EditorWindow as the second step of its split (D505, after D514's BatchSession). The run
// functions and the results-page widgets stay on the window.
struct RunSession {
    RunIdentity identity;
    QTimer timer;
    QElapsedTimer elapsed;
    double credit{};
    std::optional<RunSnapshot> snapshot;
    SimState state;
    // Fed every step, exactly as the retired M0 window did: the clamp count is the only signal
    // that the prototype car-following is being pushed, so it has to be visible where the run is.
    SummaryAccumulator summary;
    // M2.5: per-movement delay and approach queues, fed the same states as `summary`.
    std::optional<MovementAccumulator> movements;
    // D144: queue discharge with the CLI's default spec, fed the same states; the reason it
    // could not be set up, if any, is shown instead of a table.
    std::optional<DischargeAccumulator> discharge;
    std::string dischargeError;
    // D144: one row per SafetyClampEvent, read off the published vehicle (blank once it left).
    struct ClampRow { double time{}; std::uint64_t vehicleId{}; std::string typeId, routeId, segmentId; };
    std::vector<ClampRow> clamps;
};
}
