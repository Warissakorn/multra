# Behaviour lab contract (M3.3.4, D165, D166)

The owner asked (2026-10-14) for a development tool to tune driving behaviour: a window
that follows one **focus vehicle** around a purpose-built test network it can drive
**endlessly**, with a panel that changes the parameters of that vehicle, the road and the
other vehicles **live, keeping the vehicles**. The first slice covers the prototype
(Wiedemann-inspired) model only; W74 and the lane-change fields come later.

The lab is a development bed, not evidence. Nothing measured in it is a claim about a real
road (hard rule 4); its window carries the unvalidated marker, and it closes no M0, M3 or M6
gate. [ROADMAP](../ROADMAP.md#m33--driving-behaviour-library-and-models) owns status.

## 1. Engine seams

Three opt-in additions. A scenario that uses none of them runs exactly the code path it
always did: same order of operations, same draws, byte-identical events (BL1).

### 1.1 Circuit routes

`Route::circuit` (last member, default `false`) marks a route that closes on itself: its
last segment lists its first in `next`. A vehicle on it never arrives.

- The index holds a circuit route's lap **three times** (`parts`, offsets 0, L, 2L for lap
  length L). Every parts-based look-ahead — `closestVehicle`, signal heads, source first-step
  clearance — therefore sees across the seam with no other change, and `routeHeads` records a
  head on each copy.
- A vehicle whose front passes 2L is moved back by exactly L (same segment, same position).
  So after its first lap a vehicle lives in [L, 2L): its rear is on the previous copy and its
  spans stay on the right segments while it straddles the seam. A `SegmentEnteredEvent` names
  the segment entered; there is no `ArrivedEvent`.
- Validation (`UNSUPPORTED_CIRCUIT`): the route must close, and must not carry a route
  decision, priority rule, conflict zone, lane-change span, dead end or parking gate. The lab
  does not need them, and each reads route distances that would need a wrap rule of its own.
- Not modelled: lane changes on a circuit; routing on a circuit; trip statistics (a circuit
  vehicle never completes a trip, so movement/delay evaluation does not apply).

### 1.2 Live retune

```cpp
SimState retuneSimulation(SimState&&, const Scenario&, const RetuneOptions& = {});
```

