#include "red_running.hpp"
#include "detail.hpp"
#include <algorithm>
#include <cmath>

namespace trafficsim {
namespace {
constexpr double kMoving = 0.1; // m/s: kStoppedSpeed (conflicts.cpp); slower is standing
}
bool redRunner(std::uint32_t seed, std::uint64_t vehicleId, double share) {
    const auto mix = detail::splitmix64Mix;
    const std::uint64_t base = mix(static_cast<std::uint64_t>(seed)) ^ (vehicleId * 0x9E3779B97F4A7C15ULL);
    return static_cast<double>(mix(base ^ 15) >> 11) * 0x1p-53 < share;
}
double redElapsed(const SignalProgram& program, double time) {
    // The same position arithmetic as signalColorAt, so the two never disagree about the phase.
    double cycle = 0;
    for (const auto& phase : program.phases) cycle += phase.duration;
    double position = std::fmod(std::fmod(time + program.offset, cycle) + cycle, cycle);
    if (cycle - position < 1e-9) position = 0;
    std::size_t k = 0;
    while (k + 1 < program.phases.size() && position >= program.phases[k].duration - 1e-9) position -= program.phases[k++].duration;
    if (std::all_of(program.phases.begin(), program.phases.end(), [](const auto& p) { return p.color == SignalColor::red; }))
        return std::numeric_limits<double>::infinity();
    double elapsed = position;
    for (std::size_t j = (k + program.phases.size() - 1) % program.phases.size(); program.phases[j].color == SignalColor::red;
         j = (j + program.phases.size() - 1) % program.phases.size())
        elapsed += program.phases[j].duration;
    return elapsed;
}
bool runsRed(const DriverBehaviour& b, std::uint32_t seed, const Vehicle& vehicle, const SignalProgram& program, double time) {
    return b.redRunningShare && b.redRunningTime && vehicle.speed >= kMoving && redRunner(seed, vehicle.id, *b.redRunningShare) &&
           redElapsed(program, time) < *b.redRunningTime;
}
bool carriesRedRunning(const Scenario& s) {
    return std::any_of(s.behaviours.begin(), s.behaviours.end(), [](const auto& b) { return b.redRunningShare.has_value(); });
}
void reportRedRun(const RedRun& run, const Scenario& s, double time, std::uint64_t vehicleId, double move, std::vector<SimEvent>& events) {
    if (std::isfinite(run.gap) && move > run.gap) events.emplace_back(RedRunningEvent{time, vehicleId, s.signalHeads[run.head].id});
}
std::vector<ValidationIssue> redRunningIssues(const DriverBehaviour& b, const std::string& path) {
    std::vector<ValidationIssue> issues;
    const auto in = [](const std::optional<double>& v, double high) { return std::isfinite(*v) && *v > 0 && *v <= high; };
    if (b.redRunningShare && !in(b.redRunningShare, 1)) issues.push_back({"INVALID_BEHAVIOUR_PARAMETER", path + ".redRunningShare"});
    if (b.redRunningTime && !in(b.redRunningTime, 30)) issues.push_back({"INVALID_BEHAVIOUR_PARAMETER", path + ".redRunningTime"});
    if (b.redRunningShare && !b.redRunningTime) issues.push_back({"INCOMPLETE_RED_RUNNING", path + ".redRunningTime"});
    if (!b.redRunningShare && b.redRunningTime) issues.push_back({"INCOMPLETE_RED_RUNNING", path + ".redRunningShare"});
    return issues;
}
}
