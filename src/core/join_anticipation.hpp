#pragma once
#include "following.hpp"
#include "types.hpp"
#include <vector>

namespace trafficsim {
// JOIN_ANTICIPATION §2 (A4, D255), on only with `clampRules.joinAnticipation`. A vehicle within
// 50 m of a join follows, as a second law, the other stream's vehicle that reaches the join first.
// One entry per vehicle and join ahead within reach, in vehicle order: `d` is the front's distance
// to the join, `onPred` whether the front is already on the join's predecessor on its route.
struct JoinEntry { std::size_t join{}, pred{}, vehicle{}; double d{}; bool onPred{}; };
// Fills `joinOfSegment` (a segment with two or more predecessors) and `yieldSegmentOfRule`.
void indexJoins(const Scenario& scenario, ScenarioIndex& index);
std::vector<JoinEntry> joinEntries(const ScenarioIndex& index, const std::vector<Vehicle>& vehicles, const std::vector<VehicleRefs>& refs);
// Lowers `following` to the law against the nearest such vehicle when that is stricter (strict <).
void anticipateJoin(FollowingResult& following, const Scenario& scenario, const ScenarioIndex& index, const std::vector<Vehicle>& vehicles,
                    const std::vector<VehicleRefs>& refs, const std::vector<JoinEntry>& entries, std::size_t v,
                    const VehicleType& type, const DriverBehaviour& behaviour);
}
