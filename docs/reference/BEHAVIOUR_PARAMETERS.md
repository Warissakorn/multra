# Driving-behaviour parameters: what is settable, and what should be

Written 2026-10-17 (D213) at the owner's request: driving behaviour should be configurable flexibly
enough to use the program in other countries, and the project should judge which values are
appropriate to make settable. **This is a proposal for the owner to confirm**
([OWNER_SITTING](../plans/OWNER_SITTING.md) B19). Nothing here changes behaviour.

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
| Amber | `amberDeceleration` (D147) | behaviour key | yes |
| Perception | `lookAheadDistance`, `observedVehiclesAhead`, `lookBackDistance`, `rearTimeToCollision`, `reactionTime` | behaviour keys | partly |
| Lateral | `lateralMinDistanceStanding`, `lateralMinDistanceAt50`, `maxLateralSpeed` | behaviour keys (all or none) | yes (motorcycles) |
| Curve speed | `curveLateralAcceleration` (D212) | behaviour key | yes |
| Vehicle | `length`, `width`, `desiredSpeed` range, `maxAcceleration`, `comfortableDeceleration`, `maxDeceleration`, axles | type fields | yes (fleet) |
| Road behaviour | a behaviour type per Link/Connector (M3.3.2b) | network field | — |
| Driving side | `drivingSide` | network field | yes |
| Give-way | `gapTime`, `headway` per rule and zone; Stop or Yield | network fields, defaults in `data/priority-rules` | yes |
| Run | `timeStep` (≤ 0.5 s) | run setting | no |

## 3. Tier B — should become settable (proposed order)

| # | Value | Today | Why settable | Queued as |
|---|---|---|---|---|
| B-1 | Kerb-lane bias: return to the kerb lane, pass on the passing side | none (D95 has no side preference) | Keep-left/right rules differ by country (D178) | [OVERTAKING](OVERTAKING.md) (D213) |
| B-2 | Give-way and amber commitment deceleration ("cannot stop, so go") | the type's `maxDeceleration` (`conflicts.hpp` `committed`) | How hard a driver will brake before going differs by country and driver | contract first |
| B-3 | Accepted deceleration for mandatory changes (own and the follower's) | the type's `comfortableDeceleration` (`lanes.cpp` `safeAt`) | Merging assertiveness (Thai merging is tighter) | contract first |
| B-4 | Driver-factor spread | `clamp(0.5 + 0.15·N, 0, 1)` (`demand.cpp`) | Population heterogeneity is a calibration target | contract first |
| B-5 | Desired-speed distribution family | uniform only (`demand.cpp`) | Field speed distributions are rarely uniform (M2.1) | contract first |
| B-6 | Road speed limits, reduced-speed areas, desired-speed decisions | none | Limits and compliance differ by country (M3.6 later slices) | SPEED_CONTROL §1 |
| B-7 | Lane preference by vehicle type (heavy vehicles keep the slow lane) | none | Truck lane rules differ by country | OVERTAKING later slice |
| B-8 | Stop-sign service time in seconds | one tick (`conflicts.hpp`), so it depends on `timeStep` | A real stop lasts about 1–3 s, and the result should not change with the time step | contract first |
| B-9 | Amber without the key; red-light running share | amber holds like red; no running | Both vary strongly by country | contract first |
| B-10 | Lateral reference speed (50 km/h) | constant (`lateral.cpp`) | Only if a study needs it | candidate only |

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

## 5. Where tier A can be edited today

| Keys | Behaviour editor | Behaviour Lab panel |
|---|---|---|
| Prototype 5, D90, amber | yes | yes |
| W74 18 | yes | no (needs a prototype → W74 retune) |
| D95/D101 | yes | no (needs the circuit lane-change wrap rule) |
| Perception, lateral, curve | kept, not edited | no |
| Type fields | catalog dialog | focus type only |

**Proposed exposure slice (owner's wish, 2026-10-17):**
- One slice exposes every optional behaviour key, in the editor and in the Lab panel, as optional
  rows that stay off until ticked. That covers perception, lateral, curve and, once D213 is in,
  kerb bias.
- Discretionary keys and kerb bias in the Lab still wait on the circuit lane-change wrap rule.
- A key a run cannot use yet is never shown as working
  ([DRIVING_BEHAVIOUR](DRIVING_BEHAVIOUR.md) §4).
