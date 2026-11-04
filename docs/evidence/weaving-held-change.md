# The weave-end lock: a held change that may not land (D453) — 2026-11-04

Branch `claude/relaxed-einstein-nfmzno`. Local host: Linux, GCC 13.3.0, Ninja,
`--preset headless` (Debug). **Linux only; no desktop build.**

- **Contract:** [M3_8_CONTRACT](../reference/M3_8_CONTRACT.md) §2, "At a shared dead end".
- **Follows:** D450–D452 in [weaving-lock](weaving-lock.md) (patterns P1, P2a and P2b are named
  there).
- Nothing here is validated (rule 4).

The owner asked to trace P2a first and then approved the fix (2026-11-04). On `08b7e65` (D452).

## Trace

Lab ×0.6, seed 48, 300 m, switched on, vehicles 657 and 660.
- **The held crossing:** 657, a two-change stub on lane 2, starts into lane 1 at 106 m. Two lane-1
  cars 8 m apart drive alongside its target place at about 12 m/s, so the crossing holds at the lane
  edge (lateral 0.8) for 170 m. Nobody seeks a gap.
- **The landing:** braking for its dead end, 657 falls behind them. The gap opens, and it crosses at
  290 m beside 660, a lane-0 stub at its dead end.
- **Why (b) did not refuse it:** (b) was checked only when a change starts.

A first fix (B: check (b) on every outward step) left 6 locks switched on, the same count as before;
it fixed some and added others. In its new lock at ×0.8, seed 47 (150 m), the held change kept two
more claims:

- **D451 (c):** W (110) yielded to the held change as if it were already on lane 1. W stood at
  144.4 m, 5.6 m short of its dead end and outside its stop-line reach. There it no longer counted as
  waiting, so (b) lapsed and Z (121) crossed beside it.
- **D206:** where W was waiting, the held change's reserved target place kept W from starting into
  lane 1.

## Change (D453, M3_8_CONTRACT §2)

While (b) refuses its landing, a change under way:

- **(B)** takes no outward step (`moveLaterally`);
- **(C)** is no partner for D451 (c) (`yieldMutualUnderWay`);
- **(E)** holds no target place (`decideLaneChanges`).

All three reuse `landsInWaitingPlace`. Switched off, nothing changes.

Tried in scratch (locked seeds switched on at ×0.5–1.0, all scenes):

| Variant | Locks |
|---|---|
| D452 | 6 |
| + B | 6 |
| + C | 17 |
| + B, C | 21 |
| + E | 6 |
| + B, E | 3 |
| + B, C, E | 3 |

- **Why C as well:** B with E alone left the seed-47 lock, which C clears.
- **The cost of C:** a new lock in the 500 m weave at ×0.8, seed 50.

## Rows

- **BA190** (`laneswap`, switched on):
  - **Setup:** Z stands at 246 m on lane 2, its crossing held at lateral 0.8. F on lane 1 lies
    alongside Z's target place. W approaches on lane 0 at 9 m/s from 230 m.
  - **Forcing:** W is inside its look-ahead and not waiting.
  - **Before:** Z crossed once, and W never changed.
  - **After:** W changes, then Z twice, within 40 s. No clamp, no overlap.
  - **Setup search:** 108 setups around this one were tried; all pass after the change, and none
    that passed before fails.
- **BA191** (`weavinglab`): the 150 m weave alone, switched on, ×0.8, seed 42, 450 s.
  - **Before:** nothing arrived after 173.6 s.
  - **After:** arrivals continue into the last minute.

## Product runs, seeds 42–51

- Four-leg, M2.6 and the T-junction are identical to D452 apart from the build commit, off and on.
  The lane-change lab is identical switched off.
- The lane-change lab switched on changes in two movements. Mean delay goes from 9.052 to 9.055 s,
  and clamps are unchanged.

## Weaving lab, seeds 42–51

Switched off, every cell is identical to D452. Switched on, D452 → D453 (150 / 300 / 500 m):

