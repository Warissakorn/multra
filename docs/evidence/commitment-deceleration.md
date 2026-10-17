# B-2: commitment deceleration at give-way lines — 2026-10-17

Branch `claude/m3.4-lane-change-duration-slice1` on `a373e94` plus this change (D218, D219). Local
host: Linux, GCC 13.3.0, Ninja, Qt 6.4.2 offscreen, 4 cores. `--preset desktop` (Debug) and
`cmake --build build/desktop --target check`. **Linux only.** Contract:
[COMMITMENT](../reference/COMMITMENT.md).

## Rows (`tests/commitment_deceleration_tests.cpp`, ctest `commitdecel`, BA73–BA78)

The scenes are those of `commitment_tests`:
- a crossing zone: area 98–102, minor line at 90;
- a derived merge rule: minor line at 100, the join at 150.

In each scene the major vehicle is 25 m off at 10 m/s, which is 2.5 s, inside `gapTime` 3. The minor
vehicle is **8 m short at 10 m/s**:
- at the car's 8 m/s² it can stop within 6.25 m, so today it is held;
- at the key's 4 m/s² it needs 12.5 m, so it is committed.

| Row | Case | Asserts | Before the change |
|---|---|---|---|
| BA72 | Key absent | `commitment` group unchanged; shipped outputs byte-identical (below) | passes |
| BA73 | Merge rule, key 4 | Forcing: not committed at 8 m/s², committed at the key. Without the key: held at 100 while the major passes. With it: across first. No clamp either way | fails |
| BA74 | Crossing zone, key 4 | Forcing: anticipation alone closes the zone. `zoneHold` holds without the key and not with it; held at 90, or across; no clamp | fails |
| BA75 | Key 9 on a heavy vehicle (6) and on a car (8) | Every tick equals the key-absent run | passes (by design: today's rule is the minimum) |
| BA76 | Red head, `amberDeceleration` 3, key 2, 10 m short at 10 m/s | Forcing: the key alone would commit it. Every tick equals the key-absent run; held at the line; no clamp | fails at its forcing check |
| BA77 | BA73 with a W74 behaviour | The same hold and go | fails |
| BA78 | Codec | Schema 36 round-trips; 35 carrying the key → `EDIT_UNSUPPORTED_FIELD`; `"4"`, 0, 16 → `INVALID_BEHAVIOUR_PARAMETER`; 15 accepted; 37 → `EDIT_VERSION` | fails |

**How the scene was chosen.** The first version put the minor 7 m short, and both runs clamped. Even
with the key absent, the held vehicle stopped at the merge's line with a clamp, which its own
following could not avoid. The major then clamped behind it, at the join. A sweep (below) found 8 m
as the nearest clean scene.

Mutation checks (each reverted):
- **The key ignored** (the old rule): BA73, BA74, BA76 (its forcing check) and BA77 fail.
- **Without the `min`:** BA75 fails, because a heavy vehicle commits at 9 instead of 6.
- **The key also used at the signal head:** BA76 fails.

**The behaviour editor** has a `commitmentDeceleration` row in D217's either-model section. The
`behaviour-library-ui` row failed first (`FAIL: commitmentDecelerationSet`). The edited copy now
carries the key, so that test's file is schema 36. The Lab has no row (COMMITMENT §5).

Schema 36 is now valid, so the "future schema is refused" checks in eleven codec tests moved to 37.

`check` **167/167**.

## The sweep: where commitment starts to clamp

Scenes as above; the major is `T` seconds off; the minor is `short` metres from its line at 10 m/s.
Each cell shows whether the minor went (✓) or was held (·), with the clamp count after it.

| T | short | key absent (zone / merge) | key 4 | key 3 | key 2 |
|---|---|---|---|---|---|
| 2.5 | 8 m | · 0 / · 0 | ✓ 0 / ✓ 0 | ✓ 0 / ✓ 0 | ✓ 0 / ✓ 0 |
| 2.5 | 10 m | · 0 / · **1** | ✓ 0 / ✓ 0 | ✓ 0 / ✓ 0 | ✓ 0 / ✓ 0 |
| 2.5 | 15 m | · 0 / · 0 | · 0 / · 0 | ✓ **1** / ✓ 0 | ✓ **1** / ✓ 0 |
| 2.5 | 20 m | · 0 / · 0 | · 0 / · 0 | · 0 / · 0 | ✓ **1** / ✓ **1** |
| 2.9 | 15 m | · 0 / · 0 | · 0 / · 0 | ✓ 0 / ✓ 0 | ✓ 0 / ✓ 0 |
| 2.9 | 20 m | · 0 / · 0 | · 0 / · 0 | · 0 / · 0 | ✓ **1** / ✓ 0 |

- A driver who commits 15–20 m out, at 2–3 m/s², enters in front of the major vehicle, and someone
  clamps. This is D69's finding, reproduced (COMMITMENT §4).
- The key-absent clamp at 10 m short on the merge is today's behaviour, independent of the key: the
  held stop at the line.

## Shipped projects (`--seeds 42–51`)

**Key absent:** byte-identical to `a373e94`. This was checked for:
- `trafficsim-cli 42`;
- the JSON (without `buildCommit`) of all four shipped projects, compared against the scratchpad
  D210 baselines.

**With the key** on every behaviour (a scratchpad copy of `data/`):

| Project | Key | Mean delay (s, ±95 %) | Completed | Safety clamps per seed (mean) |
|---|---|---|---|---|
| `t-junction-priority` | absent | 6.45 ± 0.31 | 272.3 | 0.2 |
| | 6 | 6.36 ± 0.25 | 272.3 | 0.0 |
| | 4 | 6.22 ± 0.29 | 272.3 | 0.1 |
| | 2 | **5.65 ± 0.28** | 272.3 | **0.8** |
| `m2.6-study-template` | absent | 46.40 ± 0.88 | 1959.5 | 1.1 |
| | 6 | 46.40 ± 0.88 | 1959.5 | 1.2 |
| | 4 | 46.39 ± 0.88 | 1959.5 | **3.2** |
| | 2 | 46.38 ± 0.88 | 1959.5 | **8.3** |

- At the T-junction, a lower value lowers delay, as the contract expects (earlier commitment
  accepts shorter gaps). At 2 m/s², clamps rise.
- On M2.6 the overall delay does not move, but clamps rise to 3.2 at 4 m/s² and 8.3 at 2 m/s².
- The report counts clamps per seed, not by road. Whether they fall on the major road is not split
  here, so a study using the key must look at its own clamp events. No value ships (D178).

## Performance (callgrind Ir, Release `-g`, `four-leg-signalised` seed 42, key absent)

| Build | Ir |
|---|---|
| This change | 886.46 M |
| The same build with the two give-way sites calling the type-only test | 886.13 M |

The difference is +0.04 %.

## Not covered here

- Clamp attribution by road in the report.
- Commitment at the signal head (B-9, red-light running).
