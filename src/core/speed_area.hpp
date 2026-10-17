#pragma once
// B-6 §7 (docs/reference/SPEED_DECISIONS.md, D225): reduced-speed areas at run time.
#include "types.hpp"

namespace trafficsim {
// §7.2: each route's areas by route distance, ascending (then by id), every type slot's range.
void indexSpeedAreas(const Scenario&, ScenarioIndex&);
// §7.2: the area bound on `route` this tick, applied to `target`: inside an area, its speed; ahead,
// sqrt(speed² + 2 b d) from where the front will be after this tick. `binds` when an area lowers the
// target; `deceleration` is then that area's b, else the type's comfortable one.
struct AreaBound { double target, deceleration; bool binds; };
AreaBound areaTarget(const ScenarioIndex&, const VehicleType&, std::uint32_t route, const Vehicle&, double target, double dt);
// §7.3: after the front moved from `from` to `to` on `route`, the speed in force from an area; on a
// change the finished free-flow piece closes at the boundary crossed, else at `from`.
void passSpeedAreas(const ScenarioIndex&, const VehicleType&, std::uint32_t route, double from, double to, Vehicle&);
std::vector<ValidationIssue> speedAreaIssues(const Scenario&);
}
