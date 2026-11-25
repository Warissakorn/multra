# The stop laws under a reaction delay: options measured — 2026-11-25

Evidence for [REACTION_STOP](../reference/REACTION_STOP.md) (D659). D658 found that the line law (D652) and the
queue law (D656) were never measured with PERCEPTION's `reactionTime`. Nothing here is validated, and the
unvalidated marker stays.

## Method

- **The patch.** A temporary patch on `src/core/simulation.cpp` ([patch](reaction-stop-options.patch), reverted)
  selects an option with `ZZ_R`. Every option applies only to a vehicle whose behaviour has a reaction delay
  (`reactionTicks > 0`), so runs without one are unchanged by construction. It adds two traces:
  - `ZZ_TRACE` for each clamp's situation;
  - `ZZ_RED` for each tick a vehicle passes a red head because it cannot stop (M4.2's `committed`).

  That pass is not a `RedRunningEvent`, which only RED_RUNNING's release emits.
- **The probe.** D657's probe, with `ZZ_RT` setting every behaviour's `reactionTime` and `ZZ_LR` turning `lineRest`
  off. A copy also counts red-running events and trips.
- **Inputs.**
  - The four-leg and M2.6;
  - the project prototype (with `amberDeceleration`), the W74 fixture, the W74 SUMO preset and the W99 fixture;
  - seeds 42–51, dt 0.1, `reactionTime` 0.5 s (and 1 s on the four-leg).
- **Build:** Release, GCC 13.3.0, at `51c49ad`. `multra-cli 42` is byte-identical after the revert.

## What happens today

With a reaction delay, each tick's decision goes into PERCEPTION §5's FIFO and the one from `d` ticks earlier is
applied. The line law and the queue law are part of that decision, so they are delayed too:
- **Behind a car that has just stopped** (D656), the applied decision is from when it was still moving, and the
  law aims at exactly `s0` with no margin. The four-leg trace (W74, today) puts 673 of 755 clamps per run behind a
  vehicle at rest.
- **At a red line** (D652), the prototype's delayed law reaches the line too fast to stop, so M4.2 lets it pass on
  red: 128 passes per run on the four-leg at 0.5 s, 235 at 1 s, and 453 on M2.6. Its delay falls from 48 to 32 s
  (four-leg) because of it. With `lineRest` off it is 1.8 / 27.9 / 9.7.

## Options

| Option | For a vehicle with a reaction delay |
|---|---|
| R0 | Today |
| R1 | The queue law does not apply (its model follows a standing car, delayed) |
| R2 | `lineRest` does not apply at all (as the switch off) |
| R3 | The line law (`lineStopAcceleration`, lines and standing cars) is applied again after the delay, on the current state |
| R4 | The law aims short by the reaction distance `v·d·dt` |
| R5 | R1 and R3 |
| R6 | All of `applyLineStop` (the law and D258's floor) applied after the delay, lines and standing cars |
| **R7** | R6 for lines, R1 for standing cars |

## Results (per run, seeds 42–51)

### Four-leg, `reactionTime` 0.5 s

Clamps / crawl median (s) / moving hard braking / delay (s).

| Option | Prototype | W74 fixture | W74 SUMO | W99 |
|---|---|---|---|---|
| `lineRest` off | 12.1 / 18.0 / 2,994 / 48.07 | 23.1 / 35.7 / 716 / 114.29 | 31.6 / 50.4 / 675 / 143.22 | 554.7 / 2.1 / 1,398 / 52.36 |
| R0 | 11.4 / 0.8 / 7,118 / 32.47 | 755.2 / 2.5 / 12,582 / 110.63 | 910.3 / 3.1 / 14,398 / 133.60 | 390.8 / 1.4 / 9,818 / 52.11 |
| R1 | as R0 | 66.0 / 30.7 / 2,088 / 112.84 | 78.5 / 37.0 / 1,764 / 140.86 | 451.3 / 2.3 / 2,746 / 52.10 |
| R2 | as off | as off | as off | as off |
| R3 | 12.8 / 11.6 / 2,857 / 49.07 | 205.6 / 2.7 / 486 / 111.14 | 239.4 / 3.3 / 485 / 135.47 | 213.7 / 1.8 / 1,609 / 52.13 |
| R4 | 12.3 / 6.3 / 6,585 / 42.25 | 706.8 / 2.8 / 14,169 / 110.91 | 933.1 / 3.5 / 15,590 / 134.06 | 345.4 / 1.8 / 11,232 / 51.93 |
| R5 | as R3 | 5.2 / 30.4 / 382 / 112.86 | 6.6 / 37.3 / 505 / 141.52 | 443.8 / 2.3 / 873 / 52.24 |
| R6 | as R3 | 148.3 / 2.7 / 463 / 111.51 | 174.3 / 3.3 / 474 / 135.70 | 171.1 / 1.8 / 1,560 / 52.13 |
| **R7** | as R3 | **5.3** / 30.2 / 369 / 112.86 | **7.9** / 37.3 / 509 / 141.20 | 324.5 / 2.4 / 878 / 52.24 |

The prototype is outside the queue law (D656), so R1 = R0, and R3 = R5 = R6 = R7 for it.

**Where the clamps are** (`ZZ_TRACE`, four-leg, per run):

| | Behind a vehicle at rest | Behind a leader below 0.5 m/s | Behind a moving leader | No leader |
|---|---|---|---|---|
| W74, R0 | 673 | 13 | 46 | 23 |
| W74, R6 | 2 | 140 | 7 | 0 |
| W99, R0 | 318 | 5 | 36 | 32 |
| W99, R6 | 0.3 | 164 | 7 | 0.3 |

With R6 the delayed follower rests exactly at `s0`, so a leader edging forward and stopping again leaves it no
margin for a 0.5 s-old acceleration.

### M2.6, `reactionTime` 0.5 s

| Option | Prototype | W74 fixture | W74 SUMO | W99 |
|---|---|---|---|---|
| `lineRest` off | 51.2 / 17.6 / 12,600 / 47.34 | 91.3 / 60.1 / 2,170 / 172.67 | 67.8 / 71.5 / 1,525 / 207.58 | 2,410.1 / 2.2 / 6,219 / 50.52 |
| R0 | 43.6 / 0.7 / 25,983 / 29.70 | 3,276.9 / 3.0 / 45,859 / 149.69 | 3,625.7 / 3.7 / 50,090 / 176.06 | 1,505.8 / 1.4 / 38,133 / 50.05 |
| R6 | 46.7 / 8.7 / 11,692 / 47.00 | 591.1 / 3.2 / 1,454 / 147.49 | 696.9 / 3.7 / 1,348 / 168.69 | 740.8 / 1.8 / 6,856 / 50.19 |
| **R7** | 46.7 / 8.7 / 11,692 / 47.00 | **33.1** / 42.2 / 1,225 / 163.85 | **47.6** / 59.3 / 955 / 200.73 | 1,416.9 / 2.4 / 4,276 / 50.25 |

### Four-leg, `reactionTime` 1 s

| Option | Prototype | W74 fixture | W99 |
|---|---|---|---|
| `lineRest` off | 277.1 / 2.7 / 9,623 / 48.40 | 301.3 / 25.2 / 1,957 / 134.06 | 894.5 / 2.2 / 6,666 / 66.37 |
| R0 | 136.8 / 0.7 / 10,060 / 22.01 | 878.9 / 2.1 / 11,360 / 119.86 | 526.3 / 1.4 / 12,191 / 65.75 |
| R6 | 311.2 / 2.4 / 8,619 / 55.03 | 573.7 / 2.4 / 1,963 / 120.51 | 296.4 / 1.8 / 6,241 / 65.75 |
| **R7** | 311.2 / 2.4 / 8,619 / 55.03 | **34.6** / 28.0 / 1,218 / 138.55 | 655.4 / 2.7 / 6,409 / 65.98 |

### The prototype's passes on red (`ZZ_RED`, per run)

| Case | `lineRest` off | R0 | R7 |
|---|---|---|---|
| Four-leg, 0.5 s | 1.8 | 127.8 | 0 |
| Four-leg, 1 s | 27.9 | 235.1 | 0 |
| M2.6, 0.5 s | 9.7 | 452.6 | 0 |

Red-running events (RED_RUNNING's release) were 0 in every case measured. Trips were 505 per run under every
option on the four-leg.

## Reading

- **Today is wrong under a reaction delay.**
  - W74 clamps 36–53× its `lineRest`-off count on M2.6.
  - The prototype passes red lines hundreds of times per run. Its delay falls by a third as a result, which would
    read as a better junction.
- **Applying the line law after the delay (R3, R6, R7) removes the red passes.** A line's position is known well
  in advance, so aiming at it is the driver's judgement, as PERCEPTION §5 keeps for its hypothetical evaluations.
- **Behind a standing car, a delayed vehicle is safer with its model** (R1, R5, R7). Its own gap, wider than `s0`,
  absorbs the delay. Every option that rests it at `s0` (R0, R3, R4, R6) clamps behind leaders that edge off and
  stop again.
- **R7 is lowest for W74 everywhere:** 5–8 clamps per run on the four-leg and 33–48 on M2.6, fewer than with the
  switch off. Its cost is the crawl behind standing cars for delayed W74 vehicles: medians of 28–59 s, as with the
  switch off.
- **W99 clamps under a reaction delay whatever the line rules.** It clamps 555 / 2,410 per run (four-leg / M2.6)
  with `lineRest` off. R6 is lowest for it, at 171 / 741, and R7 gives 325 / 1,417. Its model under a delay is its
  own question (W99 §6 assumes the decision is acted on at once).
- **The prototype at 1 s** clamps 311 per run under R6/R7, against 277 with the switch off and 137 under R0. R0's
  figure is lower only because the prototype passes reds instead.
- **Runs without `reactionTime`** are unchanged by every option. No shipped project sets it.
