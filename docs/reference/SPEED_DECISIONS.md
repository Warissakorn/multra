# Desired-speed decisions and reduced-speed areas (B-6)

Written 2026-10-17 (D222). §2–§6 (decisions) are implemented the same day (D223,
[evidence](../evidence/speed-decisions.md)). §7 (reduced-speed areas, D224) is implemented in D225
([evidence](../evidence/reduced-speed-areas.md)). This is
tier B-6 of [BEHAVIOUR_PARAMETERS](BEHAVIOUR_PARAMETERS.md), level 5 under D216: a property of a
place, not of a driver. It follows [SPEED_CONTROL](SPEED_CONTROL.md) (curve speed, M3.6). Rule 4
applies: nothing here is validated.

## 1. Scope and the owner's choices

Today only the vehicle type's desired-speed range limits speed. Nothing on the road can change it,
so a speed-limit change along a corridor cannot be modelled. Limits, and how far drivers exceed
them, differ by country.

The owner chose on 2026-10-17:
- **Vissim's model.** A *desired-speed decision* gives each vehicle type a new desired-speed range.
  A vehicle keeps its place in the range: a fast driver stays fast. Speeding is expressed by the
  range itself (for example 50–65 km/h behind a 50 km/h sign).
- **Delay against the speed in force.** Driving slower because of a sign is not delay. The curve
  bound still counts as delay (SPEED_CONTROL §5).
- **Order:** decisions first, complete with engine, file and editor placement. Reduced-speed areas
  are next (§7).

## 2. The object

`definition.speedDecisions` is an array; the object belongs to the definition, beside the routing
decisions, because it names vehicle types. Each entry has:
- `id` and an optional `name`;
- `linkId` and `station`: metres along the Link's reference line, `0 ≤ station ≤ length`. It applies
  across every lane, placed as a travel-time section line is.
- `speeds`: one or more `{vehicleTypeId, min, max}` in m/s, as type ranges are, with
  `0 < min ≤ max`, finite. Type ids must be known and must not repeat. A type not listed is
  unaffected.

Validation refuses each fault at its path:
- `UNKNOWN_SPEED_DECISION_LINK` for an unknown Link;
- `INVALID_POSITION` for a station off the Link;
- `UNKNOWN_VEHICLE_TYPE` and `DUPLICATE_ID` for the type ids;
- `INVALID_RANGE` for the speeds, or for an empty list;
- `INVALID_ID` and `DUPLICATE_ID` for the decision's own id.

**Schema 38.** The array is written only when non-empty. An older schema carrying it is refused with
`EDIT_UNSUPPORTED_FIELD`.

## 3. The rule

- **Quantile.** Each vehicle keeps a quantile `q`, set once at generation:
  `q = (desired − typeMin) / (typeMax − typeMin)`, or 0.5 when the span is 0. This is the same mapping
  as D191's `rescaled`.
- **Passage.** When a vehicle's front crosses a decision on its current route within a tick (it was
  at or before `at` and is now beyond it: `from ≤ at < to`), and its type is listed, its
  `desiredSpeed` becomes `min + q · (max − min)`.
  - The half-open test applies a decision once per passage. A vehicle standing exactly on the
    station takes it on its first move.
  - Every lane chain of the Link carries the decision, so a lane change neither repeats nor skips
    it.
  - A later decision replaces an earlier one.
- **Speed.** Nothing else changes:
  - a vehicle above its new desired speed brakes toward it at comfortable deceleration (D191's
    `aboveTarget` cap);
  - the curve bound still takes the minimum.
- **Insertion.** A vehicle inserted at its route's start has passed nothing. A decision at station 0
  of the first Link is crossed on the first tick that moves the front past it.
- **Parking.** A vehicle leaving a lot (M8.4) starts a new trip with the desired speed it parked
  with, and its quantile is taken again at its next decision. The quantile is not carried through a
  lot.

## 4. Delay (owner)

The free-flow time is piecewise: each stretch's distance over the desired speed in force on it.
- A vehicle carries `freeFlowSoFar` (seconds) and `freeFlowFrom` (route distance).
- At each decision it adds `(at − freeFlowFrom) / oldSpeed` and moves `freeFlowFrom` to `at`. A lane
  change shifts `freeFlowFrom` by the same amount as its distance.
- At arrival: `freeFlowTime = freeFlowSoFar + (routeLength − freeFlowFrom) / desiredSpeed`. With no
  decision passed, this is `routeLength / desiredSpeed` bit for bit.
- A travel-time section's free flow uses the speed in force too: the section's track keeps the
  piecewise sum.

