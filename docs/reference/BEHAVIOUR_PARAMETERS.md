# Driving-behaviour parameters: what is settable, and what should be

Written 2026-10-17 (D213) at the owner's request: driving behaviour should be configurable flexibly
enough to use the program in other countries, and the project should judge which values are
appropriate to make settable. The owner delegated the decision ("ช่วยตัดสินใจ", "help decide",
2026-10-17); **the tiers and the order below are decided on that delegation (D214)**, and the owner
can overturn them. Nothing here changes behaviour.

It complements [DRIVING_BEHAVIOUR](DRIVING_BEHAVIOUR.md) §4. That table maps Vissim's parameters at
`07eb5cb`; this one covers every value that shapes behaviour in the engine today, including
hard-coded ones.

## 1. The three tiers

- **A — settable now.** A behaviour key, a vehicle-type field, a network field or a run setting.
  Absent optional keys mean the earlier behaviour, byte-identical.
- **B — should become settable.** Country- or study-sensitive values that are hard-coded today.
  Each becomes a queued item, contract and failure-first rows first, then an optional key that is
  absent by default (hard rule 5: data, never code defaults).
- **C — keep fixed.** Numerical tolerances, safety invariants, the form of a published model,
  determinism tie-breaks and display-only values. Making these settable would let a study break the
  engine's guarantees or stop being the named model.

**Regional behaviour is data, not code.** A country or region is a catalog behaviour (and vehicle
types) built only from cited values, flagged uncertain (D178). `drivingSide` is per network. There is
no per-country branch anywhere in code.

## 2. Tier A — settable now

