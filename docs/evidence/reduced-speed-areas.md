# B-6 slice 2: reduced-speed areas — 2026-10-17

Branch `claude/m3.4-lane-change-duration-slice1`, on `b4d5955` plus this change (D224, D225).

- **Host:** Linux, GCC 13.3.0, Ninja, Qt 6.4.2 offscreen, 4 cores.
- **Build and check:** `--preset desktop` (Debug), then `cmake --build build/desktop --target check`.
- **Linux only.**
- **Contract:** [SPEED_DECISIONS §7](../reference/SPEED_DECISIONS.md#7-reduced-speed-areas-d224).

## Engine rows (`tests/speed_area_tests.cpp`, ctest `speedarea`)

The rows use hand-built roads. The car's desired-speed range is 11.11–16.67 m/s and its comfortable
deceleration is 2 m/s². Each car's desired speed is set from a quantile.

"Before the change" is the same rows run against a stub, `speed_area.cpp` with functions that do
nothing.

| Row | Case | Asserts | Before the change |
|---|---|---|---|
| BA97 | No area | No `areaSpeed` in the checkpoint | passes |
| BA98 | Quantiles 0.2 and 0.8 inside `{car: 8–12}` | Speeds 8.8 and 11.2 (±1e-6); desired speed untouched | fails (12.22 vs 8.8) |
| BA99 | A `{car: 5–5}` area at 200 m, with `deceleration` 1, then without it | At 200 m the speed is ≤ 5.05, with no clamp. Braking stays within 1 m/s² (with the key) and within 2 m/s² (without it); each run brakes harder than 0.9 and 1.5 m/s² respectively | fails |
| BA100 | Leaving `[50, 150)` | Back to the desired speed; `areaSpeed` 0; desired speed never changed | fails |
| BA101 | Lanes | **Uncovered lane:** a car on lane b is not held by an area on lane a. **Changing out:** a car on stub a is held inside beside an unlisted heavy vehicle, then changes to b and is released. **Changing in:** a car changing into the covered stretch of b takes the area | fails |
| BA102 | A heavy-vehicle-only area | The car is unaffected | passes |
| BA103 | A decision inside an area | Decision `{20–24}`: desired 20.8, speed 8.8. Decision `{6–10}`: speed 6.8, quantile 0.2 | fails (20.8 vs 8.8) |
| BA104 | A lone car through `[100, 300)` at 5 m/s | Free-flow time `100/v + 200/5 + 100/v`; delay < 5 s | fails (26.66 vs 53.33) |
| BA105 | Section 100–350 m inside `[50, 380)` | Section travel time 50 s; delay < 0.5 s | fails (16.67 vs 50) |
| BA106 | Checkpoint inside the area | Carries `areaSpeed` 8.8; replays exactly | fails |

The rows also caught three faults in the first implementation:

| Fault | Caught by |
|---|---|
| The anticipation loop broke off early: its test used the largest possible deceleration, but a gentler deceleration reaches further, so the test needs the smallest | BA99: the car reached 200 m too fast |
| The acceleration cap also applied above the target, where it braked in one tick | BA99: braking harder than 1 m/s² |
| No acceleration cap at all: speed was clamped to the target, but the distance was integrated with the model's acceleration | BA105: section 48.8 s instead of 50 |

The third fault also exists in the curve bound of D212. That bound clamps speed in the same way
without capping acceleration. It is not fixed here, because the fix would change switched-on curve
runs. It is listed in NEXT.

## Codec and commands (BA107, BA108)

These rows were written after the code, so each hook was removed in turn. The script is in the
session scratchpad.

| Mutation | Caught by |
|---|---|
| No parse | BA107 |
| No writer | BA107. It first caught this only as an abort, on reading the missing array. A `contains` check now fails the row by name |
| No spec validation | BA107 |
| No compile | BA107 |
| No overlap check | BA107 |
| No prune on Link delete | BA108 |
| No reverse guard | BA108 (on a lone Link; nothing else holds it) |
| No split refusal | BA108 |
| No split move | BA108 |
| No lane-id map on split | BA108 |
| No lane-drop guard | BA108. Validation then refuses the result with `INVALID_LANE` instead of `EDIT_REFERENCED_LANE` |

BA107 also runs the four-leg project with an area on one lane of `link-1`. The run compiles to one
lane section, and the mean travel time rises.

## Editor (`tests/speed_area_ui_tests.cpp`, ctest `reduced-speed-area-ui`)

The row failed first (`No reduced-speed area table`). It checks:
- **Add** is disabled until exactly one Link is selected;
- the dialog's name, from, to and optional deceleration;
- lane boxes, all ticked by default. Unticking one writes the other lane ids. Ticking all writes no
  list, so the area also covers lanes added later. Ticking none is refused, because an empty list
  would mean every lane;
- Add, Edit and Delete are each one Undo step;
- one shaded band is drawn per covered lane;
- an inverted stretch is refused and makes no new revision;
- the tab title is translated into Thai.

`workspace-ui` and `object-groups-ui` now expect 16 pages, with the new tab under *Demand*.

`check` **172/172**.

## Shipped runs

**Without an area, every shipped output is byte-identical to `b4d5955`:**
- `trafficsim-cli 42`;
- the `--seeds 42–51` JSON (excluding `buildCommit`) of all four shipped projects.

**With an area:** a scratchpad copy of `four-leg-signalised` with a 30 km/h area on the west approach,
`link-1`, from 20 m to 200 m. Cars get 8.33–9.72 m/s and heavy vehicles 7.5–8.33 m/s. No deceleration
key is set. Lane-2 carries the West → North traffic and lane-3 the West → South traffic; West → East
uses both. Seeds 42–51, travel time / delay in seconds:

| Movement | No area | Lane-2 only | Lane-3 only | Every lane |
|---|---|---|---|---|
| West → East | 89.4 / 46.5 | 93.1 / 46.7 | 93.2 / 46.9 | 97.2 / 47.4 |
| West → North | 85.0 / 42.7 | 93.6 / 44.6 | 85.9 / 43.4 | 94.5 / 45.3 |
| West → South | 82.1 / 39.0 | 82.0 / 38.9 | 91.8 / 42.0 | 91.8 / 41.9 |
| Other approaches | — | unchanged | unchanged | unchanged |
| Safety clamps | 0 | 0 | 0 | 0 |
| Completed trips (mean) | 452.0 | 451.0 | 451.5 | 450.3 |

- **The lanes act separately.** An area on lane-2 slows the North movement by about 8.6 s and leaves
  the South movement alone; on lane-3 it is the other way round.
- **Delay rises much less than travel time.** On a covered lane, travel time rises by about 8–10 s
  but delay by only 1–3 s, because the slower stretch is not counted (the owner's rule). The remainder
  is the braking before the area, the acceleration after it, and queueing.
- Unlike D223's decision, the effect ends at 200 m, so travel time rises by about 9 s instead of
  26 s.
- **No clamp** appears with any variant. D223's seed-47 red-onset clamp does not occur here, because
  the area ends 20 m before the line.

## Performance (callgrind Ir, Release `-g`, `four-leg-signalised` seed 42, no area)

All three builds used the same build directory.

| Build | Ir |
|---|---|
| `b4d5955` | 886.94 M |
| This change | 893.72 M (+0.77 %) |
| This change with both run-time hooks removed | 889.23 M (+0.26 %) |

- **The new `Vehicle` field** (`areaSpeed`), which every tick copies, costs +0.26 %.
- **The hooks cost about +0.5 %,** and the cost is spread over unrelated lines of `stepSimulation`.
- **Three rewrites did not lower it:** reading the switch once per tick, returning the bound by value
  instead of through a reference, and marking the branch `[[unlikely]]`. The code generation shifts
  either way.
- This is above D223's +0.10 %, and it is recorded here rather than hidden.

## Not covered here

- Areas on Connectors (§9).
- The same acceleration fault in the curve bound (D212); it is listed in NEXT.
- Click placement on the canvas: Add uses the selected Link and a dialog, as D223 does.
