# M3.4 slice 1: lane-change duration evidence — 2026-10-16

Branch `claude/m3.4-lane-change-duration-slice1` on `5847cb9` plus this slice (D206). Local host:
Linux, GCC 13.3.0, Ninja, Qt 6.4.2 offscreen, 4 cores. `--preset desktop` (Debug) and
`cmake --build build/desktop --target check`. **Linux only**; Windows evidence belongs to the PR's
native CI. Contract: [LANE_CHANGE_DURATION](../reference/LANE_CHANGE_DURATION.md) §2–§7; decisions
D204, D206.

## Rows (`tests/lane_change_duration_tests.cpp`, ctest `lanechangeduration`, 7 tests)

A two-lane straight Link from `tests/lateral_test_support.hpp` whose left lane is made a stub: a
`routeDeadEnds` entry and one mandatory span onto the right lane. Keys 0/0 and `maxLateralSpeed`
1 m/s at dt 0.1 s. Every row asserts its forcing first. BA49–BA53 failed on `5847cb9` (the change
was instantaneous); BA48 pins today's switched-off behaviour and passed before and after.

| Row | Case | Asserts |
|---|---|---|
| BA48 | The stub without keys (switch asserted off) | Remapped the tick it is accepted; one event; no target, `lateral` 0 |
| BA49 | Switched on, a mandatory change at 10 m/s | Accepted as a target of −3.5 m, route unchanged, no event; ≥ 35 ticks; one `LaneChangeEvent` at the crossing; ends centred on the right lane with the target cleared on the tick it arrives; `lastLaneChange` = the crossing tick; trace flagged `lateral` |
| BA50 | A 5 m/s changer, 8 m/s followers on both lanes | The body covered both lanes (forcing); each follower stays behind the changer's rear while it covers that lane; no 2D overlap at any tick |
| BA51 | A zone line 0.01 m under, then at, `travel` ahead, on the own then the target route | Under: no start; at: starts |
| BA52 | The span ending 0.01 m short of, then at, the crossing (`front + speed·|Δy|/(2·rate)`); a changer standing 1 m short of its dead end; a vehicle with a target set | Short: no start; at: starts; the standing changer starts and crosses (D206); the vehicle with a target keeps it and changes nothing |
| BA53 | A centred car placed alongside on the target lane mid-change, one step from contact | Move refused, `lateral` kept, `lateralSpeed` 0, target kept; resumes once it is removed; no overlap. Display: D203's `lateral_display_ui_tests` already pins that a `lateral` crossing is drawn at the engine position and never slid |
| D206 | Three lanes, both outer lanes stubs onto the middle; the left changer under way with its body not yet over the line | The right changer level with it does not start |

Mutation checks (each reverted): without the zone rule BA51 fails; without the window rule BA52;
without one-change-at-a-time BA52; without clearing on arrival BA49; without the held target place
the D206 row.

**BA39's replay test changed** (`lateralspan`): switched on, `four-leg-signalised` now changes lanes
laterally, so its run differs from the lane-based one by design. The test replays the two runs
exactly up to the first tick a vehicle has a target, and asserts that tick is reached. BA39 itself
(every key absent) is unchanged: `check` **162/162**, and `trafficsim-cli 42` = 45 completed /
31.75870154816726 s / 1 clamp, output byte-identical to `5847cb9`.

## Switched-on product runs (contract §9)

Keys 0.5/1.0/1 on every catalog behaviour (a copied `data/` directory), `trafficsim-cli --data-dir
<copy> --project <p> --seeds 42-51`. `lane-change-lab.traffic.json` sha256 `5fb3147278c84d9b…`,
`four-leg-signalised.traffic.json` `3b0eb4450904ed66…`.

| Run | Completed per seed (42–51) | Safety clamps |
|---|---|---|
| lane-change-lab, off | 885, 893, **680**, 899, 887, 865, 865, 852, 860, 864 | 0 |
| lane-change-lab, on | 885, 893, **693**, 899, 887, 865, 865, **626**, 860, 864 | 2 (seed 49) |
| four-leg-signalised, off | 440, 458, 456, 477, 468, 468, 440, 397, 461, 455 | 0 |
| four-leg-signalised, on | 440, 458, 456, 477, 468, 468, 440, 396, 461, 455 | 0 |

The first implementation, exactly as D204 wrote the start rules, failed this check in two ways,
both corrected in D206:
- **The window rule counted the body's length.** `front + travel/2 ≤ fromEnd` with `travel`
  including `length` can never hold for a changer standing within `length/2` of its dead end, so
  four-leg seed 42 jammed an approach (97 vehicles active against 54). The crossing needs only the
  front's travel during the move: `front + speed·|Δy|/(2·rate)`.
- **A change under way was invisible to other starts** until its body crossed the line. Two
  changers converging on one lane from both sides each passed their start test and met 5 cm apart
  (lab seed 43: three clamps at −97.5 m/s², 19 vehicles stranded). A changer now holds its target
  place in every other vehicle's start test for the whole change.

Two findings remain, both recorded in NEXT:
- **A swap deadlock at shared dead ends, pre-existing.** In the lab's scene where two movements'
  stubs end at the same station on adjacent lanes, each heading for the other's lane, both heads
  stand at 598 m of 600 m and each is "alongside" the other's target place, so neither ever
  changes and both approaches jam. Switched off it happens in seed 44 (183 vehicles active);
  switched on in seeds 44 and 49. The duration moves which seeds deadlock; it does not cause it.
- **A lateral move ignores gaps along the lane** (two clamps, lab seed 49). A faster vehicle on the
  target lane can pass a changer whose body has not yet reached the line; the changer then crosses
  in 2.9 m behind it. Admission tests 2D overlap only (LATERAL §7), and the contract checks
  safety only at the start (§4); a per-tick check or abandoning is out of this slice's scope (§9).

## Performance (contract §7, LATERAL §11)

Release (`-O3`), five interleaved pairs, both binaries built in the same build directory (a second
worktree's build ran 15 % apart with identical source, so cross-directory pairs are not used).
Shared 4-core container; medians:

| Run | `5847cb9` | This slice | Change |
|---|---|---|---|
| `trafficsim-engine-benchmark 40 1800` run time (switch off) | 4131 ms | 4111 ms | none |
| `lane-change-lab`, switched on, seeds 42–51, `--threads 1`, process wall | 2356 ms | 3125 ms | **+33 %** |
| `four-leg-signalised`, switched on, same | 1185 ms | 1383 ms | **+17 %** |

Switched on, both exceed the 10 % budget. Before this slice no product run moved sideways, so the
baseline is the instantaneous change; the cost is `moveLaterally`'s per-tick rebuild of references,
spans and buckets on every tick with any target, plus each move's span check. D206 records the
regression as owed, not accepted: no shipped project carries lateral keys, and an engine round
is due before one does.

## Not covered here

- Discretionary lane changes take the same path (same start rules, same target) but have no
  row of their own; they are off in every shipped project (D95/D101).
- No Windows, desktop-visual or high-DPI run.
