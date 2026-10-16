# Perception contract: look-ahead, observed vehicles, look-back and reaction time (M3.5.0)

Designed 2026-10-15 (D179). **Slices 1–4 are implemented**: the codec (D181, schema 32,
[evidence](../evidence/perception-codec.md)), look-ahead with multi-anticipation in own motion
(D182, [evidence](../evidence/perception-motion.md)), look-back, rear time-to-collision and the
perceived lane-change tests (D183, [evidence](../evidence/perception-lane.md)), and the reaction-time
action FIFO (D184, [evidence](../evidence/perception-delay.md)). Rows BA30–BA38 are in
[the delivery plan](../plans/DRIVING_BEHAVIOUR.md#3-failure-first-acceptance-rows); BA30–BA37 have
automated evidence; BA38 has a [discharge sensitivity record](../evidence/perception-discharge.md)
(D185) with 40 seeds per variant, without a calibration inference. The [driving-behaviour review](../audits/DRIVING_BEHAVIOUR_REVIEW.md) §3 A–B and §4.1
motivate it; D177/D178 put it first. [DRIVING_BEHAVIOUR](DRIVING_BEHAVIOUR.md) owns assignment and
selection, [SIMULATION](SIMULATION.md) the tick, [W74](W74.md) and the prototype their equations.
This file owns **what a driver observes and when it acts on it**.

## 1. Scope and sources

In scope: a look-ahead distance and a number of observed vehicles ahead (multi-anticipation), a
look-back distance and a rear time-to-collision test for lane changes (the mirror check), and a
reaction time.

Out of scope, each a later M3.5 contract: temporary lack of attention and driver error (review
§3 C), driver classes (K), visibility at conflict areas, sight distance to signal heads, a
second follower in the rear test, and lateral perception (M3.7).

Written from public sources only:

- Vissim's public parameter vocabulary: *look ahead distance*, *observed vehicles*, *look back
  distance* (names and meaning, not values or internals).
- Treiber, Kesting & Helbing (2006), *Delays, inaccuracies and anticipation in microscopic
  traffic models*, Physica A 360: multi-anticipation over several leaders and a finite reaction
  time as separate mechanisms.

No equivalence with Vissim or the Human Driver Model is claimed. Rule 4 applies: nothing here is
validated, and the not-yet-validated marker stays.

## 2. Two leaders, never confused

Today one search (`closestVehicle`, `src/core/leaders.cpp`) returns the nearest vehicle
ahead at any distance, and the same leader feeds the acceleration, the hard cap and zone
admission. This contract splits the two roles:

| Leader | Defined as | Used by |
|---|---|---|
| **Physical leader** | Today's search, unchanged: the nearest vehicle ahead on the own route, unbounded | The hard cap `allowedDistance` (`simulation.cpp:264`) and its `SafetyClampEvent`; zone admission and receiving space (`VehicleLeader`); phase-2 caps; D108's source admission |
| **Perceived leaders** | Up to `observedVehiclesAhead` vehicles ahead on the own route whose rear is within `lookAheadDistance` of the follower's front, ordered by gap ascending; equal gaps keep scan order, as `closestVehicle`'s strict `<` does, so one observed vehicle with no limit is exactly the physical leader (D182) | The acceleration only (§3) |

**Perception never weakens non-overlap.** A vehicle that does not perceive its physical leader
still cannot move into it: the cap applies, and a clamp is counted (D105). That is the intended
"did not see it in time" outcome, reported rather than hidden.

**Static obstacles are not limited** by `lookAheadDistance` in this contract: signal heads, stop
lines, dead ends, priority rules, zone holds and courtesy holds keep today's logic and
distances. Their sight rules are later contracts. Without the key, every vehicle ahead is in
range, which is today.

## 3. Multi-anticipation

With perceived leaders `L1 … Ln` (gaps `g1 ≤ … ≤ gn`, speeds `vk`, snapshot accelerations `ak`,
lengths `lk = front − rear` of their spans), the vehicle-leader acceleration is

```
gEff1 = g1
gEffk = gk − Σ_{j<k} (lj + standstillGap(own behaviour))     for k ≥ 2
a     = min over k of follow(v, driver, previous, type, behaviour, Leader{gEffk, vk, ak})
```

`gEffk` is the gap to `Lk` less the room the vehicles in between need at rest, so a hard brake two
cars ahead is felt before the car in between reacts. With `n = 1` and no look-ahead limit the
result is exactly today's single `follow` call (`simulation.cpp:346`).

- The kept result is the smallest `a`; on equal accelerations the smaller `k` is kept (the
  existing strict `<`). Its `FollowingMode` and, on a `w74` road, its regime and sign are what
  publish stores (W74.md §7). Evaluations of other `k` are hypothetical and never write state.
- A `gEffk ≤ 0` is passed as is: the model's emergency or braking branch handles it, and only the
  physical leader caps motion.
- The courtesy or cooperative second obstacle (`simulation.cpp:347-356`) is then compared with
  this `a` exactly as today.
- No perceived leader: the vehicle is free of vehicle leaders (static obstacles still apply).

## 4. Look-back and the rear test (the mirror check)

`around` (`src/core/lanes.cpp:46`) finds, on a target lane, the nearest vehicle ahead, the
nearest behind and anything alongside. With the keys set:

- **Ahead on the target lane:** the forward test of `safeAt` (`lanes.cpp:83`) uses the perceived
  leaders there (§3, same keys), not only the nearest.
- **Behind:** a follower is **seen** only if its front is within `lookBackDistance` of the
  changer's rear. An unseen follower does not block the change. After the change it is an
  ordinary follower of the changer: it reacts through its own following model, and the hard cap
  and counted clamps keep it from overlapping.
- **Alongside** is physical, not perceived: anything overlapping the changer's span on the target
  lane always refuses the change, whatever the keys.
- **Rear time-to-collision:** with `rearTimeToCollision` set and a seen follower closing
  (`vF > v`), the change also needs `gap / (vF − v) ≥ rearTimeToCollision`. Equal is accepted. A
  follower not closing passes this test. The existing test (the follower's own `follow` toward
  the changer within comfortable deceleration, or within `acceptedDecelerationTrailingVehicle` for
  a discretionary change) still applies to a seen follower.

Two details settled by D183. The forward test's one-tick room check uses the nearest **seen**
leader; when no leader is seen on the target lane there is no forward test, and the hard cap with
counted clamps governs, as in §2. The time-to-collision uses the snapshot speeds of both vehicles.

These rules apply to mandatory and discretionary changes alike. `courtesyHolds`
(`lanes.cpp:324`) searches followers in the same order as today; look-back does not restrict
courtesy, because courtesy is the follower's decision, not the changer's perception.

Not tested here: a second follower closing fast behind a slow first one. Only the immediate
follower has the changer as its leader; the case is recorded for a later row (review §3 B).

## 5. Reaction time (an action delay)

With `reactionTime` τ, a vehicle acts on what it decided `d` ticks earlier:

```
d = floor(τ / dt + 0.5)          (ticks; d = 0 means today)
```

- Each vehicle on a road whose behaviour has `d > 0` carries a FIFO of `d` entries
  `{acceleration, mode, w74 regime and sign}`. Each tick the acceleration computed by §3 and the
  second-obstacle comparison is pushed, and the oldest entry is **applied**.
- The applied acceleration then passes every existing step unchanged: the type's bounds, D105 (a
  stopped vehicle within its standstill gap gets no positive acceleration), `integrate`, the
  desired-speed cap, the hard cap with counted clamps, Stop rest and phase-2 resolution. The cap
  always reads the **current** physical leader, so the delay can never cause overlap.
