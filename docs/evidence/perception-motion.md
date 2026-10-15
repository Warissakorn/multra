# M3.5 slice 2: look-ahead and multi-anticipation evidence — 2026-10-15

Branch `claude/eloquent-carson-qgorzf` on `2db3ee3` plus this slice (D182). Local host: Linux,
GCC 13.3.0, Ninja, nlohmann/json 3.11.3, Qt 6.4.2 (offscreen). `--preset desktop` 140/140 CTest
through `cmake --build build/desktop --target check` (architecture, file sizes, docs included).
**Linux only**; Windows evidence belongs to the PR's native CI. Contract:
[PERCEPTION](../reference/PERCEPTION.md) §2, §3, §8; decisions D179, D182.

## What changed

`perceivedLeaders` (`src/core/perception.cpp`) returns up to `observedVehiclesAhead` distinct
vehicles ahead whose rear gap is at most `lookAheadDistance`, nearest first, scan order on equal
gaps. Own motion (`src/core/simulation.cpp`) starts its obstacle merge from the nearest perceived
leader instead of the physical one, and keeps the lowest of `follow` over the further perceived
leaders at `g_k − Σ_{j<k}(l_j + standstillGap)`. The physical leader still sets `allowedDistance`,
zone admission and the D108 source test. Neither key set: no extra search, the physical leader as
before.

## Rows (`tests/perception_motion_tests.cpp`, ctest `perceptionmotion`, 5 tests)

| Row | Case | Asserts |
|---|---|---|
| BA31 | Follower at 25 m/s, standing car 150 m ahead; first the key-absent run | Without the key: `approaching` on the first tick (the forcing). With `lookAheadDistance` 100: `free` on every tick whose start gap is > 100 m, not `free` on the first tick at ≤ 100 m; the front never passes the standing car's rear; the standing car never moves; it closes to between 2 and 3 m at < 0.05 m/s (the prototype creeps toward its standstill gap; no clamp occurred, and clamps are counted, not asserted absent) |
| BA31 | Rears at exactly 100 m and at 146 m | Range 100 returns the 100 m vehicle with gap 100 exactly; range 99.999 returns none |
| BA32 | F 10 m/s; L1 gap 20 m at 10 m/s; L2 standing at gap 45 m | n = 2 brakes strictly harder than n = 1; its acceleration equals `follow` against `Leader{39, 0, 0}` (45 − (4 + 2)) to 1e-12, mode `approaching`; n = 1 equals the key-absent run |
| BA32 | A vehicle straddling a section cut and two more ahead | One entry per vehicle, gap 38 from its rear span; nearest first; count 1 with no range returns the physical leader |
| BA32 / BA30 | Four-leg project, seed 42, `observedVehiclesAhead` 1 and `lookAheadDistance` 1e9 on every behaviour (asserted different first) | Vehicles, events, time and next id equal the key-absent run |

`perceptioncodec`'s BA30 test now sets only the keys no motion reads yet (`lookBackDistance`,
`rearTimeToCollision`, `reactionTime`) and still finds the run unchanged. The CLI pin holds:
`trafficsim-cli 42` = 45 completed / 31.75870154816726 s / 1 clamp.

## Performance (§8)

`trafficsim-engine-benchmark 40 1800` (40-crossing corridor, 1,800 s, 12,065 trips, peak 626
vehicles; no perception key set), Release `-O3`, same machine, six interleaved pairs of the
binary before and after this slice:

| | Runs (ms) | Median |
|---|---|---|
| Before (`2db3ee3`) | 3372, 2765, 2761, 2867, 3009, 2851 | ≈ 2859 |
| After | 2912, 2855, 2867, 3087, 3016, 2835 | ≈ 2889 |

About +1 %, inside the spread of either binary; under §8's 10 % budget. Wall clock on a shared
container: compare only within this table.

## Not covered here

A run with the keys set is not compared against any field data; nothing here validates
look-ahead or multi-anticipation (rule 4). `safeAt`'s forward test, look-back, the rear
time-to-collision and reaction time are slices 3–4 (BA33–BA36); BA38 is pending.
