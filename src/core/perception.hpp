#pragma once
#include "following.hpp"
#include "routes.hpp"
#include "types.hpp"
#include <string>
#include <vector>

namespace trafficsim {
// M3.5 perception keys (docs/reference/PERCEPTION.md §6, D179/D181): the one list the codec, the
// validator and the serializer read, so a key name exists once (hard rule 3).
struct PerceptionKey {
    const char* name;
    std::optional<double> PerceptionParameters::* member;
    bool integer; // written and read as a JSON integer
};
const std::vector<PerceptionKey>& perceptionParameterKeys();
// §6 ranges, finiteness and integrality of present keys: INVALID_BEHAVIOUR_PARAMETER at
// `<path>.<key>`. Absent keys are always valid.
std::vector<ValidationIssue> perceptionParameterIssues(const PerceptionParameters& parameters, const std::string& path);
bool hasPerception(const PerceptionParameters& parameters);

// §5: nearest tick, with absent/zero meaning the original, FIFO-free motion path.
std::size_t reactionTicks(const PerceptionParameters& parameters, double dt);
// Resize at insertion or publish: drop oldest entries when shrinking, pad the old end with
// copies of the oldest when growing (zero/free/no-state for an empty history).
void resizeReactionHistory(Vehicle& vehicle, std::size_t ticks);
// Read the oldest entry after that resize WITHOUT changing the snapshot. Reapply the current
// type bounds and D105 against the current leader; every physical cap still runs afterwards.
// Requires ticks > 0; the zero-delay caller keeps its current decision instead. An empty FIFO
// (arriving from a zero-delay road, D186) applies `decided`, this tick's undelayed decision.
FollowingResult delayedAction(const Vehicle& vehicle, std::size_t ticks, const FollowingResult& decided,
    const VehicleType& type, const DriverBehaviour& behaviour, std::optional<Leader> currentLeader);
// Publish only: resize, consume the oldest action and append this tick's undelayed decision.
// An empty FIFO with ticks > 0 is filled with copies of that decision (D186).
void storeReactionAction(Vehicle& vehicle, std::size_t ticks, const FollowingResult& decided);

// §2: the perceived leaders on the vehicle's own route -- up to `count` distinct vehicles ahead
// whose rear gap is at most `range`, nearest first. Equal gaps keep scan order, exactly as
// closestVehicle's strict < does, so count 1 with an infinite range returns its leader. Only the
// acceleration reads these; the hard cap and admission keep the physical leader.
struct PerceivedLeader { Leader leader; std::uint64_t vehicleId{}; };
struct LateralFilter; // lateral.hpp; LATERAL §6 filters the scan when the switch is on (D188)
std::vector<PerceivedLeader> perceivedLeaders(const Vehicle& vehicle, const std::vector<RoutePart>& parts,
    const std::vector<OccupiedSpan>& spans, const SpanBuckets& buckets, std::size_t count, double range,
    const LateralFilter* lateral = nullptr);
// One sighting into a nearest-first list of at most `count` distinct vehicles: a vehicle seen twice
// keeps its smaller gap, and an equal gap goes after the ones already there (§2's tie rule).
void notePerceived(std::vector<PerceivedLeader>& found, std::size_t count, const PerceivedLeader& seen);
// Either forward key is set: the acceleration reads perceived leaders (§2-§4).
bool perceivesAhead(const PerceptionParameters& parameters);
std::size_t observedCount(const PerceptionParameters& parameters);
double lookAheadRange(const PerceptionParameters& parameters);
// §3: lowers `following` (already computed against perceived[0]) to the smallest acceleration
// against each further perceived leader at its gap less the room the vehicles in between need at
// rest; the nearer leader is kept on a tie (strict <).
void anticipate(FollowingResult& following, const Scenario& scenario, const std::vector<Vehicle>& vehicles,
    const std::vector<VehicleRefs>& refs, const Vehicle& vehicle, const VehicleType& type,
    const DriverBehaviour& behaviour, const std::vector<PerceivedLeader>& perceived);
}
