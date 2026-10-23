# Lane-change duration contract (M3.4)

Written 2026-10-16 (D204). **Slice 1 implemented** (D206, [evidence](../evidence/lane-change-duration.md)),
with D206's two corrections to §3. The three open choices were settled by the owner on
2026-10-16 (§3 start rule, §6 display, §2 no new key). Rows BA48–BA53 are in
[the delivery plan](../plans/DRIVING_BEHAVIOUR.md#3-failure-first-acceptance-rows). It builds on
[LATERAL](LATERAL.md) (spans §5, predicates §6, motion and crossing §7) and changes only the
mandatory and discretionary lane changes of [M3_8_CONTRACT](M3_8_CONTRACT.md) §2. Rule 4 applies:
nothing here is validated, and the marker stays.

## 1. Scope and sources

The [driving-behaviour review](../audits/DRIVING_BEHAVIOUR_REVIEW.md) §3 F names the gap: a change
is instantaneous, so a changer blocks nothing on its old lane and appears in the new one at once,
while the screen slides it over `3·max(v, 5)` m. Its §6 sketch is the target: during a change the
changer occupies both lanes, followers on either lane treat it as a leader, nothing enters the
space it is vacating until the change ends, and display and engine positions agree.

In scope: when a change starts, how it runs, how it ends, and what the display draws.
Out of scope, each its own item in ROADMAP M3.4: `laneChangeDistance`, politeness, cooperative
lane change, zipper merging and blocking timeouts, overtaking with the driving side, and
abandoning a change once started.

## 2. The switch and the parameters

- The duration applies only when `ScenarioIndex::lateral.on` (LATERAL §8). Switched off, every
  change is instantaneous exactly as today (`decideLaneChanges` → `recordLaneChange`), and the
  CLI seed 42 pin, the frozen baselines and every shipped project stay byte-identical (BA48 keeps
  BA39).
- **No new key (owner, D204).** A change sets a lateral target; the existing `maxLateralSpeed`
  caps the rate (LATERAL §7, §9), so a change across a lane of width `w` lasts at least
  `w / maxLateralSpeed` (3.5 m at 1 m/s: 3.5 s). One parameter governs every lateral move, so a
  lane change and a later in-lane move cannot disagree (hard rule 3). No schema change; no values
  are shipped (D178: a preset needs a cited value).

## 3. Starting a change

Switched on, `decideLaneChanges` (`src/core/lanes.cpp`) keeps every existing test (the span window,
`inConflictArea` on both routes, Stop service, `around` with the perception keys, `safeAt`,
the discretionary incentive and hold) and then:

- **No route remap at start.** Instead of `recordLaneChange`, an accepted change sets
  `lateralTarget` to the target lane's centre, expressed from the current lane's centre (the
  section centres of LATERAL §3 at the front's reference station). The route remaps later, at the
  crossing (§4).
- **It must finish before the next conflict area (owner, D204).** Let
  `travel = speed · |lateralTarget − lateral| / maxLateralSpeed + length` (current speed, a stated
  simplification). The change starts only if, on both the current route and the target route at
  the mapped distance, the next zone's line (`waitAt` for a minor role, `entryAt` for a major one)
  is at least `travel` ahead of the front. This extends the existing rule that no change starts
  inside an area: a changer therefore never straddles into a zone, and no zone has to hold a
  straddler. D202's occupancy stays as the safety net other vehicles see.
- **It must cross inside the span window.** The crossing (LATERAL §7) needs a span that covers the
  whole body; the change starts only if `front + speed · |lateralTarget − lateral| / (2 · maxLateralSpeed)
  ≤ fromEnd` of that span, so the centre passes the lane line before the window closes. D206: the
  body's length is not counted here (D204 wrote `travel / 2`), or a changer standing within half its
  length of its dead end could never start.
- **A change under way holds its target place (D206).** Every vehicle with `lateralTarget` set is
  counted, at its target, in the other vehicles' start tests for the whole change, not only in the
  tick it was accepted; otherwise two changers converging on one lane from both sides both start.
- **One change at a time.** A vehicle with `lateralTarget` set starts no other change.
- **A behaviour without the keys changes instantaneously (D206).** With the switch on, a vehicle
  whose behaviour carries no `maxLateralSpeed` (every key or none, LATERAL §9) cannot move sideways,
  so its change is recorded at acceptance as with the switch off.
- Accepted changes of one tick are decided in vehicle-id order against the snapshot and the
  changes already accepted, as today; an accepted changer's spans on both lanes join `moved`.

## 4. During a change

- `moveLaterally` (LATERAL §7) moves the body toward the target at the rate cap, after the
  longitudinal step, admitted only without 2D overlap, in vehicle-id order.
- The body casts spans on both lanes while it covers both (§5), so the follower on either lane
  treats it as a leader through the existing leader search; nothing new is needed for "nothing
  enters the space it is vacating" — the old lane's follower is held by the changer's own-lane span
  until the body leaves that lane.
- **The crossing** remaps the route through `recordLaneChange` with the trace entry flagged
  `lateral` (D189, D203); `lateral` and `lateralTarget` are re-expressed from the new lane.
- **A refused move waits:** `lateral` is kept and `lateralSpeed` is 0; the change is neither
  restarted nor abandoned in this version (§9).
- **Waiting at the line (owner, D208).** A move outward from the own lane's centre that would put the
  body (further) over a lane line must also pass the start's safety tests on the lane it would cover
  (M3_8_CONTRACT §2 rules 3–5): no vehicle of that lane alongside the body, and the car-following
  model accepts the nearest leader ahead of it and the nearest follower behind it with the changer as
  leader. Each vehicle whose route runs on the covered segment is compared in its own route's
  distances, so the test sees across segment ends. Refused, the changer keeps driving in its own
  lane; a body already over the line steps back toward its centre by up to one step, so it never
  waits straddling the line (a straddler does not follow the other lane's leaders, LATERAL §6).
  Moves back toward the own centre, and moves after the crossing, are never tested.
- Longitudinal motion during the change follows the vehicle's own route as today; a change does
  not alter the dead-end or cooperation logic (D71, D90).

## 5. Ending a change

When `lateral` equals the target (centred on the target lane), `lateralTarget` is cleared and the
vehicle may start another change from the next tick. `lastLaneChange` keeps the crossing's tick,
as an instantaneous change records it, so the D101 hold counts from the crossing.

## 6. Display (owner, D204)

As D203: switched on, the canvas draws the engine's position (lane centre + `lateral`) and never
slides a lateral crossing. The D97 slide stays only for switched-off runs, where changes remain
instantaneous; it is removed only if a later decision makes every run lateral.

## 7. Performance

The implementing slice re-runs LATERAL §11 with real targets: `multra-engine-benchmark`
switched off (no change expected), and `lane-change-lab` and `four-leg-signalised` switched on
against main. A regression above 10 % switched on needs its own decision (D164).

## 8. Rows

BA48–BA53 in [DRIVING_BEHAVIOUR](../plans/DRIVING_BEHAVIOUR.md) §3, each written failure-first and
asserting its forcing before its consequence.

## 9. Failure conditions (when to reopen D204)

- On short approaches, vehicles miss their movement because no change can finish before the next
  zone: then hold a straddler at the other lane's zone through mapped incidence (D202).
- Two changers wait on each other's refused moves (a lateral deadlock): then add abandoning and
  returning to the old lane.
- Clamps from a lateral move into a lane where a vehicle has passed close ahead (seen twice in ten
  lab seeds, [evidence](../evidence/lane-change-duration.md)): resolved by §4's waiting at the line
  (D208, [evidence](../evidence/lane-change-gap.md)). Reopen if a changer waits so long it misses its
  window: then abandoning.
- A cited source shows lane-change duration independent of lateral speed: then a separate key.
- **Observed 2026-10-23** ([lateral-run-audit](../evidence/lateral-run-audit.md)): on
  `m2.6-study-template`, a changer past its window stood at its dead end with its centre on the
  line and jammed both lanes. Two changers swapping lanes waited side by side for 500 m in the lab.
  The first and second conditions above are met; nothing has been reopened yet.
