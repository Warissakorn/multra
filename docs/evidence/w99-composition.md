# M3.3.3b W99 composition evidence — 2026-11-24

Branch `claude/vibrant-maxwell-4uq0we` after `65768c0` (D641). Local host: Linux, GCC 13.3.0, desktop
preset. Windows evidence belongs to the packaging CI. Contract: [W99](../reference/W99.md) §6–§7;
decision D642; rows BA226, BA230 and BA232 of
[DRIVING_BEHAVIOUR](../plans/DRIVING_BEHAVIOUR.md#3-failure-first-acceptance-rows).

## Rows (`tests/w99_run_tests.cpp`, ctest `w99run`; W99 §5's fixture values)

| Row | Case | Asserts |
|---|---|---|
| BA230 | Accessors | `standstillGap` = `cc0` (1.5); `desiredGap` = `cc0 + cc1·v`; `stopLineReach` reads them |
| BA226 | Stop line, arriving at 10 m/s; and at rest 3.3 m short with `aPrev` −0.3 (inside `SDXc` 1.5 … `SDXo` 5.5, beyond `stopLineReach`) | Both are served (≥ 2 ticks at the line) and cross. Without the standstill creep the second never is (mutation) |
| BA226 | At rest 3.3 m short of a red head, `aPrev` −0.3, 60 s | It creeps, stops within `SDXc` of the head and never passes it |
| BA230 | Red head 1 m ahead at 15 m/s; D105 at gap 1.5 and 1.0 behind a leader pulling away; source D108 with the entry vehicle's rear 1.4 m in | Hard cap with a counted clamp; no positive acceleration; no departure |
| BA230 | 60 s red then green on 400 m (900 veh/h); an open 2 km platoon (600 veh/h) | > 50 trips discharged; the platoon never clamps; every vehicle carries `zW` and no W74 state |
| BA230 | Two vehicles ahead, the far one standing, `observedVehiclesAhead` 2 vs 1; `reactionTime` 0.3 s with a filled FIFO | Seeing two brakes harder; the published acceleration is the applied FIFO entry (next tick's `aPrev`) |
| BA230 | A35's lane-change road, every behaviour w99, 600 ticks | The courtesy obstacle is the kept result at least once; the hypothetical evaluations leave the vehicles unchanged |
| BA230 | Copied state, and two full runs at seed 42 | Equal vehicles and events |
| BA232 | Prototype → w99 → w74 → w99 by `segmentBehaviours`; entering w99 from the prototype and from W74 with `aPrev` −0.3, 0.3, 0 | The decision equals `w99Acceleration` with `aPrev` = the published acceleration (following keeps its sign); leaving W74 clears its state; on the W74 segment it is written again |

`w99codec`: a w99 behaviour used by a vehicle type, or assigned to a road, now compiles and runs with
trips completed and no `UNSUPPORTED_*` row. `behaviour-lab-ui`: with a prototype behaviour left, the lab
opens beside a car on w99 and refuses Apply; with none left it does not open, as with only W74 (D165).
Insertion passes `aPrev = 0` to its first-step check, and a new `Vehicle`'s acceleration is 0.

## Mutations

Each fails a case, then was restored:
- the standstill creep removed;
- `aPrev` always 0;
- `standstillGap` without w99;
- the dispatch skipped, so w99 falls to the prototype.

## Byte guards and limits

`multra-cli 42` summary and `--events` output byte-identical to the build before D641 (prototype only).
Shipped projects and catalogs hold no `w99`. No calibration, preset or validation claim. BA231's
timestep record is the next slice.