- The stored mode and W74 state are those of the applied entry, written at publish only. The W74
  sign hysteresis therefore follows what the driver did, not what it decided last.
- **Insertion:** a new vehicle's FIFO is filled with `{0, free, no state}`. It enters at rest
  (D108) and starts after `d` ticks: start-up reaction, which lowers source and discharge rates.
  BA38 records by how much.
- **Hypothetical evaluations are not delayed:** `safeAt` forward and rear, the courtesy and
  cooperative tests, `deadEndGoverns` (`lanes.cpp:138`), the discretionary gain (`lanes.cpp:301`),
  D108's first step (`simulation.cpp:168`) and `stopLineReach`. They model the driver's judgement,
  not its action.
- **Road boundaries** (D127 selection): the FIFO keeps its entries when the behaviour changes. If
  `d` changes, the oldest entries are dropped, or the FIFO is padded at its old end with copies of
  its oldest entry. On a road with `d = 0` the FIFO is cleared at publish. A vehicle arriving from
  such a road has an empty FIFO: its first delayed tick applies that tick's own decision and fills
  the FIFO with copies of it, so a braking driver keeps braking rather than coasting for `d` ticks
  (D186). Only insertion fills with `{0, free, no state}`.
- The FIFO lives in `Vehicle` (a `std::vector`, empty when `d = 0`), so copying `SimState`
  copies it and replay is exact. It is written only at publish, in vehicle-id order.
