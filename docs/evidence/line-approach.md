# M3.2.8a.1c: stopping cleanly at a line and behind a hard stop, prototypes measured — 2026-10-19

Branch `claude/red-running-owner-choices`, on D255's commit. Linux, GCC 13.3.0, Release `-g` in a
scratch directory for the runs; the row suites ran in the Debug `desktop` build. Contract and
options: [LINE_APPROACH](../reference/LINE_APPROACH.md) (D256). **No engine code is committed.**
Every variant was a temporary patch of `src/core/following.cpp`, and for J of
`src/core/simulation.cpp`, built and reverted (D232's, D241's, D249's and D253's method). Each
patch also carried D249's span-pair overlap counter and a clamp print.

## Method

- **Runs:** seeds 42–81 of the four shipped projects at D251's defaults (`joinAnticipation` off),
  plus a copy of M2.6 with a 5 % red-running share and a 2 s window (D241's case).
- **Baseline:** the probe-only build's outputs are byte-identical to the committed build's.
- **Classes:** M2.6's 17 clamps are keyed by seed, vehicle and time, and classed as in D249/D251
  (pull-out J1 9, late catch-up J2 4, merge creep M 3, other 1). A clamp at a key not in that set
  counts as new.
- **Rows:** each variant's `trafficsim-tests`, all groups (954 rows), and `trafficsim-tests reference`
  (the 5 TypeScript reference rows).
- **Binding:** a further probe printed each law evaluation in which a floor binds, with the floor
  and the law's own value. That includes hypothetical evaluations such as insertion checks, so the
  counts are an upper bound.

## Variants

| Variant | Rule |
|---|---|
| F1 | `braking` mode, leader standing (`speed == 0`), vehicle moving: acceleration ≤ `−v²/(2·gap)` (D227's floor) |
| F1b | F1 in any mode but `free`, with a leader slower than 0.1 m/s |
| F2 | Any mode but `free`, leader moving and braking (`a < −0.1`): acceleration ≤ `−v²/(2·(gap − s0 + vL²/(2·−aL)))` |
| F2c | F2, only when that asks for more than the type's `comfortableDeceleration` |
| F1a, F2ca | F1 and F2c, only for a behaviour carrying `amberDeceleration` |
| J | A vehicle its rule held on the previous tick is not `committed` to that rule (D249's J) |

## Results

"Seeds changed" is against today, out of 40.

| Variant | Four-leg cl / delay / changed | Lane lab cl / changed | M2.6 cl / delay / changed | T-junction delay / changed | M2.6 + red running cl | Pin | TS rows | Rows |
|---|---|---|---|---|---|---|---|---|
| Today | 0 / 46.001 / — | 1 / — | 17 / 46.010 / — | 7.069 / — | 17 | — | 5/5 | 954 |
| F1 | 0 / 46.001 / 0 | 1 / 0 | 5 / 46.026 / 23 | 7.099 / 23 | 5 | same | 4/5 | 951 |
| F1b | 0 / 46.001 / 25 | 1 / 2 | 5 / 46.028 / 38 | 7.100 / 28 | 5 | same | — | — |
| F2 | 0 / 45.981 / 39 | 0 / 40 | 18 / 46.054 / 40 | 7.093 / 40 | 18 | **differs** | — | — |
| F2c | 0 / 45.986 / 23 | 0 / 31 | 17 / 46.031 / 38 | 7.069 / 9 | 17 | same | 1/5 | 950 |
| F1b + F2 | 0 / 45.993 / 40 | 0 / 40 | 5 / 46.071 / 40 | 7.125 / 40 | 5 | differs | 1/5 | 947 |
| F1b + F2 + J | identical to F1b + F2 | | | | | | | |
| F1 + F2c | 0 / 45.986 / 23 | 0 / 31 | 5 / 46.048 / 39 | 7.097 / 27 | 5 | same | 1/5 | 948 |
| **F1a** | identical to F1 | identical | identical to F1 | identical to F1 | 5 | same | **5/5** | **953** |
| F1a + F2ca | identical to F1 + F2c | | | | 5 | same | **5/5** | 953 |
| J | 0 / 46.001 / 0 | 1 / 0 | **27** / 46.010 / 15 | 7.069 / 0 | 27 | same | — | — |

- **No overlap** in any variant, and completed trips change by at most 0.1 per run.
- **M2.6's classes under F1 (and F1a):** J1 9 → 1, M 3 → 0, other 1 → 0, J2 4 → 4, none new. F2
  keeps 16 of the 17 and adds 2. J keeps 7 and adds 20.
- **The rows that fail:**
  - F1a and F1a + F2ca fail only BA164's forcing (`mergeclamps.ba164…`, line 113). The red-head
    creep that row forces with `redLineStop` off no longer clamps, because the floor stops it.
  - Unscoped F1 also fails BA165 (the same creep without `amberDeceleration`) and
    `reference.typescript_seed_uint32_max` (events[131].time).
  - The F2 forms fail reference seeds 0, 42, 43 and 2³²−1.
- **The T-junction under F1a, by movement (s):**

  | Movement | Today | F1a |
  |---|---|---|
  | Major eastbound | 4.23 | 4.22 |
  | Major westbound | 4.13 | 4.12 |
  | Minor → Major westbound | 25.13 | 25.49 (±2.96) |
  | Minor → Major eastbound | 15.03 | 15.09 (±1.83) |

- **With `joinAnticipation` on** (scratch copies at schema 46): today M2.6 16, T-junction 7.545 s;
  F1 gives M2.6 4 and T-junction 7.606 s; F1 + F2c gives M2.6 4 and T-junction 7.573 s. The four-leg
  is unchanged by the switch.

### How hard the floors brake (F1a + F2ca, binding evaluations)

| Project | F1a per run | F1a median (law median), m/s² | F2ca per run | F2ca median |
|---|---|---|---|---|
| Four-leg | 0 | — | 478 | −2.05 |
| Lane lab | 1 | past the maximum (−25.9 before the cap), so 8 | 56 | −2.16 |
| M2.6 | 5.5 | −5.90 (−3.94) | 1,933 | −2.05 |
| T-junction | 5.2 | −6.21 (−4.06) | 41 | −2.05 |

F1a's binding values on M2.6 and the T-junction stay within the car's maximum (the lowest is
−7.93). Values past the maximum, in the lane lab and in F2ca's tail, are the floor before the law's
final clamp to `maxDeceleration`.

**Cost.** Ir was not measured: every prototype carried the overlap counter, which dominates its
cost. F1a adds one branch in the prototype law.

## Not covered here

- D227's 30 km/h-car case on the four-leg (its creep clamp at seed 56) was not re-run.
- The W74 law.
- F1a's effect on comfort or emissions measures.
