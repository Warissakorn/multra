#pragma once
#include "routes.hpp"
#include "types.hpp"
#include <string>
#include <vector>

namespace trafficsim {
// M3.7 lateral keys (docs/reference/LATERAL.md §9, D180/D187): the one list the codec, the
// validator and the serializer read, so a key name exists once (hard rule 3).
struct LateralKey {
    const char* name;
    std::optional<double> LateralParameters::* member;
};
const std::vector<LateralKey>& lateralParameterKeys();
// Out-of-range values at `path.key`; a partial set at the path of its first missing key. Both
// INVALID_BEHAVIOUR_PARAMETER.
std::vector<ValidationIssue> lateralParameterIssues(const LateralParameters& parameters, const std::string& path);
bool hasLateral(const LateralParameters& parameters);
// §8: the switch. True iff some behaviour carries a lateral key.
bool lateralSwitch(const std::vector<DriverBehaviour>& behaviours);
// §3: compiled cross-sections are either absent or one per segment with finite values and
// non-negative widths, and `drivingSide` is present exactly when they are.
std::vector<ValidationIssue> lateralScenarioIssues(const Scenario& scenario);

// --- Slice 2 (LATERAL §5-§6, D188): spans and predicates. Nothing here runs with the switch off.
// A numerical tolerance, not behaviour: a body must cover more than this of another lane to occupy it.
inline constexpr double kLateralOverlap = 0.01;
// §6 dLat(v): interpolated from standstill to 50 km/h (13.8889 m/s), constant above. A behaviour
// without the keys keeps 0 when another behaviour switches the layer on.
double lateralDistance(const LateralParameters& parameters, double speed);
// A lateral interval in one carriageway's frame: right <= left, y positive to the left of travel.
struct LateralBand { std::size_t carriageway{}; double right{}, left{}; };
// A body of `width` at `lateral` from the lane centre, widened by `widen` on each side.
LateralBand bodyBand(const SegmentCrossSection& section, double width, double lateral, double widen = 0);
// Exact touching counts as overlapping (§6).
bool bandsOverlap(double rightA, double leftA, double rightB, double leftB);
// A position on segment `from` (length fromLength) at the same reference station on `to`.
double mapStation(const SegmentCrossSection& from, double fromLength, const SegmentCrossSection& to,
                  double toLength, double position);
LateralIndex buildLateralIndex(const Scenario& scenario);
// §5: spans[first..] are one vehicle's own spans. Give them its band, then append one span per other
// lane its body covers by more than kLateralOverlap (own = false), in segment-index order.
void appendLateralSpans(std::vector<OccupiedSpan>& spans, std::size_t first, const Scenario& scenario,
                        const LateralIndex& index, double width, double lateral);
// §6: the follower's band on its front segment, widened by dLat. A span on that carriageway counts
// as a leader (or as alongside) only if it overlaps; spans on other carriageways always count.
struct LateralFilter { const Scenario* scenario{}; LateralBand band; bool on{}; };
LateralFilter lateralFilter(const Scenario& scenario, std::size_t frontSegment, double width,
                            double lateral, const DriverBehaviour& behaviour, double speed);
bool lateralCounts(const LateralFilter* filter, const OccupiedSpan& span);
// The filter for a body whose front is at `front` on `parts`; off (counting everything) without the switch.
LateralFilter followerFilter(const Scenario& scenario, const ScenarioIndex& index,
    const std::vector<RoutePart>& parts, double front, double width, double lateral,
    const DriverBehaviour& behaviour, double speed);
}
