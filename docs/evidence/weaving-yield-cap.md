# The weave-end lock: the yielding partner brakes as cooperative braking does (D454) — 2026-11-04

Branch `claude/relaxed-einstein-nfmzno` on `8d97430` (D453). Local host: Linux, GCC 13.3.0, Ninja,
`--preset headless` (Debug). **Linux only; no desktop build.** Nothing here is validated (rule 4).

- **Contract:** [M3_8_CONTRACT](../reference/M3_8_CONTRACT.md) §2, "At a shared dead end", D450 (a).
- **Follows:** [weaving-lock](weaving-lock.md) (D450–D452, patterns P1, P2a, P2b) and
  [weaving-held-change](weaving-held-change.md) (D453).

## Why

The owner asked for a recommendation and chose NEXT option (1) (2026-11-04).

**What was left after D453:**
- **P2b**, switched on (×0.6, seed 49, 300 m): the front stub reaches its dead end at 12 m/s and
  stops at about 6 m/s². D450 (a)'s rear partner yields at comfortable braking only (2 m/s² for a
  car), stands 1.1 m short of the standstill distance, and the pair locks.
- **Off at ×1.0:** 0 / 3 / 2 seeds locked, which is the same overrun.

## Change (D454)

Once the front one closes in (D450 (a): it brakes for its dead end or waits there), the rear partner
brakes up to the larger of its comfortable deceleration and its behaviour's
`maxDecelerationCooperativeBraking`.

- **The value:** the key is named after Vissim's "Maximum deceleration for cooperative braking". It
  is 3 m/s² in `data/driver-behaviour/default.json`, and M3.2.8c already uses it for a target-lane
  vehicle helping a stub.
- **Unchanged:**
  - Before the front one closes in, the cap stays comfortable.
  - A behaviour without the key keeps the comfortable cap.
  - No new parameter, no state.
- **Code:** a few lines in `yieldMutualAs` (`src/core/lane_swap.cpp`).

## Rows

- **BA192** (`laneswap`, two lanes, switched off): X is at 230 m at 10 m/s, 20 m from its dead end;
  its partner Y is 7 m behind at 12 m/s.
  - **Forcing:**
    - X is inside its look-ahead.
    - Stopping by the dead end needs 2.5 m/s², more than the 2 m/s² comfortable deceleration.
    - The behaviour carries the key.
  - **Before:** the pair locked.
  - **After:** both arrive within 60 s, with no clamp.
  - **Setup search:** 144 setups were tried (X from 200–230 m at 10–14 m/s, Y 4–14 m behind at
    10–14 m/s). 22 lock before and resolve after; none that resolved before locks after.
- **BA193** (`weavinglab`): two weaves run alone at ×0.8 for 450 s each.
  - **The runs:** the 150 m weave switched off with seed 47, and the 300 m weave switched on with
    seed 51.
  - **Before:** nothing arrived after 82.9 s and after 152.5 s.
  - **After:** arrivals continue into the last minute in both.

## Product runs, seeds 42–51

- Four-leg, M2.6 and the T-junction are identical to D453 apart from the build commit, off and on.
- The lane-change lab is identical switched off.
- The lane-change lab switched on changes slightly. Mean delay goes from 9.05504 to 9.05499 s, and
  clamps stay 0.

## Weaving lab, seeds 42–51

Up to ×0.5 the results are identical to D453, except one mean: the waiting count for the 150 m weave
at ×0.5 off goes from 0.4 to 0.5. From ×0.6, D453 → D454 (150 / 300 / 500 m):

| Factor | Lateral | Locked seeds | Longest wait (s) | Clamps |
|---|---|---|---|---|
| ×0.6 | off | 0 / 0 / 0 → 0 / 0 / 0 | unchanged | 9 → 9 |
| ×0.6 | on | 0 / 1 / 0 → **0 / 0 / 0** | 29 / 8 / 15 → 29 / 13 / 15 | 15 → 16 |
| ×0.8 | off | 0 / 0 / 0 → 0 / 0 / 0 | 11 / 8 / 6 → 16 / 8 / 6 | 12 → 12 |
| ×0.8 | on | 0 / 0 / 1 → 0 / 0 / 1 | unchanged | 12 → 11 |
| ×1.0 | off | 0 / 3 / 2 → **0 / 2 / 1** | unchanged | 36 → 35 |
| ×1.0 | on | 0 / 1 / 0 → **0 / 0 / 0** | 21 / 27 / 26 → 21 / **205** / 26 | 8 → 7 |