## 5. What does not change

- With no decision in a project, every run, output and checkpoint is byte-identical. The new vehicle
  fields are written to a checkpoint only when they differ from their defaults.
- Following, perception and W74 read `desiredSpeed` as before. A perceived decision already in the
  reaction history ages out.
- The Lab has no decisions.

## 6. Lifecycle and editor

- **Commands:** put, delete and move-station, following travel-time sections:
  - deleting the Link deletes its decisions;
  - splitting the Link moves a decision to the piece that holds its station;
  - reversing the Link is refused while a decision names it.
- **Editor:**
  - a *Speed decision* tool places one by clicking a Link;
  - the canvas marks each decision across the Link;
  - a table lists them;
  - an inspector edits name, station and one min/max row per vehicle type.
  - Each edit is one Undo.

## 7. Reduced-speed areas (D224)

A reduced-speed area is a stretch of a Link where some lanes have per-type speed ranges, for example
a school zone. Unlike a decision it is temporary: when the vehicle's front leaves the area, its
previous speed applies again. As in Vissim, the vehicle slows down *before* the area.

**Owner's choices (2026-10-17):**
- **Anticipation deceleration:** a value on each area; when absent, the type's comfortable
  deceleration is used.
- **Delay:** driving slower inside an area is not delay. It is a speed in force, as in §4.
- **Lanes:** an area may name lanes; when it names none, it covers every lane.

Lanes are named by lane id, not by position. Adding or removing a lane at the left edge would shift
every position, but it leaves ids alone.

**This slice covers Links only.** Areas on Connectors are a failure condition (§9).

### 7.1 The object

`definition.reducedSpeedAreas` is an array. Each entry has:
- `id` and an optional `name`;
- `linkId`, `from` and `to`: stations on the Link's reference line, with `0 ≤ from < to ≤ length`;
- `laneIds`: an optional list of the Link's lane ids, each unique. Absent means every lane.
- `deceleration`: optional, in m/s², within (0, 10];
- `speeds`: a list of `{vehicleTypeId, min, max}`, with the same rules as for a decision (§2).

Validation refuses each fault at its path:
- `UNKNOWN_REDUCED_SPEED_AREA_LINK` for an unknown Link;
- `INVALID_POSITION` for `from` or `to`;
- `INVALID_LANE` for a lane id that is not one of the Link's lanes, or that repeats;
- `INVALID_DECELERATION` for a deceleration outside (0, 10];
- `INVALID_RANGE`, `UNKNOWN_VEHICLE_TYPE` and `DUPLICATE_ID` as in §2;
- `OVERLAPPING_REDUCED_SPEED_AREA` when two areas share a lane and overlap. This keeps the speed in
  force unambiguous.

**Schema 39.** The array is written only when it is non-empty. An older schema that carries it is
refused with `EDIT_UNSUPPORTED_FIELD`.

### 7.2 The rule

- **Area speed:** `min + q · (max − min)`, using the vehicle's quantile `q` from §3. `q` is taken
  at the vehicle's first decision or area, whichever comes first.
- **Inside:**
  - A vehicle is inside an area when its front is at or past the start and before the end
    (`start ≤ front < end`), on its *current* lane route.
  - While inside, the speed target is `min(desiredSpeed, areaSpeed)`. An area never raises a
    vehicle's speed.
  - `desiredSpeed` itself is untouched, so leaving the area restores the earlier target. A
    decision inside an area changes `desiredSpeed` and keeps the minimum.
