#pragma once
#include "types.hpp"
#include <limits>

namespace trafficsim {
// B-9 (docs/reference/RED_RUNNING.md, D237/D238): a share of drivers may pass a red head in its
// first seconds while still moving. Absent keys: nothing here runs and the step is unchanged.
// The nearest head a vehicle was let through this tick, so the step can report it passing the line.
struct RedRun { double gap{std::numeric_limits<double>::infinity()}; std::size_t head{}; };
// W74's trait hash (W74 §5) at index 15: never drawn from the run's random stream.
bool redRunner(std::uint32_t seed, std::uint64_t vehicleId, double share);
// Seconds since the program last turned red; consecutive red phases are one red. Infinite when the
// program is red throughout, so such a red never "began".
double redElapsed(const SignalProgram&, double time);
// §3: a runner of this behaviour, inside its window, moving, at a head that is red at `time`.
bool runsRed(const DriverBehaviour&, std::uint32_t seed, const Vehicle&, const SignalProgram&, double time);
bool carriesRedRunning(const Scenario&);
// The event, when this tick's move takes the front past the line the vehicle was let through.
void reportRedRun(const RedRun&, const Scenario&, double time, std::uint64_t vehicleId, double move, std::vector<SimEvent>&);
std::vector<ValidationIssue> redRunningIssues(const DriverBehaviour&, const std::string& path);
}
