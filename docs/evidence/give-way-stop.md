# A feasible stop at a give-way line — 2026-10-18

Branch `claude/m3.4-lane-change-duration-slice1`, on `1080fa3` (D239's contract) plus this change (D240).

- **Host:** Linux, GCC 13.3.0, Ninja, Qt 6.4.2 offscreen, 4 cores.
- **Build and check:** `--preset desktop` (Debug), then `cmake --build build/desktop --target check`.
- **Linux only.**
- **Contract:** [GIVE_WAY_STOP](../reference/GIVE_WAY_STOP.md), a slice of ROADMAP M3.2.8a.1. No gate
  is closed, and no clamp-free claim is made.

## Rows (`tests/give_way_stop_tests.cpp`, ctest `givewaystop`)

The scenarios:
- **The crossing** of the commitment tests: an area at 98–102 on both roads, the minor line at 90,
  and a 3 s gap time with a 10 m headway.
- **A merge** whose minor waits at 100.

In each, a major vehicle 28 m short of the area at 15 m/s holds the line. "Before" is the build of
`1080fa3`.

| Row | Case | Asserts | Before |
|---|---|---|---|
| BA146 | 20 cm short at 1 m/s (no clamp today) | No clamp, short of the line; every one of 20 positions equal, bit for bit, to the pinned positions from `1080fa3` | passes (it pins today) |
| BA147 | Yield zone, 2 cm short at 0.5 m/s (needs 6.25 m/s²) | No clamp; never past the line; hardest braking ≥ −8 m/s² | fails (one clamp) |
| BA148 | Priority-rule line, the same | The same | fails |
| BA149 | Stop zone, the same | No clamp; never past the line | fails |
| BA150 | 1 cm at 0.5 m/s (needs 12.5 m/s²) with a standing major inside the area | Still one clamp | passes (the feasibility mutation fails it) |
| BA151 | A standing car whose buffer leaves 1 cm, nearer than the 2 cm line | Still one clamp | passes (the binding-line mutation fails it) |

The placement was found by a scan over gaps of 1–40 cm and speeds of 0.2–2 m/s. Today, 2 cm at
0.5 m/s and 1 cm at 0.2 m/s clamp, though stopping there needs only 6.25 and 2.0 m/s².

**Mutations,** each removed in turn:

| Mutation | Caught by |
|---|---|
| No feasibility test | BA150 |
| No binding-line test | BA151 |
| Rule holds not recorded as lines | BA148 |
| Zone holds not recorded as lines | BA147, BA149 |
| No bound of the move by the line | not caught. The bound was then removed: the stop ends at the line up to rounding, which `kPast` tolerates. The contract says so |

`check` **178/178**: 177 tests plus the new `givewaystop`.

**First sanitizer run.** CI `linux (sanitizer)` failed BA151 on `b81635f` with heap-use-after-free.
The row held a reference into the temporary scenario that `crossing()` returned. The row now keeps
the scenario in a local; the engine was not involved. Linux Debug had passed by chance.

## Shipped runs

**Byte-identical** against D210's and D225's baselines, as the contract requires (no give-way line
clamps there):
- `trafficsim-cli 42`;
- `four-leg-signalised`, `m2.6-study-template` and `lane-change-lab` at seeds 42–51.

**`t-junction-priority` moves,** seeds 42–81, before and after in the same build directory:

| Measure | Before | After |
|---|---|---|
| Safety clamps (seeds) | 6 (46: 2, 74: 1, 79: 2, 80: 1) | 0 |
| Mean delay (s) | 7.0694 | 7.0694 |
| Completed trips (mean) | 274.925 | 274.925 |
| Movement delays | — | unchanged, every digit |
| Queue counter mean length (m) | 2.532912184 | 2.532912232 |

Nothing else in the batch JSON changed.

**Lane-change lab,** seeds 42–81: byte-identical apart from `buildCommit`. Its one clamp (seed 74)
is not at a give-way line and remains.

**The replaced ticks.** A temporary probe at the new branch, never committed, recorded 11 ticks over
the 40 seeds. Each case is one or two consecutive ticks at the same deceleration:
- 2.50, 4.44, 5.02, 6.29, 6.33 and 7.38 m/s², all under the car's 8 m/s²;
- speeds 0.15–1.02 m/s;
- gaps 1.6 mm – 8.2 cm.

**Replay.** Two runs of the after build at seeds 42–81 are byte-identical to each other and to the
comparison run. The existing overlap checks (`conflict_zone`, `commitment`, `tjunction` groups) pass
in `check`.

**Correction to D232.** D232 wrote that the 6 Yield clamps had none at seeds 42–51. Seed 46 has 2,
and D225's ten-seed baseline does show them: a mean of 0.2 per seed.

## Performance (callgrind Ir, Release `-g`, seed 42)

Both builds were made in the same build directory.

| Project | `1080fa3` | This change |
|---|---|---|
| `four-leg-signalised` | 886.53 M | 887.52 M (+0.11 %) |
| `t-junction-priority` | 194.09 M | 194.37 M (+0.15 %) |

The step keeps one more local per vehicle and one comparison per hold.

## Not covered here

- Signal heads: the red-onset clamp (D227) stays recorded.
- The other M3.2.8a.1 cases: moving merge clamps behind a shared leader's standstill buffer (D105)
  and the standstill cap that treats a moving leader as standing (D201 B5).
- The lane-change lab's seed-74 clamp.
