# B-6 slice 1: desired-speed decisions — 2026-10-17

Branch `claude/m3.4-lane-change-duration-slice1` on `2c72f1c` plus this change (D222, D223). Local
host: Linux, GCC 13.3.0, Ninja, Qt 6.4.2 offscreen, 4 cores. `--preset desktop` (Debug) and
`cmake --build build/desktop --target check`. **Linux only.** Contract:
[SPEED_DECISIONS](../reference/SPEED_DECISIONS.md).

## Engine rows (`tests/speed_decision_tests.cpp`, ctest `speeddecision`)

Hand-built roads; the car's range is 11.11–16.67 m/s, and each car's desired speed is set from a
quantile.

| Row | Case | Asserts | Before the change |
|---|---|---|---|
| BA86 | No decision | Free-flow time is `400 / desired` exactly; no checkpoint field | passes |
| BA87 | Quantiles 0.2 and 0.8 pass `{car: 8–12}` | 8.8 and 11.2; unchanged before the station | fails (12.222 vs 8.8) |
| BA88 | A heavy-vehicle-only decision | The car is unchanged | passes |
| BA89 | `{car: 5–5}` and `{car: 20–20}` | Braking no harder than 2 m/s², no clamp; accelerates past 19 m/s | fails |
| BA90 | A stub car changing lanes before, and one held alongside past the station that changes after | 10 m/s; `freeFlowSoFar = 100 / desired`, counted once | fails |
| BA91 | Two decisions | 8.8, then 20.8: the quantile is kept | fails |
| BA92 | Lone car through `{car: 5–5}` at 50 m of 400 | Free-flow time `50 / desired + 350 / 5` | fails (26.66 vs 73.33) |
| BA93 | Section 100–350 m on that road | Section travel time 50 s; delay < 0.5 s | fails with piecewise off |
| BA94 | Checkpoint after the decision | Carries `desiredQuantile` 0.2; replays exactly | fails |
| BA95 | Codec and validation on the four-leg project | Schema 38 round-trips; 37 → `EDIT_UNSUPPORTED_FIELD`; unknown Link, station off the Link, empty, inverted, zero, non-number and duplicate types each refused at their paths; unknown type on an owned catalog; 39 → `EDIT_VERSION`. A run compiles one entry per lane, and west-movement travel time rises | — |
| BA96 | Commands | `speed-` id; reverse refused only because the decision names a lone Link (forcing: the Link reverses without it); split refuses in the cut and moves the decision downstream; Link delete removes it; Undo restores it | — |

BA93 was written after the code. It was shown to fail by forcing `piecewise_` off: delay ≈ 33 s.
BA95 and BA96 were also written after the code, so each hook was removed in turn:

| Mutation | Caught by |
|---|---|
| Quantile retaken at every decision | BA91 |
| Old free-flow at arrival | BA92 |
| No parse | BA95 |
| No spec validation | BA95 |
| No compile | BA95 |
| No prune on Link delete | BA96 |
| No reverse guard | BA96 (only after the row used a lone Link; link-1 is also held by its Connectors) |
| No split move | BA96 |

**Applied once** cannot be shown by a mutation. Re-applying a decision at the same station adds 0 to
`freeFlowSoFar` and gives the same speed, so it is harmless by construction.

## Editor (`tests/speed_decision_ui_tests.cpp`, ctest `speed-decision-ui`)

The editor has no new canvas tool, which differs from the plan. Instead:
- select one Link and use **Add speed decision**;
- the dialog sets the name, the station (default: the Link's middle) and one min/max row per
  vehicle type, off until ticked;
- the tab lists each decision with its Link and speeds;
- the canvas draws a dotted bar across every lane.

The row failed first (`No speed-decision table`). It checks:
- Add is disabled without one selected Link;
- one Undo step each for Add, Edit and Delete;
- the marks are drawn on every lane;
- an inverted range is refused with no revision;
- the Thai tab title.

`workspace-ui` and `object-groups-ui` pin the object-page list. They failed on the new tab and now
expect 15 pages, with the tab under *Demand*.

`check` **170/170**.

## Shipped runs

**Without a decision,** everything is byte-identical to `2c72f1c`: `trafficsim-cli 42` and the
`--seeds 42–51` JSON (without `buildCommit`) of all four shipped projects.

**With a 30 km/h decision:** a scratchpad copy of `four-leg-signalised` with one decision on the
west approach, `link-1` at 20 m. Cars get 8.33–9.72 m/s and heavy vehicles 7.5–8.33 m/s. Seeds
42–51:

| Movement | Travel time (s) | Delay (s) |
|---|---|---|
| West → East | 89.4 → 115.3 | 46.5 → 50.1 |
| West → North | 85.0 → 112.3 | 42.7 → 48.6 |
| West → South | 82.1 → 108.7 | 39.0 → 43.8 |
| Other approaches | unchanged | unchanged |

- Travel time on the west movements rises by about 26 s. Delay rises by only 4–6 s, because the
  slower driving is not counted (owner's rule, §4). What remains is queueing: slower arrivals
  discharge less per green.
- 6 fewer completed trips on average, and the mean delay goes from 46.28 to 47.79 s.
- **One clamp appeared in seed 47.** Vehicle 58 was cruising at 8.7 m/s, braked at about −7 m/s² from
  6.7 m short of a stop line, and was clamped as it came to rest on it.
  - The pattern matches AMBER §1's red rule: a vehicle that went on amber but is short of the line
    when red begins is held there if it can stop at its type's maximum.
  - A slower vehicle meets that edge further from the line.
  - The decision code does not brake, so it can only change when vehicles arrive. The exact
    trigger is not traced further, and it is recorded in NEXT.

## Performance (callgrind Ir, Release `-g`, `four-leg-signalised` seed 42, no decision)

| Build | Ir |
|---|---|
| D221 (`2c72f1c`), the same build directory | 883.93 M |
| This change | 886.94 M (+0.34 %) |
| This change with the two run-time hooks removed | 886.07 M |

- The hooks themselves cost +0.10 %. A first version tested each vehicle's route list and cost
  +0.34 % alone, so it now tests `scenario.speedDecisions.empty()` once per vehicle.
- The rest of the difference comes with the three new `Vehicle` fields, which the run copies each
  tick.

## Not covered here

- Reduced-speed areas (§7).
- Decisions on Connectors or by lane.
- The seed-47 red-onset clamp with slow approaches.
