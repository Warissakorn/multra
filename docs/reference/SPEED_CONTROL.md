# Speed control contract: curve speed (M3.6)

Written 2026-10-16 (D211). **Slice 1 implemented** (D212, [evidence](../evidence/curve-speed.md)), with §3's
storage and §4's distance corrected by it. The owner settled three
choices on 2026-10-16:
- curve speed from Connector geometry comes first, and reduced-speed areas later (§1);
- the lateral acceleration is a driver-behaviour key (§2);
- delay stays measured against the desired speed (§5).

Rows BA56–BA63, and BA109 (D226), are in [the delivery plan](../plans/DRIVING_BEHAVIOUR.md#3-failure-first-acceptance-rows).
Rule 4 applies: nothing here is validated, and the marker stays.

## 1. Scope and sources

The [driving-behaviour review](../audits/DRIVING_BEHAVIOUR_REVIEW.md) §3 E names the gap: **turning
vehicles cross the junction at full desired speed**. Turning saturation flow and conflict-zone
occupancy times are therefore wrong in a predictable direction. Its §5 item 3 ranks M3.6 as the
cheapest large effect on the outputs users report, and its §6 sketch is the target row: "a 90°
Connector of 15 m radius is crossed at no more than its curve speed … unset, byte-identical".

Today nothing but the vehicle type's desired-speed range limits speed. The tick caps speed at
`Vehicle::desiredSpeed`, and when a vehicle is above it, free flow brakes toward it at
`comfortableDeceleration` (D191, `src/core/simulation.cpp`, the `aboveTarget` lines). Neither `Link`
nor `Connector` carries a speed, and the core `Segment` carries no geometry
([SIMULATION](SIMULATION.md): desired speeds belong to the vehicle type).

**In scope (slice 1):** a speed bound on Connector lane paths from their curvature, anticipated
before the curve. M2.1's "desired speed versus Link limits" is delivered by M3.6 and not duplicated
there.

Desired-speed decisions and reduced-speed areas have their own contract,
[SPEED_DECISIONS](SPEED_DECISIONS.md) (B-6, D222).

**Out of scope**, each a later M3.6 slice with its own contract section written first:
- reduced-speed areas by vehicle type;
- desired-speed decisions (speed-limit changes);
- Link curvature;
- speed-dependent acceleration (W74 already has its own, `w74.cpp`);
- grades.

## 2. The switch and the key

- There is one optional driver-behaviour key, **`curveLateralAcceleration`**: the lateral
  acceleration, in m/s², a driver accepts on a curve. Its range is (0, 10], and values outside it
  are refused at the key's path with `INVALID_BEHAVIOUR_PARAMETER`. There is no code default (hard
  rule 5).
- **An absent key means today's behaviour.** The switch is on when some behaviour carries the key.
  Switched off, nothing is compiled or evaluated, and the CLI seed 42 pin, the frozen baselines and
  every shipped project stay byte-identical (BA56).
- A vehicle is bounded only while its **effective behaviour at its front** carries the key: the same
  behaviour M3.3.2b selects by front segment. A behaviour without it is unbounded in a run where
  another behaviour switches the bound on. This is the same arrangement as LATERAL's `dLat = 0`
  (D188).
- **Schema 34.** The key is written only when an owned behaviour carries it. An older schema carrying
  it is refused with `EDIT_UNSUPPORTED_FIELD`, following the `rejectLateralBefore33` pattern
  (`src/project/behaviour_library.cpp`). A document without the key keeps its schema (BA63).
- **No shipped value.** D178 requires cited published standard values flagged uncertain before a
  value ships, so `data/` and the presets are unchanged. Design guides put the comfortable lateral
  acceleration in turns at roughly 1.5–3 m/s²; that is context, not a value.

## 3. The radius

- It is compiled per Connector lane-path segment, **only while the switch is on**, in the same way
  `segmentCrossSections` are compiled only for the lateral layer. It is stored as **curvature**
  `Segment::curvature` = 1/R (D212: a field cannot fall out of step with the segment list, and 0 needs
  no infinity). It is 0 (no bound) when the switch is off, for Link lane sections in this slice, and
  for a straight path.
- A path's radius is **the minimum circumradius over consecutive vertex triples of its compiled
  polyline** (`connectorPaths`). The formula is `|ab|·|bd|·|ad| / (4·area)`, the same as the
  `TIGHT_CONNECTOR_RADIUS` advisory in `connectorShapeIssues` (`src/model/network/compile.cpp`), so
  there is one radius definition (hard rule 3): `minimumCircumradius` (`network.hpp`).
  - A collinear triple (area ≤ 1e-12) gives an infinite radius.
  - A path with fewer than three vertices has no bound.