2D overlaps and removals are 0 throughout. The full table:

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
| Weave 150 m | 0.5 | off | 317.3 | 0.0 | 100.0 | 6.0 | 138.7 | 0.5 | 3.4 | 630.9 | 0 | 4 | 0 | 0 |
| Weave 300 m | 0.5 | off | 326.2 | 0.0 | 100.0 | 6.8 | 138.6 | 1.2 | 7.7 | 646.5 | 0 |  |  |  |
| Weave 500 m | 0.5 | off | 321.0 | 0.0 | 100.0 | 7.9 | 136.5 | 1.3 | 5.2 | 661.8 | 0 |  |  |  |
| Weave 150 m | 0.5 | on | 317.3 | 0.0 | 100.0 | 8.4 | 138.7 | 2.8 | 10.7 | 631.1 | 0 | 6 | 0 | 0 |
| Weave 300 m | 0.5 | on | 326.2 | 0.0 | 100.0 | 7.9 | 138.6 | 4.1 | 7.1 | 646.5 | 0 |  |  |  |
| Weave 500 m | 0.5 | on | 321.0 | 0.0 | 100.0 | 8.4 | 136.5 | 2.4 | 8.7 | 662.1 | 0 |  |  |  |
| Weave 150 m | 0.6 | off | 376.8 | 0.0 | 100.0 | 7.0 | 159.6 | 2.2 | 4.6 | 632.4 | 0 | 9 | 0 | 0 |
| Weave 300 m | 0.6 | off | 373.5 | 0.0 | 100.0 | 7.5 | 156.8 | 2.4 | 7.0 | 642.2 | 0 |  |  |  |
| Weave 500 m | 0.6 | off | 399.0 | 0.0 | 100.0 | 9.3 | 168.1 | 3.3 | 8.9 | 665.5 | 0 |  |  |  |
| Weave 150 m | 0.6 | on | 376.8 | 0.0 | 100.0 | 21.4 | 159.6 | 7.6 | 28.8 | 639.2 | 0 | 16 | 0 | 0 |
| Weave 300 m | 0.6 | on | 373.5 | 0.0 | 100.0 | 9.6 | 156.8 | 4.3 | 13.4 | 642.2 | 0 |  |  |  |
| Weave 500 m | 0.6 | on | 399.0 | 0.0 | 100.0 | 10.9 | 168.1 | 4.5 | 15.3 | 665.5 | 0 |  |  |  |
| Weave 150 m | 0.8 | off | 510.1 | 6.2 | 98.8 | 11.5 | 213.1 | 5.0 | 16.1 | 573.0 | 0 | 12 | 0 | 0 |
| Weave 300 m | 0.8 | off | 509.3 | 0.0 | 100.0 | 10.9 | 207.0 | 5.6 | 8.4 | 657.6 | 0 |  |  |  |
| Weave 500 m | 0.8 | off | 502.6 | 0.0 | 100.0 | 12.3 | 218.5 | 5.6 | 6.4 | 668.3 | 0 |  |  |  |
| Weave 150 m | 0.8 | on | 502.2 | 14.1 | 97.3 | 112.2 | 210.9 | 20.4 | 24.1 | 806.7 | 0 | 11 | 0 | 0 |
| Weave 300 m | 0.8 | on | 507.2 | 2.1 | 99.6 | 61.7 | 206.8 | 15.5 | 33.0 | 660.4 | 0 |  |  |  |
| Weave 500 m | 0.8 | on | 471.4 | 31.2 | 93.8 | 28.4 | 204.3 | 12.6 | 11.9 | 484.3 | 1 |  |  |  |
| Weave 150 m | 1.0 | off | 639.0 | 0.0 | 100.0 | 46.7 | 269.4 | 19.1 | 16.6 | 658.3 | 0 | 35 | 0 | 0 |
| Weave 300 m | 1.0 | off | 530.3 | 98.0 | 84.4 | 34.7 | 218.0 | 16.0 | 28.1 | 82.5 | 2 |  |  |  |
| Weave 500 m | 1.0 | off | 594.8 | 41.1 | 93.5 | 25.0 | 256.0 | 15.2 | 16.2 | 335.6 | 1 |  |  |  |
| Weave 150 m | 1.0 | on | 504.8 | 134.2 | 79.0 | 183.0 | 219.6 | 20.0 | 21.4 | 893.3 | 0 | 7 | 0 | 0 |
| Weave 300 m | 1.0 | on | 528.3 | 100.0 | 84.1 | 163.7 | 219.7 | 21.4 | 205.3 | 892.6 | 0 |  |  |  |
| Weave 500 m | 1.0 | on | 565.9 | 70.0 | 89.0 | 128.5 | 248.2 | 21.9 | 25.7 | 894.8 | 0 |  |  |  |

## Reading

- **Switched on:** one lock is left from ×0.5 to ×1.0. It is ×0.8, seed 50, 500 m, where a stub
  stands short of its stop-line reach and is not counted as waiting. Not traced further.
- **Switched off:** at ×1.0, 0 / 2 / 1 seeds still lock. Not traced.
- **The cost:** the 300 m weave on at ×1.0 has a 205 s wait, a long wait and not a lock.
- **Underneath:** there is still no gap seeking. A held crossing can ride alongside same-speed
  traffic for 170 m.
