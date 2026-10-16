# Lateral contract: two-dimensional occupancy (M3.7.0)

Designed 2026-10-15 (D180). **Slices 1–4 are implemented**: the §9 keys (schema 33), the §3
cross-sections and `drivingSide` (D187, [evidence](../evidence/lateral-codec.md)), the §5–§6
spans and predicates (D188, [evidence](../evidence/lateral-spans.md)), §7's motion, admission
and crossing (D189, [evidence](../evidence/lateral-motion.md)), zone occupancy by span (D202,
[evidence](../evidence/lateral-zones.md)) and the display (D203, [evidence](../evidence/lateral-display.md)). Only a test sets a lateral target
yet, so every product run still keeps `lateral` at 0. Rows BA39–BA47 are in
[the delivery plan](../plans/DRIVING_BEHAVIOUR.md#3-failure-first-acceptance-rows); BA40–BA47 have
automated evidence and BA39 is kept in every slice. The [driving-behaviour review](../audits/DRIVING_BEHAVIOUR_REVIEW.md)
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
- **Carriageway.** Every lane section of one Link shares one carriageway. Its shared station is the
  Link's **reference line**: lanes are offset polylines, so on a bend their own metres differ and
  `linkStationStart + position` names different cross-sections on different lanes. A position maps
  linearly between the section's `referenceStart`/`referenceEnd`, the same approximation lane
  changes make (`lane_family.cpp`, `mappedOnto`), exact on straight Links (D188). Each Connector lane
  path is its own carriageway.
- **Compiled data.** `Scenario` gains `segmentCrossSections`, parallel to `segments`:

  ```
  SegmentCrossSection { carriageway (index), linkStationStart, centre, startWidth, endWidth,
                        referenceStart, referenceEnd }
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
- `Vehicle::lateralTarget` is §7's `yT`, also from the current lane's centre and stored in
  `SimState`. Absent by default; no product code writes it yet, only tests (D189).
- The **body** in the carriageway frame is
  `[rear, front] × [centre + lateral − width/2, centre + lateral + width/2]`, using the type's
  existing `width`. That width becomes physical only when the switch is on.
- The route still names one lane chain. Longitudinal motion, routing, signals, priority rules,
  zones and measurements stay keyed by the route. Lateral position adds occupancy, and it never
  changes which route a vehicle follows, except through §7's crossing rule.

## 5. Spans: occupying every lane the body covers

- `OccupiedSpan` gains `right, left`, the body's lateral extent in its carriageway frame, and `own`,
  false on a span emitted for another lane (D188).
- A vehicle emits a span on **every lane-section segment its body overlaps by more than
  `kLateralOverlap = 0.01 m`**, not only on its route's segment. A changer between lanes, a car
  pushed toward a lane line or a motorcycle riding on one therefore appears on both lanes.
  The span-reading machinery then sees it with **no new code path**: `closestVehicle`'s leader
  search, PERCEPTION's search, `around`, D108 admission and the priority-rule give-way scan. **Zones
  work from route positions**, so slice 3b (D202) adds their own span scan: see §6. `courtesyHolds` ignores
  `own = false` spans, because its hold is applied on the holder's own route.
- The other lane's span is the own span's `[rear, front]` mapped through the reference line (§3) and
  clamped to that segment; a body covering several own sections yields one merged span per lane.
- `appendVehicleSpans` (`src/core/routes.cpp`) stays the single producer of spans; every indexed
  `occupiedSpans` goes through it. For one
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
`lateralMinDistanceAt50`. A behaviour without the keys, in a run another behaviour switched on, has
`dLat = 0` (D188).

- **Leader.** A span on the follower's carriageway (the one its front is on) counts as a leader only
  if its `[right, left]` overlaps the follower's body widened by `dLat(v)` on each side; spans on later
  carriageways along the route are not filtered (D188). Exact touching counts as
  overlapping. Two motorcycles side by side in one lane, with clearance above `dLat`, are not each
  other's leaders. This applies to the physical leader and to PERCEPTION's perceived leaders alike.
- **Alongside** (`around`, `src/core/lanes.cpp:46`) means longitudinal overlap together with the
  same lateral test. It still refuses a lane change, whatever the perception keys.
- **Non-overlap.** The hard cap (`allowedDistance`, `src/core/simulation.cpp:264`) and its counted
  clamps apply to the overlapping physical leader. Together with §7's admission rule, no two bodies
  ever overlap in two dimensions at a published tick.
- **Priority rules** read spans, so a straddler on a conflict segment holds the minor approach, without
  a lateral filter. This is conservative and stated.
- **Zones** (slice 3b, D202). An `own = false` span on a segment of a zone side's chain counts as the
  other side sees a body, with no lateral filter: inside a **major** area (front past `entry`, rear
  short of `exit`, in that segment's metres) it sets `majorInside` and `majorBlocks`; on the chain's
  first segment short of entry it is anticipated by the same headway/gapTime test as a route
  position; inside a **minor** area it joins `holders`, so major vehicles wait at entry; and in
  phase 2 it sweeps a major area when its front plus the tick's move passes entry while its rear is
  short of exit (the move mapped one to one, exact on straight Links). Stated limits: the straddler
  itself is **not held** at the other lane's zone, because its route has no incidence there (M3.4,
  the first product writer of targets, adds mapped incidence if it is needed); approach on segments
  upstream of the chain is not seen.

## 7. Lateral motion and its admission

- Each tick, a vehicle may have a **lateral target** `yT` (`lateralTarget`, relative to its lane
  centre). This contract sets none. M3.4's lane change and M3.7.1's desired position are the first
  writers; until then only tests write it (D189). Without a target, or for a behaviour without the
  keys, `lateral` stays where it is.
- The proposed move is `Δ = clamp(yT − lateral, ±maxLateralSpeed · dt)`, read at the cross-section
  of the vehicle's front. `yT` is first clamped so the body stays inside the carriageway's outer
  edges, the outermost lane edges among the front's section and the neighbour sections at that
  reference station (a Connector path: its own width there), and so the centre stays inside the
  current lane (on the boundary at most) on each side no span covers (below). An empty range moves nothing.
- **Admission.** Implemented in `moveLaterally` (`src/core/lateral_motion.cpp`), which runs after the
  tick's longitudinal step has published every body. Proposed moves are resolved in **vehicle-id
  order**, as phase 2 (`resolveRequests`) does. A move is admitted only if the new body overlaps
  no published body of this tick and no move admitted before it. Here overlapping means positive area
  in both directions: touching bodies may stand side by side, unlike §6's inclusive leader test
  (D189). A refused move keeps the previous `lateral`, and its `lateralSpeed` is 0. `lateralSpeed`
  is `Δ / dt` for an admitted move.
- **Crossing.** A span **covers** a side when it leaves the vehicle's route, its from-range holds the
  whole body (the test the instantaneous change already makes), and its target lane at the mapped
  front is the nearest lane on that side. When the body's centre is strictly past the boundary on a
  covered side, the route remaps through `recordLaneChange` (`lanes.cpp`), the bookkeeping today's
  instantaneous change uses, to the mapped distance; `lateral` and `yT` are re-expressed from the new
  lane's centre, so neither moves on the road. The vehicle's `MovedEvent` of that tick names the new lane
  (it reports where the tick ends), and the `LaneChangeEvent` follows it. Without a span, `yT` is
  clamped so the centre stays inside the current lane.
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
| `courtesyHolds` (`lanes.cpp:324`) | Unchanged logic; `own = false` spans are skipped (D188) |
| Priority rules (give-way scan in `stepSimulation`) | A straddler's span on the conflict segment holds the minor approach |
| Zones (`summarizeZones`, `resolveRequests`, `src/core/conflicts.cpp`) | A straddler's other-lane span blocks, is anticipated, holds and sweeps per §6 through `LateralIndex::zoneSides`; `zoneHold` reads that state and does not hold the straddler at the other lane's zone (D202) |
| `placeVehicle` (`retune.cpp`), Lab telemetry (`lab_run.cpp`), `dead_end_waits.cpp` | Stay one-dimensional mirrors: a placement next to a straddler is refused, and the two displays do not filter laterally (stated, D188) |
| D108 source insertion (`simulation.cpp:160`) | A new vehicle starts centred, and its first span is checked in two dimensions |
| Lateral admission (§7, `moveLaterally`) | After the longitudinal step, in vehicle-id order (D189) |
| Editor pose (`src/editor/canvas_run.cpp`, `vehicle_pose.hpp`, `lane_change_pose.cpp`) | Draws the body at lane centre + `lateral`, offset along the left normal of the body's heading (exact on straight lanes). A crossing made by §7's motion is flagged `lateral` in `LaneChangeTrace` and never slid; instantaneous changes keep the D97 slide until M3.4 replaces it with the engine's own lateral path. Display never feeds physics (D203) |
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
3. Lateral motion and admission (BA43, BA44; 3a, D189), then zone occupancy by span (BA47; 3b, D202).
4. Display (BA46; D203).

BA39 runs in every slice. M3.4's lane-change duration then sets `yT` across the boundary, and its
changer occupies both lanes through §5; its contract is [LANE_CHANGE_DURATION](LANE_CHANGE_DURATION.md) (D204).
