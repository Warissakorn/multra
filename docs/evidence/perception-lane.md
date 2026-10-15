# M3.5 slice 3: look-back, rear TTC and perceived lane-change evidence — 2026-10-15

Branch `claude/eloquent-carson-qgorzf` on `5643c94` plus this slice (D183). Local host: Linux,
GCC 13.3.0, Ninja, nlohmann/json 3.11.3, Qt 6.4.2 (offscreen). `--preset desktop` 141/141 CTest
through `cmake --build build/desktop --target check` (architecture, file sizes, docs included).
**Linux only**; Windows evidence belongs to the PR's native CI. Contract:
[PERCEPTION](../reference/PERCEPTION.md) §3, §4, §7, §8; decisions D179, D182, D183.

## What changed

All in `src/core/lanes.cpp`, on the **changer's** behaviour keys:

- `around` drops the nearest follower when it is further back than `lookBackDistance` (equal is
  seen). With `lookAheadDistance` or `observedVehiclesAhead` it also lists the perceived leaders
  on the target lane in the same scan, including this tick's accepted changes, through
  `notePerceived` (`src/core/perception.cpp`, the slice-2 tie rule). Alongside stays physical.
- `safeAt`'s forward test, with a forward key, takes §3's lowest acceleration over the perceived
  leaders (`anticipate`) and checks the one-tick room against the nearest seen one. With none
  seen there is no forward test.
- `safeAt`'s rear test refuses a seen follower closing on the changer with
  `gap / (vF − v) < rearTimeToCollision`; equal accepts.
- The discretionary gain uses the perceived leaders on both lanes.
- `courtesyHolds` is unchanged. Own motion's multi-anticipation loop moved into `anticipate`
  unchanged, and both callers share it.

## Rows (`tests/perception_lane_tests.cpp`, ctest `perceptionlane`, 6 tests)

| Row | Case | Asserts |
|---|---|---|
| BA33 | A29's road: changer at 50 m (5 m/s), target follower at 40 m (15 m/s), gap 5.5 m | Forcing: refused without the key, the follower's own `follow` is below −2 m/s². `lookBackDistance` 5.5 (equal): refused. 5: accepted at once; then, over more than 50 ticks to the end, the follower's front never passes the changer's rear (clamps counted, not asserted absent) |
| BA33 | A28's road: a vehicle alongside the target place | Refused without the key and with `lookBackDistance` 0.1 |
| BA34 | Changer at 60 m (10 m/s), follower at 40 m (12 m/s): TTC 15.5 / 2 = 7.75 s | Forcing: accepted without the key. Key 7.8 refuses; 7.75 and 7.7 accept; a follower at 10 m/s with key 100 accepts; with `lookBackDistance` 15 the follower is unseen and key 7.8 accepts |
| §4 forward | Target lane: a car 20 m ahead at 10 m/s, a standing one at rear gap 30.5 m | Forcing: `follow` against the first alone ≥ −2, against the second at 30.5 − (4.5 + 2) = 24 m < −2. Without a key and with `observedVehiclesAhead` 1: accepted; with 2: refused |
| §7 gain | D95's road: the vehicle 15.5 m behind a 3 m/s one on L1, a car standing 20 m ahead of its place on L2 | Forcing: gain against the standing car < 0.5, free gain ≥ 0.5. No change without the key; with `lookAheadDistance` 18 L2 looks free and it changes |
| BA30 | Four-leg project, seed 42, `lookBackDistance` 1e9, `observedVehiclesAhead` 1, `lookAheadDistance` 1e9 on every behaviour (asserted different first) | Vehicles and events equal the key-absent run on **every** tick; lane changes occurred (> 0); next id equal |

**Mutation check.** With `src/core/lanes.cpp` reverted to `5643c94` and the tests kept, four of
the six fail (the look-back, TTC, forward and gain tests). The alongside and identity tests pass
there too, as they should.

`perceptioncodec`'s BA30 test now sets only `reactionTime`, the one key nothing reads yet, and
still finds the run unchanged. The CLI pin holds: `trafficsim-cli 42` = 45 completed /
31.75870154816726 s / 1 clamp.

## Performance (§8)

`trafficsim-engine-benchmark 40 1800` (40-crossing corridor, 1,800 s, 12,065 trips, peak 626
vehicles; no perception key set), Release `-O3`, same machine, six interleaved pairs of the binary
before and after this slice:

| | Runs (ms) | Median |
|---|---|---|
| Before (`5643c94`) | 2909, 3307, 2961, 2994, 2996, 3060 | ≈ 2995 |
| After | 2997, 3161, 2933, 2896, 2849, 2876 | ≈ 2915 |

No regression: the difference is inside the spread of either binary. Wall clock on a shared
container: compare only within this table.

## Not covered here

A run with the keys set is not compared against field data. Nothing here validates look-back or
the rear TTC (rule 4). A second follower closing fast behind a slow first one is not tested (§4).
Reaction time is slice 4.