| Factor | Locked seeds | Longest wait (s) | Clamps |
|---|---|---|---|
| ×0.5 | 0 / 0 / 0 → 0 / 0 / 0 | unchanged | 6 → 6 |
| ×0.6 | 1 / 2 / 0 → **0 / 1 / 0** | 29 / 237 / 15 → 29 / **8** / 15 | 15 → 15 |
| ×0.8 | 1 / 0 / 0 → **0 / 0 / 1** | 20 / 103 / 133 → 24 / **33** / **12** | 13 → 12 |
| ×1.0 | 0 / 2 / 0 → **0 / 1 / 0** | 21 / 17 / 26 → 21 / 27 / 26 | 9 → 8 |

2D overlaps and removals: 0 throughout. The full table:

| scene | factor | lateral | served | unfinished | served % | mean delay (s) | changes | waiting at dead end | longest wait (s) | last arrival (s), earliest seed | locked seeds (of the range) | clamps | 2D overlap | removed |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Weave 150 m | 0.3 | off | 189.7 | 0.0 | 100.0 | 4.5 | 81.7 | 0.1 | 2.4 | 634.5 | 0 | 2 | 0 | 0 |
| Weave 300 m | 0.3 | off | 189.2 | 0.0 | 100.0 | 5.0 | 80.2 | 0.1 | 2.0 | 645.6 | 0 |  |  |  |
| Weave 500 m | 0.3 | off | 201.0 | 0.0 | 100.0 | 6.1 | 83.0 | 0.1 | 1.1 | 647.4 | 0 |  |  |  |
| Weave 150 m | 0.3 | on | 189.7 | 0.0 | 100.0 | 4.9 | 81.7 | 1.1 | 5.2 | 634.1 | 0 | 4 | 0 | 0 |
| Weave 300 m | 0.3 | on | 189.2 | 0.0 | 100.0 | 5.2 | 80.2 | 1.0 | 4.8 | 645.6 | 0 |  |  |  |
| Weave 500 m | 0.3 | on | 201.0 | 0.0 | 100.0 | 6.3 | 83.0 | 1.1 | 5.2 | 647.4 | 0 |  |  |  |
| Weave 150 m | 0.4 | off | 258.8 | 0.0 | 100.0 | 5.2 | 110.9 | 0.4 | 4.9 | 632.3 | 0 | 1 | 0 | 0 |
| Weave 300 m | 0.4 | off | 256.1 | 0.0 | 100.0 | 6.0 | 108.8 | 0.5 | 2.6 | 650.2 | 0 |  |  |  |
| Weave 500 m | 0.4 | off | 257.8 | 0.0 | 100.0 | 7.1 | 111.1 | 0.5 | 3.5 | 654.5 | 0 |  |  |  |
| Weave 150 m | 0.4 | on | 258.8 | 0.0 | 100.0 | 6.1 | 110.9 | 1.8 | 6.1 | 632.3 | 0 | 3 | 0 | 0 |
| Weave 300 m | 0.4 | on | 256.1 | 0.0 | 100.0 | 6.4 | 108.8 | 2.1 | 7.8 | 650.2 | 0 |  |  |  |
| Weave 500 m | 0.4 | on | 257.8 | 0.0 | 100.0 | 7.3 | 111.1 | 1.7 | 8.1 | 654.5 | 0 |  |  |  |
| Weave 150 m | 0.5 | off | 317.3 | 0.0 | 100.0 | 6.0 | 138.7 | 0.4 | 3.4 | 630.9 | 0 | 4 | 0 | 0 |
| Weave 300 m | 0.5 | off | 326.2 | 0.0 | 100.0 | 6.8 | 138.6 | 1.2 | 7.7 | 646.5 | 0 |  |  |  |
| Weave 500 m | 0.5 | off | 321.0 | 0.0 | 100.0 | 7.9 | 136.5 | 1.3 | 5.2 | 661.8 | 0 |  |  |  |
| Weave 150 m | 0.5 | on | 317.3 | 0.0 | 100.0 | 8.4 | 138.7 | 2.8 | 10.7 | 631.1 | 0 | 6 | 0 | 0 |
| Weave 300 m | 0.5 | on | 326.2 | 0.0 | 100.0 | 7.9 | 138.6 | 4.1 | 7.1 | 646.5 | 0 |  |  |  |
| Weave 500 m | 0.5 | on | 321.0 | 0.0 | 100.0 | 8.4 | 136.5 | 2.4 | 8.7 | 662.1 | 0 |  |  |  |
| Weave 150 m | 0.6 | off | 376.8 | 0.0 | 100.0 | 7.0 | 159.6 | 2.2 | 4.6 | 632.4 | 0 | 9 | 0 | 0 |
| Weave 300 m | 0.6 | off | 373.5 | 0.0 | 100.0 | 7.5 | 156.8 | 2.4 | 7.0 | 642.2 | 0 |  |  |  |
| Weave 500 m | 0.6 | off | 399.0 | 0.0 | 100.0 | 9.3 | 168.1 | 3.2 | 8.9 | 665.5 | 0 |  |  |  |
| Weave 150 m | 0.6 | on | 376.8 | 0.0 | 100.0 | 21.3 | 159.6 | 7.4 | 28.8 | 639.2 | 0 | 15 | 0 | 0 |
| Weave 300 m | 0.6 | on | 349.2 | 24.3 | 93.5 | 9.3 | 146.1 | 3.6 | 8.4 | 265.4 | 1 |  |  |  |
| Weave 500 m | 0.6 | on | 399.0 | 0.0 | 100.0 | 10.9 | 168.1 | 4.5 | 15.3 | 665.5 | 0 |  |  |  |
| Weave 150 m | 0.8 | off | 505.8 | 10.5 | 98.0 | 11.3 | 211.3 | 4.4 | 11.4 | 573.0 | 0 | 12 | 0 | 0 |
| Weave 300 m | 0.8 | off | 509.3 | 0.0 | 100.0 | 10.9 | 207.0 | 5.6 | 8.4 | 657.6 | 0 |  |  |  |
| Weave 500 m | 0.8 | off | 502.6 | 0.0 | 100.0 | 12.3 | 218.5 | 5.6 | 6.4 | 668.3 | 0 |  |  |  |
| Weave 150 m | 0.8 | on | 502.5 | 13.8 | 97.3 | 112.8 | 210.9 | 20.5 | 24.1 | 806.7 | 0 | 12 | 0 | 0 |
| Weave 300 m | 0.8 | on | 505.6 | 3.7 | 99.3 | 63.3 | 206.5 | 15.7 | 33.0 | 660.4 | 0 |  |  |  |
| Weave 500 m | 0.8 | on | 471.4 | 31.2 | 93.8 | 28.4 | 204.3 | 12.6 | 11.9 | 484.3 | 1 |  |  |  |
| Weave 150 m | 1.0 | off | 639.0 | 0.0 | 100.0 | 46.3 | 269.4 | 18.2 | 16.6 | 658.3 | 0 | 36 | 0 | 0 |
| Weave 300 m | 1.0 | off | 507.1 | 121.2 | 80.7 | 35.8 | 206.6 | 15.1 | 28.1 | 66.2 | 3 |  |  |  |
| Weave 500 m | 1.0 | off | 550.7 | 85.2 | 86.6 | 25.3 | 237.3 | 14.2 | 16.2 | 269.4 | 2 |  |  |  |
| Weave 150 m | 1.0 | on | 504.3 | 134.7 | 78.9 | 183.0 | 219.4 | 20.0 | 21.4 | 893.3 | 0 | 8 | 0 | 0 |
| Weave 300 m | 1.0 | on | 480.6 | 147.7 | 76.5 | 146.5 | 199.8 | 19.3 | 26.9 | 66.2 | 1 |  |  |  |
| Weave 500 m | 1.0 | on | 563.9 | 72.0 | 88.7 | 127.6 | 247.1 | 21.6 | 26.0 | 871.7 | 0 |  |  |  |

## Reading

- **Switched on, three locks are left in 40 scene-runs per factor from ×0.5 to ×1.0:**
  - ×0.6, seed 49 (300 m): P2b, a late hard stop.
  - ×0.8, seed 50 (500 m): new. A lane-0 stub stands 6.1 m short of its dead end, outside its
    stop-line reach, so (b) does not count it as waiting. Not traced further.
  - ×1.0, seed 48 (300 m): not traced.
- **The 237 s wait is gone:** it was the seed-48 lock's.
- **Underneath:** a crossing still holds for 170 m alongside traffic at the same speed. That is the
  gap-seeking case, deferred in NEXT.