- Timestep: `d` is quantised to ticks. The effect of dt is recorded (BA35/BA38), not bounded, as
  in W74.md §8.

**Why an action delay and not a delayed view of the road** (the review's §4.1 proposed a ring of
past snapshots): the FIFO costs one entry per tick per vehicle, where a snapshot ring stores every
span for every delayed tick. It needs no mapping of past positions across lane changes and route
remaps. And in the linearised car-following analysis that motivates reaction time, both delay
the response by the same `τ`. D179 records the change.

## 6. Parameters (hard rule 5: data, never code defaults)

All optional, on both models. **An absent key means today's behaviour**, so an existing file and
an unassigned scenario run byte-identically (BA18, BA30).

| Key | Range | Meaning |
|---|---|---|
| `lookAheadDistance` | > 0 m, finite | Perceived leaders' rear must be within this distance of the front |
| `observedVehiclesAhead` | integer, 1–8 | How many perceived leaders §3 uses; absent = 1 |
| `lookBackDistance` | > 0 m, finite | A target-lane follower is seen only within this distance |
| `rearTimeToCollision` | > 0 s, finite | Minimum time-to-collision with a seen, closing follower |
| `reactionTime` | 0–2 s, finite | Action delay, quantised to ticks |

The upper bounds keep per-tick work bounded (§8). They are not behavioural claims.

**Codec.** The five names live once (one key table, as `w74ParameterKeys` does, D136), read by
`parseBehaviour` (`src/project/parse.cpp:300`), the library codec
(`parseBehaviourLibrary`, `src/project/behaviour_library.cpp:87`, whose unknown-key list gains
them), the serializer and the range check. A non-number or out-of-range value is
`INVALID_BEHAVIOUR_PARAMETER` with its path, on unused behaviours too. The schema is the next free
number, **32** (D181), written **only** when an owned behaviour carries one of
these keys. Every older file keeps its version and bytes, and an older schema carrying a key is
refused (`EDIT_UNSUPPORTED_FIELD`). External catalogs in `data/driver-behaviour/` read the keys
without a schema.

No preset values are set here. A shipped value needs a cited source (D176's pattern, D178's
standard-values rule) and the uncalibrated label.

The Behaviour Lab (`retuneSimulation`, `src/core/retune.cpp`) refuses a retune that changes
`reactionTime`'s tick count, as it refuses a prototype↔W74 change, because the FIFOs would no
longer match. The other four keys may change on a retune.

## 7. Consumers

| Call site | Leaders it reads | Delayed? |
|---|---|---|
| Own motion (`simulation.cpp:346`) | Perceived (§3) for the acceleration, physical for the cap | Yes (§5) |
| Courtesy / cooperative second obstacle (`simulation.cpp:350`) | Today's hold | Yes: part of the pushed value |
| `safeAt` forward (`lanes.cpp:89`) | Perceived on the target lane | No |
| `safeAt` rear (`lanes.cpp:107`) | Seen follower (§4) | No |
| `courtesyHolds` (`lanes.cpp:324`) | Today's order, not limited by look-back | No |
| Discretionary gain (`lanes.cpp:301`) | Perceived, on both lanes | No |
| `deadEndGoverns` (`lanes.cpp:138`) | Static, unchanged | No |
| D108 source first step (`simulation.cpp:160-168`) | Physical | No; the FIFO starts at insertion |
| Zone admission and receiving space | Physical | No |
| `upcomingArrivals` (`src/core/demand.cpp:68`) | None (generation only) | — |

## 8. Performance budget

The perceived-leader search reuses `closestVehicle`'s early break, keeping a sorted list of at
most `observedVehiclesAhead` entries (≤ 8). `lookAheadDistance` can only shorten the scan. The
physical leader comes out of the same pass. The FIFO is `O(d)` memory per vehicle, `d ≤ 2 / dt`.
The implementation slice re-runs `trafficsim-engine-benchmark` before and after (benchmarks are
not in `check`), and a regression over 10 % on an unassigned scenario needs its own decision
(D164).

## 9. Delivery order

One slice per session:

1. Codec (BA37).
2. Look-ahead and multi-anticipation (BA31, BA32).
3. Look-back and rear TTC (BA33, BA34).
4. Reaction time (BA35, BA36).
5. The discharge record (BA38).

BA30 runs in every slice.