- **Stated simplification.** Vertices on a circular arc give its true radius. For any other shape, the
  value depends on how many vertices the path has (two ends plus three intermediate points by
  default, more where the author edits). M6 measures turning speeds by radius (review §7); this
  definition is reopened if they disagree (§9).

## 4. The bound

- The curve speed of a segment with radius `R`, for a behaviour carrying `a`, is
  **`vC = sqrt(a · R)`**.
- A vehicle's speed target is **`target = min(desiredSpeed, bound)`**. `bound` is the minimum, over
  the segments with a finite radius ahead on its own route parts (the one its front is on included),
  of `sqrt(vC² + 2 · b · d)`, where:
  - `d` is the distance to that segment's start from **where the front will be after this tick at
    its current speed** (`front + speed·dt`), 0 once that place is on it (D212: measured from the
    front's present place, a capped vehicle lags the profile by a tick and enters above `vC`);
  - `b` is its type's `comfortableDeceleration`.

  A vehicle that keeps to the target therefore reaches every curve at or below the curve's speed,
  braking no harder than comfortable.
- The target takes `desiredSpeed`'s place in the tick's cap lines. That is the existing D191 path:
  above the target, free flow brakes toward it at no more than `comfortableDeceleration`; at or
  below it, speed is capped at it. The rule is the same in both models, with no new braking mode.
  The model's own free term still reads `desiredSpeed`; the cap bounds it.
- **Acceleration (D226).** While the target is below the desired speed and the vehicle is at or below
  it, acceleration is also capped at what reaches the target in one tick.
  - Before D226 only the speed was clamped, and the distance was still integrated with the model's
    acceleration toward `desiredSpeed`. A held vehicle then gained `½·a·dt²` more each tick than its
    speed allows.
  - Row BA109 (the BA57 fixture): each tick adds at most the trapezoid of its two speeds.
  - Reduced-speed areas use the same line (SPEED_DECISIONS §7.2).
- The look-ahead stops at the first segment whose start lies so far ahead that no curve beyond it
  could bound the vehicle: where `sqrt(2 · b · d)` already reaches the desired speed, because then
  `sqrt(vC² + 2 · b · d)` does too, even for `vC = 0`.
- A leader still governs when it is the tighter limit: the bound is a free-flow target, not a
  following input. The hard cap and clamps are unchanged.

## 5. What does not change

- **Delay is measured against the desired speed (owner).** `freeFlowTime` (route length ÷
  `desiredSpeed`), section delay and discharge records are unchanged, so slowing for a curve counts
  as delay. This is the convention of a field study that uses the straight-road speed.
- `Vehicle::desiredSpeed` is not redrawn or changed. The target is computed each tick and never
  stored.
- The display, the lateral layer and following are untouched.

## 6. Consumers

| Site | Change |
|---|---|
| `simulation.cpp` cap (the `aboveTarget` lines) | Reads `target` instead of `desiredSpeed`; `speedTarget` (`src/core/speed_control.cpp`) is called only when the behaviour carries the key |
| `src/model/network/compile.cpp` | Compiles the radius vector while some behaviour carries the key; the shared circumradius helper is also used by `connectorShapeIssues` |
| Codec: `parse.cpp`, `definition.cpp`, `behaviour_library.cpp`; `document.cpp` schema chain | The key; schema 34 |
| `freeFlowTime`, sections, discharge, outputs | None (§5) |

## 7. Performance

Switched off: no cost, and output byte-identical. Switched on: at most 5 % Ir on `four-leg-signalised`
(seed 42, callgrind, as D209 measured). The look-ahead is bounded by §4's stop and reads a
precomputed vector.

## 8. Delivery

**Slice 1** is §2–§5 with rows BA56–BA63, in one session, rows first (BA57 fails first). The later
slices of §1 each start with a contract section and its own rows.

## 9. Failure conditions (reopen D211)

- M6 measures turning speeds that disagree with `sqrt(a · R)` by radius beyond the spread of its
  data.
- The vertex-count dependence of §3 changes a study's result materially: the same Connector drawn
  with more points gives a different turning flow.
- A run needs a bound on Links (a curved road) before the Link slice exists.