Replaces the state's scenario with one of the **same shape** — the same ids in the same
canonical order for segments, routes, vehicle types, behaviours, inputs, signal programs and
heads, the same segment `next` lists and route segment lists — and rebuilds the index (its
`discretionary`/`w74` flags depend on values). Anything else throws `RETUNE_SHAPE_CHANGED`
before the state is touched; the scenario must also pass validation. A change to any behaviour's
quantised reaction-time tick count is refused by the same check ([PERCEPTION §5](PERCEPTION.md#5-reaction-time-an-action-delay));
a value change that keeps the count is allowed. Vehicles, queues, RNG
state, tick, time and stop service are kept. With `rescaleDesiredSpeeds`, a vehicle whose
type's desired-speed range changed keeps its quantile in that range; otherwise its sampled
desired speed is kept (DRIVING_BEHAVIOUR §3: identity and draws survive).

Inputs must remain identical, including rates and time windows: the pending arrival schedule
was sampled from them and is preserved. A demand edit throws `RETUNE_SHAPE_CHANGED` atomically
rather than accepting values that the old schedule cannot represent (D191). Rescaling a desired
speed changes the target only. A vehicle above its new target brakes on subsequent ticks, using
comfortable deceleration capped to reach the target, or more strongly for an obstacle within
the usual type limits; its speed, acceleration and integrated distance remain consistent.

A retune is a discontinuity a reproducible run does not have: a run with retunes replays only
when the same retunes are applied at the same ticks.

### 1.3 Placing a vehicle

```cpp
SimState placeVehicle(SimState&&, const VehiclePlacement&);
// VehiclePlacement { routeId, vehicleTypeId, distance, speed }
```

Adds one vehicle with the next id, drawing its desired speed and driver factor exactly as an
arrival does (`samplePending`), with no input (`kNoInput`). `PLACEMENT_INVALID` when the route
or type is unknown, the distance is outside the route (circuit: [0, 2L)), the speed is
negative, or its span overlaps another vehicle's. A ring is filled this way at t = 0, so a
lab vehicle count is exact and no source admission is involved.

## 2. Lab module (`src/lab/`)

Qt-free; depends on core, model, project and commands. `LabSettings` (focus type and
behaviour, other-traffic counts, heavy share, speed range and behaviour, the layout and its
dimensions, lanes, signal timings) → `buildLabDocument` (built with the editor's commands; it
names the main Route and the cross Routes) → `LabRun` (compile with `buildScenario`, mark every
lane chain circuit, place the vehicles evenly loop by loop, step, retune). Focus, traffic and
signal-timing values retune live; a layout, road or vehicle-count change rebuilds the run from t = 0.

Two layouts (D166), chosen in the Road tab:

- **Urban circuit** (default). Left-hand traffic; a block `straightLength` × `blockDepth`.
  The tuned vehicle's loop runs anticlockwise: the south street (a long straight with two
  straight-through signalised crossings X1a and X1b), a sweeping SE corner (`curveRadius`), the
  east street, a **left turn** at X2, the north street, a **left turn** at X3, the west street
  and a sweeping SW corner. Three cross-traffic loops: C1 crosses the south street at X1a
  (northbound) and X1b (southbound); C2 runs south through X2 beside the east street, then turns
  away; C3 runs east through X3 beside the north street, then turns away. A one-way street's
  reference line lies `lanes × laneWidth / 2` to the left of its centreline, so two loops on one
  street read as a two-way road.
- **Oval**: the D165 stadium ring, with one optional signal.

No loop shares a segment with another, so there is no merge and no priority rule; each
intersection is two loops crossing under one fixed-time controller: main green `green`, then
`amber` and `allRed`, cross green for `red − amber − 2·allRed`, cross amber, `allRed`. The four
controllers' offsets are a quarter cycle apart. Crossing streams never share a segment, so the
engine does not see them conflict: the signals alone keep them apart, as the four-leg fixture's
split phasing does before conflict areas (honest only while every phase has its all-red).
Give-way intersections need conflict zones or priority rules on circuits, which §1.1 refuses
until a wrap rule is written. The focus vehicle is the
`lab-focus` vehicle; `focusTelemetry` reports its speed, acceleration, desired speed, mode,
leader gap and speed, and lap count.

## 3. Window

Simulation ▸ *Behaviour Lab…* opens a non-modal window: the Run canvas with the camera
locked on the focus vehicle (clicking another vehicle moves the focus) and, by default,
**heading-up** (D166): the view turns every frame so the followed body's nose points to the top
of the screen (`setRunFollow(id, true)`); turning it off restores north-up at the same scale.
The canvas reads its scale as `viewScale()` = hypot(m11, m12), equal to |m11| unrotated, so the
editor's drawing is unchanged. Run/Pause, Step,
Reset, speed and seed controls, a parameter panel (focus vehicle, other traffic, road) and
a telemetry strip. Initial values come from the open project's resolved catalog.
Each behaviour has the prototype keys, amber and D90 and, since D217, the perception keys, the
lateral keys (one box sets all three) and `curveLateralAcceleration`. Each optional row stays off
until ticked. The curve acts on curved Connectors only: the Urban left turns and the Oval's joins.
The D95 keys and kerb bias are not offered, because the circuit clears lane changes (§1.1).
*Apply to project* writes the tuned focus behaviour back into the behaviour the project's
first vehicle type uses, as one History edit. Body length and axle geometry are written together;
type and behaviour IDs retain their identity. Valid explicit wheelbase/front/rear overhangs scale
in proportion to a changed focus length (`lab::rescaleAxles`); missing geometry keeps nominal
defaults, and malformed geometry still fails strict validation. The panel displays the resulting
dimensions and this policy. Labels are translated; parameter names are not.

## 4. Acceptance rows

| Row | Check | Where |
|---|---|---|
| BL1 | A scenario without circuit, retune or placement replays byte-identically (frozen fixtures, CLI 42) | existing suites |
| BL2 | One vehicle on a circuit laps indefinitely: distance stays in [0, 2L), no `ArrivedEvent` | `circuit` |
| BL3 | A follower behind the seam sees its leader across it, at the geometric gap | `circuit` |
| BL4 | A red head just past the seam stops a vehicle before it, on every lap | `circuit` |
| BL5 | An open circuit route, or one with a decision/rule/zone/span/dead end, is refused | `circuit` |
| BL6 | Retune with the same scenario continues byte-identically; a changed value leaves the vehicles where they are; a shape change throws and leaves nothing changed | `circuit` |
| BL7 | Desired speeds keep their quantile with `rescaleDesiredSpeeds`, are kept without it | `circuit` |
| BL8 | Placement refuses overlap, unknown ids and out-of-route distances | `circuit` |
| BL9 | The lab document compiles; a default ring runs 1 h without a safety clamp | `lab` |
| BL10 | A focus `additiveSafetyDistance` retune keeps the vehicles and lengthens the focus vehicle's steady gap; a road change restarts | `lab` |
| BL11 | Telemetry gap equals the geometric gap, also across the seam | `lab` |
| BL12 | The window opens from the menu, keeps the focus vehicle centred, retunes without changing ids, rebuilds on a road change, and Apply adds one History revision | `behaviour-lab-ui` |
| BL13 | The Urban circuit (default) compiles four circuit loops per lane, four controllers and a head on every lane of both approaches of each intersection | `lab` |
| BL14 | On 1 and 2 lanes an hour keeps every vehicle without a safety clamp; the tuned vehicle laps, takes both left turns and slows below 1 m/s at a red | `lab` |
| BL15 | A signal timing retunes in place; a cross-traffic count or layout change restarts; a block too small for its curves and boxes is refused (`LAB_ROAD_INVALID`) | `lab` |
| BL16 | Heading-up keeps the followed vehicle pointing up and centred through the turns; off restores north-up | `behaviour-lab-ui` |
| BL17 | Length retune preserves axle proportions, vehicle IDs and time; repeated Apply is unchanged; project Apply and Undo carry body and axle geometry together | `lab`, `behaviour-lab-ui` |
