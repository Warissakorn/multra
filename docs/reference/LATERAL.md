# Lateral contract: two-dimensional occupancy (M3.7.0)

Designed 2026-10-15 (D180). **Slice 1 is implemented**: the §9 keys (schema 33), the §3
cross-sections and `drivingSide`, compiled only with the switch on (D187,
[evidence](../evidence/lateral-codec.md)). Nothing in the tick reads them yet. Rows BA39–BA46 are in
[the delivery plan](../plans/DRIVING_BEHAVIOUR.md#3-failure-first-acceptance-rows); BA45 has
automated evidence, BA39 is kept in every slice, and BA40–BA44 and BA46 are pending. The [driving-behaviour review](../audits/DRIVING_BEHAVIOUR_REVIEW.md)
§3 H and §4.2 motivate it, and D177/D178 order it second. Three later pieces build on it:
M3.4's lane-change duration, M3.7's lateral behaviour and M3.8's motorcycles.
[PERCEPTION](PERCEPTION.md) owns what a driver observes, [SIMULATION](SIMULATION.md) owns the tick,
and this file owns **where a vehicle is across the road and what it occupies there**.

## 1. Scope and sources

In scope:
- the lateral frame and the cross-section data compiled into core;
- a vehicle's lateral state;
- spans with lateral extent, including occupancy of two lanes at once;
- the leader, alongside and cap predicates in two dimensions;
- the admission rule for a lateral move;
- the switch that keeps lane-based runs identical;
- the display mapping.

Later, each its own contract or slice:
- desired lateral position at free flow and overtaking inside a lane (M3.7.1+);
- the lane-change manoeuvre that sets a lateral target (M3.4);
- filtering, lane sharing and forming at the stop line (M3.8);
- swept-body clearance on Connectors;
- lateral perception (PERCEPTION is longitudinal);
- lateral dynamics beyond a speed limit.

Written from public vocabulary only: Vissim's *desired position at free flow*, *overtake on same
lane* and *minimum lateral distance at 0 and 50 km/h*, meaning names, not values or internals. No
equivalence is claimed. Rule 4 applies, and the not-yet-validated marker stays.

## 2. What core knows today

| Fact | Where |
|---|---|
| A segment is `{id, length, next}`, with no width and no neighbour | `src/core/types.hpp:13` |
| Segments are **lane sections** (one per lane between section cuts) and **Connector lane paths** | `buildScenario`, `src/model/network/compile.cpp:62-66`; `LaneSection`, `src/model/network/network.hpp:291` |
| A span is longitudinal: `{vehicleId, segmentIndex, rear, front, speed, acceleration}` | `src/core/routes.hpp:9` |
| Lanes relate only through compiled `LaneChangeSpan`s | `types.hpp:176` |
| Lane width, lane order, `laneOffset` and the driving side exist only in the model; the lane-centre offset is one expression in `laneGeometry` | `src/model/network/geometry.cpp:89-99` |
| A vehicle type's `width` is display only | `types.hpp:72` |

So the engine is lane-based, and a vehicle occupies exactly the lane its route names.

## 3. Frame and compiled cross-section

- **Lateral coordinate `y`** is in metres, **positive to the left of the direction of travel**,
  measured from the carriageway's reference line.
- **Carriageway.** Every lane section of one Link shares one carriageway, with station
  `s = linkStationStart + (position on the section)`. Each Connector lane path is its own
  carriageway.
- **Compiled data.** `Scenario` gains `segmentCrossSections`, parallel to `segments`:

  ```
  SegmentCrossSection { carriageway (index), linkStationStart, centre, startWidth, endWidth }
  ```

  For a lane section, `centre` is the lane-centre offset that `laneGeometry` already computes.
  The compiler and `laneGeometry` call **one shared function** for it, so the drawing and the
  physics cannot disagree (hard rule 3): `laneCentreOffset` in `geometry.cpp`. On a lane section both
  widths are the lane's own width. For a Connector path, `centre = 0`, and the width varies linearly
  along the path from `startWidth` to `endWidth`, the two ends `connectorLaneWidths` already gives
  (0 at a surplus lane's tapered end). Two numbers, because one cannot vary (D187).
  Carriageway indices are the Link's position in the Network, then `links + path index` for
  Connector paths; only their equality is meaningful. `createSimulation` re-sorts segments by id
  and carries each cross-section with its segment.
- **Driving side.** `Scenario` gains `drivingSide`, compiled once from the Network. D178's
  keep-left default and M3.8 read it, and nothing re-derives it. The `DrivingSide` enum itself
  moved into core (`lateral_types.hpp`); the model's Network uses the same type (D187).
- These fields are compiled **only when the switch (§8) is on**. Otherwise they are empty, and a
  compiled Scenario compares equal to today's. `validateScenario` refuses a compiled table that is
  not one per segment, a non-finite station or centre, a negative or non-finite width, or a
  `drivingSide` present without the table or absent with it (`INVALID_CROSS_SECTION`).

## 4. Vehicle state

- `Vehicle` gains `lateral`, the offset in metres of the body centre from **its current lane's
  centre**, and `lateralSpeed` in m/s. Both are 0 by default, stored in `SimState` (exact replay)
  and written only at publish, in vehicle-id order.
- The **body** in the carriageway frame is
  `[rear, front] × [centre + lateral − width/2, centre + lateral + width/2]`, using the type's
  existing `width`. That width becomes physical only when the switch is on.
- The route still names one lane chain. Longitudinal motion, routing, signals, priority rules,
  zones and measurements stay keyed by the route. Lateral position adds occupancy, and it never
  changes which route a vehicle follows, except through §7's crossing rule.

## 5. Spans: occupying every lane the body covers

- `OccupiedSpan` gains `left, right`, the body's lateral extent in its carriageway frame.
- A vehicle emits a span on **every lane-section segment its body overlaps by more than
  `kLateralOverlap = 0.01 m`**, not only on its route's segment. A changer between lanes, a car
  pushed toward a lane line or a motorcycle riding on one therefore appears on both lanes.
  The existing per-lane machinery then sees it with **no new code path**: `closestVehicle`'s leader
  search, `around`, zone occupancy (`summarizeZones`, `src/core/conflicts.cpp:65`) and admission.
- `appendVehicleSpans` (`src/core/routes.cpp:186`) stays the single producer of spans. For one
  vehicle it emits the own-lane spans first, then the other lanes' spans in segment-index order, so
  the bucket order that tie-breaks rely on is defined (SpanBuckets, `routes.hpp`).
- On a Connector path, a body never spans another path in this contract. Paths that overlap
  geometrically are handled by conflict zones, as today.
- `kLateralOverlap` is a numerical tolerance, not behaviour. It is a named constant in core, the
  only one this contract adds.

## 6. Predicates in two dimensions

Define the speed-dependent **minimum lateral distance** of the follower's behaviour:

```
dLat(v) = lateralMinDistanceStanding
        + (lateralMinDistanceAt50 − lateralMinDistanceStanding) · min(v, 13.8889) / 13.8889
```

Here 13.8889 m/s is 50 km/h. Below 50 km/h the value is interpolated; at or above it, the value is
`lateralMinDistanceAt50`.

- **Leader.** A span on the follower's segment counts as a leader only if its `[left, right]`
  overlaps the follower's body widened by `dLat(v)` on each side. Exact touching counts as
  overlapping. Two motorcycles side by side in one lane, with clearance above `dLat`, are not each
  other's leaders. This applies to the physical leader and to PERCEPTION's perceived leaders alike.
- **Alongside** (`around`, `src/core/lanes.cpp:46`) means longitudinal overlap together with the
  same lateral test. It still refuses a lane change, whatever the perception keys.
- **Non-overlap.** The hard cap (`allowedDistance`, `src/core/simulation.cpp:264`) and its counted
  clamps apply to the overlapping physical leader. Together with §7's admission rule, no two bodies
  ever overlap in two dimensions at a published tick.
- **Zones and priority rules** stay segment-based. Any span on a zone's segment occupies it, so a
  straddling vehicle blocks both lanes' zones. This is conservative and stated.

## 7. Lateral motion and its admission

- Each tick, a vehicle may have a **lateral target** `yT` (relative to its lane centre). This
  contract sets none. M3.4's lane change and M3.7.1's desired position are the first writers.
  Without a target, `lateral` stays where it is.
- The proposed move is `Δ = clamp(yT − lateral, ±maxLateralSpeed · dt)`, and the body never
  crosses the carriageway's outer edges.
- **Admission.** After the longitudinal move, proposed lateral moves are resolved in **vehicle-id
  order**, as phase 2 (`resolveRequests`) does. A move is admitted only if the new body overlaps
  no published body of this tick and no move admitted before it. A refused move keeps the previous
  `lateral`, and its `lateralSpeed` is 0.
- **Crossing.** When the body's centre crosses the boundary into the adjacent lane and a
  `LaneChangeSpan` covers that station, the route remaps exactly as today's instantaneous change
  does (`simulation.cpp:212-228`), and `lateral` is re-expressed from the new lane's centre. Without
  a span, `yT` is clamped so the centre stays inside the current lane.
- Lateral speed is a capped rate, not an acceleration-limited state, in this version. That is a
  stated simplification for M6 to test.

## 8. The switch and identity

- `ScenarioIndex::lateral` is true iff some behaviour carries a §9 key, or a later slice's type
  key does (M3.8).
- **When it is false**, nothing in §3–§7 runs. Spans carry no lateral extent and nothing compares
  one. The Scenario equals today's, and the CLI seed 42 pin, the frozen baselines, and the
  prototype, W74 and perception runs are byte-identical (BA39).
- **When it is true**, every vehicle takes part. Cars start centred (`lateral = 0`). Results may
  then differ from the lane-based run, because widths now decide what overlaps. That difference is
  recorded, not hidden.

## 9. Parameters (hard rule 5: data, never code defaults)

All are optional behaviour keys, on both models. Absent means the switch is off.

| Key | Range | Meaning |
|---|---|---|
| `lateralMinDistanceStanding` | ≥ 0 m, finite | `dLat` at standstill |
| `lateralMinDistanceAt50` | ≥ 0 m, finite | `dLat` at 50 km/h and above |
| `maxLateralSpeed` | > 0 and ≤ 3 m/s | Lateral rate cap |

A behaviour carrying any one of them must carry all three (`INVALID_BEHAVIOUR_PARAMETER`, with the
path to the first missing key). The codec follows D136/D179: one key table read by the parser,
the serializer and the range check. The schema is the next free number at implementation time,
written only when used. Older files keep their bytes, and an older schema that carries a key is
refused. No values are shipped. A preset follows D178's rule: cited standard values, flagged
uncertain.

## 10. Consumers

| Consumer | Change when the switch is on |
|---|---|
| `closestVehicle` (`simulation.cpp:35`) and PERCEPTION's search | Lateral filter (§6); spans of straddling vehicles appear on both lanes |
| `around` / `safeAt` (`lanes.cpp:46`, `:83`) | Lateral filter; alongside per §6 |
| `courtesyHolds` (`lanes.cpp:324`) | Unchanged logic over the new spans |
| Zones (`summarizeZones`, `zoneHold`, `resolveRequests`, `src/core/conflicts.cpp`) | Any span on a zone segment occupies it (§6) |
| D108 source insertion (`simulation.cpp:160`) | A new vehicle starts centred, and its first span is checked in two dimensions |
| Lateral admission (new, §7) | After the longitudinal step, in vehicle-id order |
| Editor pose (`src/editor/vehicle_pose.hpp`, `src/editor/lane_change_pose.cpp`) | Draws the body at lane centre + `lateral`. Once M3.4 lands, the display slide is replaced by the engine's own lateral path. Display never feeds physics |
| Evaluation (queue counters, travel-time sections) | A straddling vehicle counts on its route's lane only (stated) |

## 11. Performance budget

A body spans at most two lane sections in this contract, because widths are at most a lane apart.
The lateral filter is one interval test per candidate. Lateral admission is `O(n · k)` for `k`
candidate neighbours from the buckets. The implementation slice re-runs
`trafficsim-engine-benchmark` before and after (benchmarks are not in `check`). A regression above
10 % with the switch on needs its own decision (D164). With the switch off, no change is expected,
and the slice demonstrates that.

## 12. Delivery order

One slice per session:

1. Compile and codec: cross-sections, `drivingSide`, the keys (BA45).
2. Spans and predicates (BA40–BA42).
3. Lateral motion and admission (BA43, BA44).
4. Display (BA46).

BA39 runs in every slice. M3.4's lane-change duration then sets `yT` across the boundary, and its
changer occupies both lanes through §5.
