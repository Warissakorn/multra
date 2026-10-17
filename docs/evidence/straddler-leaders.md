# A straddler follows the other lane's leaders — 2026-10-16

Branch `claude/m3.4-lane-change-duration-slice1` on `53ba453` plus this change (D210). Local host:
Linux, GCC 13.3.0, Ninja, Qt 6.4.2 offscreen, 4 cores. `--preset desktop` (Debug) and
`cmake --build build/desktop --target check`. **Linux only.** Contract:
[LATERAL](../reference/LATERAL.md) §6 ("Leader").

## The failure

With the lateral switch on, a body over a lane line casts `own = false` spans on the lane it covers,
so that lane's followers see it (D188). Its own leader search did not look back: `closestVehicle` and
`perceivedLeaders` walked only the straddler's route parts. A straddler could therefore drive up
beside, or into, a slower car on the lane it covers. D208's step back hid this only while a change
waited ([lane-change-gap](lane-change-gap.md), "Not covered here").

## Rows (`tests/straddler_leader_tests.cpp`, ctest `straddlerleader`, BA55, 4 tests)

A two-lane Link, keys 0.5/1.0. The straddler (vehicle 1) is on the right lane at 50 m, 12 m/s, held
at lateral 1.6 m with no target, so it never moves sideways. Its body is 0.75 m over the line. Vehicle 2
drives 5 m/s on the left lane, 0.3 m toward the line, so the two bodies overlap across the road. Each
row asserts its forcing first: vehicle 1 casts a lateral copy on the left lane, has nothing ahead on
its own lane, and has no target.

| Row | Case | Asserts | On `53ba453` |
|---|---|---|---|
| 1 | Vehicle 2 40 m ahead | No 2D overlap, no clamp; still behind after 30 s at 5 ± 0.5 m/s; lateral unchanged | 2D overlap |
| 2 | Control: the straddler centred | It passes vehicle 2 (not its leader); no overlap, no clamp | passes |
| 3 | The road cut at 150 m (a Connector lane path to a second Link); vehicle 2's rear past the cut | From the first tick the physical leader is vehicle 2 at the bodies' gap; no overlap, no clamp, still behind | 2D overlap |
| 4 | Row 1 with `observedVehiclesAhead = 2` | No clamp, no overlap; speed 5 ± 0.5 | 2D overlap |

Mutation checks (each reverted):
- Without the physical search on the covered lane, rows 1 and 3 fail.
- Without the perceived search, row 4 fails: its acceleration ignores the car and the hard cap clamps.
- Without the successor walk, row 3's first-tick leader check fails. A first version of row 3
  asserted only the run, and it survived this mutation: the straddler caught the car after both were
  on the second Link.

## BA54 row 4 restated

D208's step-back row put the changer's front 1 m behind a slower car on the lane it covered. That is
inside the 2 m standstill distance. With D210 the changer follows that car, so the hard cap stopped
it from 8 m/s in one tick (a clamp), and it then completed its change. A straddler that follows
the lane it covers can no longer reach that start. The row now starts the car 4 m ahead. Following
there brakes at −4.84 m/s²: beyond the comfortable 2 m/s², so the move must wait, but inside the
maximum, so there is no clamp. The changer steps back within two ticks and never covers the lane
again. Without the step back, the row still fails.

`check` **164/164**.

## Runs (`--seeds 42-51`)

- **Switched off:** byte-identical to `53ba453`. This covers `trafficsim-cli 42` and the JSON
  (without `buildCommit`) of all four shipped projects.
- **Switched on** (keys 0.5/1.0/1 on every behaviour), against D208/D209:

| Project | Clamps | Completed / active | Mean delay |
|---|---|---|---|
| `lane-change-lab` | 0 → 0 | unchanged in every seed | all 10 seeds move, by ≤ 0.04 s |
| `four-leg-signalised` | 0 → 0 | unchanged in every seed | 5 seeds move, by ≤ 0.29 s (seed 43 −0.29, seed 51 −0.23) |

## Performance

Callgrind Ir, Release `-g`, seed 42, switched on:

| Project | `5847cb9` | D209 | D210 | D210 vs `5847cb9` |
|---|---|---|---|---|
| `lane-change-lab` | 1568 M | 1623 M | **1662 M** | **+6.0 %** |
| `four-leg-signalised` | 998 M | 1028 M | **1046 M** | **+4.8 %** |

The cost is `followerFilter` listing the covered lanes for every vehicle each tick; a body inside its
lane's clear band (D209) skips that loop. Both stay inside LATERAL §11's 10 %.

Wall time compares five interleaved pairs, both binaries from one build directory, `--seeds 42-51
--threads 1`; D209 → D210 medians:

- `lane-change-lab`: 1813 → 1901 ms (+4.9 %).
- `four-leg-signalised`: 1211 → 1232 ms, within noise; the ranges overlap.

**Switched off, not measured here:** moving `closestVehicle` out of line cost the switched-off
four-leg run 1.7 % Ir (879 → 894 M), against LATERAL §11. D212 found it and recovered most of it
([curve-speed](curve-speed.md), "Performance").

## Not covered here

- The walk along the covered lane follows a segment's only successor. At a fork it stops, because
  the straddler has no route on that lane to choose by. A leader beyond such a fork is not seen until
  the straddler's front reaches it.
- Covered lanes are taken at the body's front segment only, as the filter's band is (§6).