| Area | Values | Where | Country-sensitive |
|---|---|---|---|
| Car-following, prototype | `standstillDistance`, `additiveSafetyDistance`, `multiplicativeSafetyDistance`, `followingTime`, `speedThreshold` | behaviour keys (`types.hpp`) | yes (headways) |
| Car-following, W74 | the 18 keys (`w74ParameterKeys`) | behaviour keys | yes |
| Cooperative braking | `maxDecelerationCooperativeBraking` (D90) | behaviour key | yes |
| Discretionary changes | `discretionaryLaneChangeThreshold`, `acceptedDecelerationTrailingVehicle`, `discretionaryLaneChangeHoldTime` (D95/D101, off in shipped data) | behaviour keys | yes |
| Kerb-lane bias | `kerbLaneBias` (D215; needs the D95 pair) | behaviour key | yes |
| Amber | `amberDeceleration` (D147) | behaviour key | yes |
| Mandatory lane changes | `mandatoryDecelerationOwn`, `mandatoryDecelerationTrailingVehicle` (D221; the trailing one is the changer's) | behaviour keys | yes |
| Give-way commitment | `commitmentDeceleration` (D219; the lower of it and the type's `maxDeceleration`, give-way lines only) | behaviour key | yes |
| Perception | `lookAheadDistance`, `observedVehiclesAhead`, `lookBackDistance`, `rearTimeToCollision`, `reactionTime` | behaviour keys | partly |
| Lateral | `lateralMinDistanceStanding`, `lateralMinDistanceAt50`, `maxLateralSpeed` | behaviour keys (all or none) | yes (motorcycles) |
| Curve speed | `curveLateralAcceleration` (D212) | behaviour key | yes |
| Vehicle | `length`, `width`, `desiredSpeed` range, `maxAcceleration`, `comfortableDeceleration`, `maxDeceleration`, axles | type fields | yes (fleet) |
| Road behaviour | a behaviour type per Link/Connector (M3.3.2b) | network field | — |
| Driving side | `drivingSide` | network field | yes |
| Give-way | `gapTime`, `headway` per rule and zone; Stop or Yield | network fields, defaults in `data/priority-rules` | yes |
| Speed limits | desired-speed decisions: per-type ranges at a Link station (D223); reduced-speed areas over a stretch, by lane (D225) | definition objects | yes |
| Run | `timeStep` (≤ 0.5 s) | run setting | no |

## 3. Tier B — should become settable (decided order, D214)

The order follows each item's effect on the outputs a Thai study reports: turning, merging and
give-way delay first. Item numbers are stable labels, not ranks.

| Rank | # | Value | Today | Why settable | Queued as |
|---|---|---|---|---|---|
| 1 | B-1 | Kerb-lane bias: return to the kerb lane, pass on the passing side | **done:** `kerbLaneBias` (D215) | Keep-left/right rules differ by country (D178) | [OVERTAKING](OVERTAKING.md) (D213, D215) |
| 2 | B-2 | Give-way commitment deceleration ("cannot stop, so go") | **done:** `commitmentDeceleration` (D219), give-way lines only (owner); the red head keeps the type's maximum | Sets minor-approach capacity at every priority junction; assertiveness differs by country | [COMMITMENT](COMMITMENT.md) (D218, D219) |
| 3 | B-3 | Accepted deceleration for mandatory changes (own and the follower's) | **done:** `mandatoryDecelerationOwn` / `mandatoryDecelerationTrailingVehicle` (D221) | Sets merging and weaving capacity (Thai merging is tighter) | [MERGE_DECELERATION](MERGE_DECELERATION.md) (D220, D221) |
| 4 | B-6 | Road speed limits, reduced-speed areas, desired-speed decisions | **done** (decisions D223, areas D225); Connector areas when a study needs them | Corridor travel times; limits and compliance differ by country | [SPEED_DECISIONS](SPEED_DECISIONS.md) (D222–D225) |
| 5 | B-5 | Desired-speed distribution family | uniform only (`demand.cpp`) | Speed spread and platooning; field distributions are rarely uniform (M2.1) | contract first |
| 6 | B-8 | Stop-sign service time in seconds | one tick (`conflicts.hpp`) | Also a correctness fix: the result must not depend on `timeStep`; a real stop lasts about 1–3 s | contract first |
| 7 | B-7 | Lane preference by vehicle type (heavy vehicles keep the slow lane) | none | Truck lane rules differ by country | OVERTAKING later slice |
| 8 | B-4 | Driver-factor spread | `clamp(0.5 + 0.15·N, 0, 1)` (`demand.cpp`) | Population heterogeneity; calibration only | contract first |
| 9 | B-9 | Red-light running share | not modelled | Varies strongly by country; a new model, so last. Amber without the key is already settable by giving `amberDeceleration` | contract first |

## 4. Tier C — keep fixed

| Values | Where | Why fixed |
|---|---|---|
| 1e-9 comparisons, `kPast`, the 1e-7 tick check, `kLateralOverlap` | throughout `src/core` | Numerical tolerances, not behaviour |
| The hard cap and safety clamp, no 2D overlap, D105's standstill rule | `simulation.cpp`, `following.cpp` | Safety invariants every run relies on |
| W74's internal 0.5 factors, the √v floor, `dmax ≥ v²/2b` | `w74.cpp` | The model's published form; change them and it is no longer W74 |
| The prototype's free-flow relaxation reusing `followingTime`; the 1.5 × desired-gap band | `following.cpp` | The prototype's form; a different form is a new model, not a key |
| Id and slot tie-breaks (D207's higher id yields, lower slot on ties) | `lanes.cpp` | Determinism |
| `kStoppedSpeed` 0.1 m/s, `kWaitingSpeed` 0.5 m/s, `kLineReach` 0.1 m | `conflicts.cpp`, `types.hpp` | They change outcomes but classify states (standing, waiting, at the line); as keys a study could break invariants. Reopen only with evidence |
| Lane-change display slide `3·max(v, 5)` | `lane_change_pose.cpp` | Display only |
| Lateral reference speed 50 km/h (`dLat` stops rising there) | `lateral.cpp` | Moved here from tier B by D214: no study needs it, and Vissim uses the same fixed reference |

## 5. Where tier A can be edited today

| Keys | Behaviour editor | Behaviour Lab panel |
|---|---|---|
| Prototype 5, D90, amber | yes | yes |
| W74 18 | yes | no (needs a prototype → W74 retune) |
| D95/D101, kerb bias | yes | no (needs the circuit lane-change wrap rule) |
| Perception 5, lateral 3 (one box), curve, commitment (D219), mandatory decelerations (D221) | yes, for either model (D217) | yes (D217) except commitment (no give-way line in the circuits) and mandatory decelerations (no lane changes); the curve acts on curved Connectors only |
| Type fields | catalog dialog | focus type only |

**Exposure slice, done (D217, [evidence](../evidence/behaviour-key-exposure.md)):**
- Every optional behaviour key is a row that stays off until ticked, both in the editor and, where a
  Lab run can use it, in the Lab panel.
- Discretionary keys and kerb bias in the Lab still wait on the circuit lane-change wrap rule.
- A key a run cannot use yet is never shown as working
  ([DRIVING_BEHAVIOUR](DRIVING_BEHAVIOUR.md) §4).

## 6. Scope: project-wide or local (D216)

The owner asked which settings apply to the whole project and which to a part of it. Each value lives
at exactly one of six levels, widest first. **Where two levels could both apply, the more local one
wins:** an object's own value over a road's behaviour, over the vehicle type's behaviour, over the
catalog default.

| Level | Scope | Values (today, then tier B to come) | Why this level |
|---|---|---|---|
| 1. Project | One value for the whole project; never local | `timeStep`, duration, seeds, `drivingSide`, evaluation settings | One clock keeps runs deterministic; mixed driving sides are not modelled; a study compares whole runs |
| 2. Catalog default | Project-wide default, overridden below | priority `gapTime`/`headway` defaults; each vehicle type's default behaviour | A sensible start that every object or road may override |
| 3. Vehicle type | Per class (car, heavy vehicle, motorcycle, …) | dimensions, accelerations and decelerations, desired-speed range; **to come:** desired-speed family (B-5), lane preference by type (B-7) | Physical and legal properties of a class, the same on every road |
| 4. Driver behaviour | Per behaviour, so **local by road** through the per Link/Connector behaviour assignment (M3.3.2b) | the car-following keys, D90, D95/D101, `kerbLaneBias`, `amberDeceleration`, perception, lateral, `curveLateralAcceleration`, `commitmentDeceleration` (B-2), the mandatory decelerations (B-3); **to come:** driver-factor spread (B-4), Stop service seconds (B-8), red-light running (B-9) | How people drive, which changes with place (urban, rural, a busy junction); the existing assignment already makes every key local |
| 5. Network object | Per rule, zone, signal, Connector or area | priority rule and zone `gapTime`/`headway` and Stop/Yield; signal programs; Connector curvature (from the drawing), desired-speed decisions and reduced-speed areas (B-6, D223, D225) | Properties of a place, not of a driver |
| 6. Per vehicle | Drawn per vehicle, never set directly | `desiredSpeed`, `driverFactor`, W74 traits | Heterogeneity; set through the distributions at levels 3–4 |

- **Local by road needs no new mechanism.** Assign a behaviour type to the Links and Connectors where
  drivers behave differently. A vehicle takes the behaviour of the segment its front is on, so values
  change at segment boundaries.
- **Not covered:** values that change over time (by study period). They would need their own contract.
- **Editing follows the level:**
  - project settings in the run settings;
  - level 2–4 values in the catalog, behaviour and type dialogs and the Behaviour Lab;
  - level 5 values on the object in the editor.

  The exposure slice (§5, D217) added the missing behaviour rows at level 4.