- **Anticipation:**
  - Each area ahead on the current route binds the target to `sqrt(areaSpeed² + 2·b·d)`. Here `d` is
    the distance from the front's end-of-tick position to the area, and `b` is the area's
    deceleration (or the type's comfortable one).
  - The loop has the same shape as the curve bound (SPEED_CONTROL §4), and the two bounds take the
    minimum.
  - When an area sets the target, the braking cap uses that area's `b` instead of comfortable
    deceleration.
  - **Acceleration.** The following model still aims for the full desired speed. So, while an area
    binds and the vehicle is at or below the target, acceleration is also capped at what reaches the
    target in one tick. Without this cap, speed would be clamped to the target but the distance would
    be integrated with the model's acceleration, so the vehicle would cover more ground than its
    speed allows. Row BA105 caught that: the vehicle crossed the section about 2.5 % fast.
- **Lanes:**
  - Each lane has its own route, so a vehicle on an uncovered lane is unaffected.
  - A vehicle that changes into a covered lane mid-area takes the area at once and brakes under the
    same cap.
  - A vehicle that changes out of a covered lane is released.
- **Types:** a type not listed is unaffected.

### 7.3 Delay (owner)

The free-flow time of §4 gains the area pieces:
- The vehicle stores the speed in force, `areaSpeed` (0 outside an area).
- Entering or leaving an area adds the finished piece to `freeFlowSoFar` and moves `freeFlowFrom`:
  - a longitudinal entry or exit closes the piece at the area's boundary;
  - an entry or exit caused by a lane change closes it at the front.
- Each piece is timed at the speed in force, `min(desiredSpeed, areaSpeed)`. A travel-time
  section's track uses the same speed.
- Braking *before* an area and accelerating *after* it fall outside the area pieces, so they count
  as delay. For a lone car slowing from 15 to 5 m/s, that is a few seconds.

### 7.4 Lifecycle and editor

**Commands** follow §6:
- deleting the Link deletes its areas;
- a split is refused when the cut falls strictly inside an area, and an area wholly downstream
  moves to the downstream piece;
- reversing the Link is refused while an area names it;
- removing lanes is refused (`EDIT_REFERENCED_LANE`) when it would drop a lane that an area names.
  An area without `laneIds` never blocks it.
- splitting a Link moves an area's lane ids to the downstream piece's lanes.

**Editor:**
- a *Reduced-speed areas* tab, with Add (for one selected Link), Edit and Delete;
- a dialog sets the name, `from`, `to`, an optional deceleration, one check box per lane, and one
  min/max row per vehicle type;
- the canvas shades each covered lane over the stretch;
- each edit is one Undo step.

### 7.5 Rows (BA97–BA108)

| Row | Case | Asserts |
|---|---|---|
| BA97 | No area | Runs, outputs and the checkpoint are byte-identical; no `areaSpeed` field |
| BA98 | Two cars with quantiles 0.2 and 0.8 inside `{car: 8–12}` | Hold 8.8 and 11.2 (±1e-6); forcing: their speeds before the area first |
| BA99 | Anticipation | At the area's start, speed ≤ area speed + 0.05, with no clamp. The braking is held to `deceleration` when one is given, and to comfortable deceleration otherwise |
| BA100 | Leaving the area | The car accelerates back to its desired speed |
| BA101 | Lanes | A car on an uncovered lane is unaffected. A car changing into a covered lane mid-area takes the area, and one changing out is released |
| BA102 | A type not listed | Unaffected |
| BA103 | A decision inside an area | The target is the minimum, and the quantile is shared |
| BA104 | A lone car through an area | Free-flow time `100/v + 200/5 + 100/v` (±1e-6). Delay < 5 s: braking before the area and accelerating after it still count |
| BA105 | A travel-time section over the same road | Section delay < 0.5 s |
| BA106 | A checkpoint inside the area | Carries `areaSpeed` and replays exactly |
| BA107 | Codec and validation | Round-trip at 39; 38 → `EDIT_UNSUPPORTED_FIELD`; each fault of §7.1 at its path; 40 → `EDIT_VERSION` |
| BA108 | Commands | Link delete cascades; split refuses in the area and moves one downstream with its lanes; reverse refused; lane drop refused only for a named lane; Undo restores |

## 8. Rows (BA86–BA96)

| Row | Case | Asserts |
|---|---|---|
| BA86 | No decision | CLI pin, the shipped projects and a checkpoint byte-identical |
| BA87 | Two cars with quantiles 0.2 and 0.8 pass `{car: 8–12}` | New speeds 8.8 and 11.2 (±1e-9); forcing: their quantiles first |
| BA88 | A type not listed | Its desired speed unchanged |
| BA89 | A lower and a higher range | Brakes at comfortable deceleration with no clamp; accelerates |
| BA90 | A lane change before, and one after, the decision | Applied exactly once |
| BA91 | Two decisions in a row | The second replaces the first; the quantile is kept |
| BA92 | A lone free car through a slower decision | Trip delay < 0.5 s |
| BA93 | A travel-time section over the same road | Section delay < 0.5 s |
| BA94 | A checkpoint after the decision | Replays byte-identically |
| BA95 | Codec and validation | Round-trip at 38; 37 → `EDIT_UNSUPPORTED_FIELD`; each fault of §2 at its path; 39 → `EDIT_VERSION` |
| BA96 | Commands | Link delete cascades; split moves; reverse refused |

## 9. Failure conditions (reopen D222)

- A study needs a decision per lane, or a decision or an area on a Connector.
- A study needs drivers to redraw at a sign rather than keep their quantile.
