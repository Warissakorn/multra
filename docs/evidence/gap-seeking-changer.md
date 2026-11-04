# Gap seeking, slice 2: the changer falls back behind its blocker (D458)

2026-11-04. Not validated; development evidence (rule 4). Linux, headless build.
[GAP_SEEKING](../reference/GAP_SEEKING.md) §2 is the contract.

## Why

After D456 no seed locked, but in a dense stream a refused changer still rode alongside the
target-lane car whose rear blocked it:
- the 150 m weave switched on at ×0.8 averaged 43 s of delay;
- the lateral lab's short weave averaged 47–62 s.

Slice 1 asks only followers, so nobody acted on a blocker ahead.

## Variants measured

| Variant | Result |
|---|---|
| Fall back at the cooperative key (3 m/s²) | `laneswap` BA184 clamped: the changer's own followers in a dense mixed queue at 7 m headways could not follow it. Not kept |
| Fall back at `comfortableDeceleration`, both switch states | No lock or clamp anywhere. Switched on: much better. **Switched off: worse when dense.** |
| Only inside the dead-end look-ahead | Almost none of the gain (150 m weave alone, ×1.0, on: delay 67.5 → 65.6 s) |
| Only with the blocker alongside | Close to the kept variant, slightly worse |
| **Kept: comfortable, with the lateral switch on only** (the owner's choice) | As the second row switched on; switched off, unchanged |

The second variant, switched off:
- **Weaving lab** delay rose at ×1.0, 25/19/21 → 29/25/30 s, and at ×1.2, 71/57/51 → 77/73/69 s.
- **Lane-change lab** delay rose 1.7 % (lane drop +0.5 s).
- **Why:** switched off, a change is instantaneous once a gap opens, so falling back only loses time.
  Switched on, the crossing takes time (D204), and lining up with a gap first pays.

## Rows (failure-first)

- **BA204** (`laneswap`): a stub car in a 10 m/s stream, 1.5 m behind the rear of the car ahead on
  the target lane.
  - **Before:** it got in at 218.3 m (slice 2 forced off in the same build).
  - **After:** 199.6 m. It brakes at most 2 m/s², with no clamp.
- **BA205** (`weavinglab`): the 150 m weave alone, ×1.0, switched on, seed 42, 450 s.
  - **Before:** 295 served, mean delay 83.0 s (the D456 build).
  - **After:** 389 served, 36.9 s.
- **BA197 revised:** the 500 m weave alone, ×1.0, seed 42 serves 372, against 387 after D456. Its
  served floor is now 365. It still has no clamp.

## Weave scenes alone, switched on (450 s, seeds 42–51; sums and seed means)

| Factor | Scene | Served, D456 → D458 | Mean delay (s), D456 → D458 |
|---|---|---|---|
| ×0.8 | 150 m | 3132 → 3403 | 29.6 → 14.7 |
| ×0.8 | 300 m | 3341 → 3387 | 14.6 → 11.9 |
| ×0.8 | 500 m | 3251 → 3239 | 13.5 → 13.5 |
| ×1.0 | 150 m | 2973 → 3467 | 67.5 → 42.6 |
| ×1.0 | 300 m | 3335 → 3922 | 44.1 → 28.5 |
| ×1.0 | 500 m | 3868 → 3770 | 24.2 → 28.5 |

**The 500 m weave alone at ×1.0 is the one cost:** a long weave leaves time to find a gap later,
and falling back early costs delay there. In the full lab the same scene improves (below).

## Weaving lab, full, seeds 42–51 (against D456)

**Switched off:** every row is byte-identical to D456.

**Switched on:**

| Factor | Mean delay (s), 150/300/500 m | Served (%) | Waiting at the dead end (mean) |
|---|---|---|---|
| ×0.8 | 43.4/19.7/13.1 → 15.4/12.4/13.7 | 100 → 100 | 1.7/0.6/0 → 0/0/0 |
| ×1.0 | 137.3/97.7/40.5 → 87.3/43.7/34.6 | 96.1/98.0/99.7 → 100/100/100 | 2.8/2.2/0.3 → 0.2/0.1/0 |
| ×1.2 | 184.6/149.8/103.4 → 148.4/84.8/72.4 | 80.2/90.6/96.7 → 91.2/99.5/100 | 4.2/3.0/1.4 → 0.5/0.1/0 |

- No seed locks at any factor.
- Clamps are 0 everywhere; at ×1.2 they fall from 1 to 0.

## Product runs (seeds 42–51, against D456's batches)

| Project | Switched off | Switched on (the audit's keys): delay |
|---|---|---|
| Four-leg | identical | 47.60 → 47.45 s |
| Lane-change lab | identical | 9.06 → 9.17 s |
| M2.6 | identical | 46.57 → 46.42 s |
| T-junction | identical | identical |

Clamps and completed trips are unchanged everywhere.

**Lateral lab:**
- mean delay 30.3 → 13.1 s;
- short weave 62.4/46.6 → 19.3/17.1 s;
- no clamp, nothing unfinished.

## Full sweep table, D458

Not validated; development evidence (D449). Means per seed; clamps, runs with a 2D overlap and removals are totals over the seeds, for all three scenes. 465.7 s for 140 runs.

| scene | factor | lateral | served | unfinished | served % | mean delay (s) | changes | waiting at dead end | longest wait (s) | last arrival (s), earliest seed | locked seeds (of the range) | clamps | 2D overlap | removed |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Weave 150 m | 0.3 | off | 189.7 | 0.0 | 100.0 | 4.5 | 81.7 | 0.0 | 0.0 | 634.5 | 0 | 0 | 0 | 0 |
| Weave 300 m | 0.3 | off | 189.2 | 0.0 | 100.0 | 5.0 | 80.2 | 0.0 | 0.0 | 645.6 | 0 |  |  |  |
| Weave 500 m | 0.3 | off | 201.0 | 0.0 | 100.0 | 6.2 | 83.0 | 0.0 | 0.0 | 647.4 | 0 |  |  |  |
| Weave 150 m | 0.3 | on | 189.7 | 0.0 | 100.0 | 4.7 | 81.7 | 0.0 | 0.0 | 634.3 | 0 | 0 | 0 | 0 |
| Weave 300 m | 0.3 | on | 189.2 | 0.0 | 100.0 | 5.1 | 80.2 | 0.0 | 0.0 | 645.6 | 0 |  |  |  |
| Weave 500 m | 0.3 | on | 201.0 | 0.0 | 100.0 | 6.3 | 83.0 | 0.0 | 0.0 | 647.4 | 0 |  |  |  |
| Weave 150 m | 0.4 | off | 258.8 | 0.0 | 100.0 | 5.1 | 110.9 | 0.0 | 0.0 | 632.3 | 0 | 0 | 0 | 0 |
| Weave 300 m | 0.4 | off | 256.1 | 0.0 | 100.0 | 6.0 | 108.8 | 0.0 | 0.0 | 650.2 | 0 |  |  |  |
| Weave 500 m | 0.4 | off | 257.8 | 0.0 | 100.0 | 7.1 | 111.1 | 0.0 | 0.0 | 654.5 | 0 |  |  |  |
| Weave 150 m | 0.4 | on | 258.8 | 0.0 | 100.0 | 5.4 | 110.9 | 0.0 | 0.0 | 632.3 | 0 | 0 | 0 | 0 |
| Weave 300 m | 0.4 | on | 256.1 | 0.0 | 100.0 | 6.1 | 108.8 | 0.0 | 0.0 | 650.2 | 0 |  |  |  |
| Weave 500 m | 0.4 | on | 257.8 | 0.0 | 100.0 | 7.4 | 111.1 | 0.0 | 0.0 | 654.5 | 0 |  |  |  |
| Weave 150 m | 0.5 | off | 317.3 | 0.0 | 100.0 | 5.9 | 138.7 | 0.0 | 0.0 | 630.9 | 0 | 0 | 0 | 0 |
| Weave 300 m | 0.5 | off | 326.2 | 0.0 | 100.0 | 6.7 | 138.6 | 0.0 | 0.0 | 646.5 | 0 |  |  |  |
| Weave 500 m | 0.5 | off | 321.0 | 0.0 | 100.0 | 7.9 | 136.5 | 0.0 | 0.0 | 661.9 | 0 |  |  |  |
| Weave 150 m | 0.5 | on | 317.3 | 0.0 | 100.0 | 6.3 | 138.7 | 0.0 | 0.0 | 632.6 | 0 | 0 | 0 | 0 |
| Weave 300 m | 0.5 | on | 326.2 | 0.0 | 100.0 | 7.1 | 138.6 | 0.0 | 0.0 | 646.5 | 0 |  |  |  |
| Weave 500 m | 0.5 | on | 321.0 | 0.0 | 100.0 | 8.3 | 136.5 | 0.0 | 0.0 | 662.2 | 0 |  |  |  |
| Weave 150 m | 0.6 | off | 376.8 | 0.0 | 100.0 | 6.6 | 159.6 | 0.0 | 0.0 | 632.4 | 0 | 0 | 0 | 0 |
| Weave 300 m | 0.6 | off | 373.5 | 0.0 | 100.0 | 7.3 | 156.8 | 0.0 | 0.0 | 642.2 | 0 |  |  |  |
| Weave 500 m | 0.6 | off | 399.0 | 0.0 | 100.0 | 9.2 | 168.1 | 0.0 | 0.0 | 665.7 | 0 |  |  |  |
| Weave 150 m | 0.6 | on | 376.8 | 0.0 | 100.0 | 7.4 | 159.6 | 0.0 | 0.0 | 632.4 | 0 | 0 | 0 | 0 |
| Weave 300 m | 0.6 | on | 373.5 | 0.0 | 100.0 | 7.9 | 156.8 | 0.0 | 0.0 | 642.2 | 0 |  |  |  |
| Weave 500 m | 0.6 | on | 399.0 | 0.0 | 100.0 | 9.9 | 168.1 | 0.0 | 0.0 | 666.2 | 0 |  |  |  |
| Weave 150 m | 0.8 | off | 516.3 | 0.0 | 100.0 | 9.7 | 216.2 | 0.0 | 0.0 | 633.9 | 0 | 0 | 0 | 0 |
| Weave 300 m | 0.8 | off | 509.3 | 0.0 | 100.0 | 10.2 | 207.0 | 0.0 | 0.0 | 657.3 | 0 |  |  |  |
| Weave 500 m | 0.8 | off | 502.6 | 0.0 | 100.0 | 11.8 | 218.5 | 0.0 | 0.0 | 668.4 | 0 |  |  |  |
| Weave 150 m | 0.8 | on | 516.3 | 0.0 | 100.0 | 15.4 | 216.2 | 0.0 | 0.0 | 643.8 | 0 | 0 | 0 | 0 |
| Weave 300 m | 0.8 | on | 509.3 | 0.0 | 100.0 | 12.4 | 207.0 | 0.0 | 0.0 | 658.3 | 0 |  |  |  |
| Weave 500 m | 0.8 | on | 502.6 | 0.0 | 100.0 | 13.7 | 218.5 | 0.0 | 0.0 | 669.2 | 0 |  |  |  |
| Weave 150 m | 1.0 | off | 639.0 | 0.0 | 100.0 | 24.5 | 269.4 | 0.1 | 0.2 | 643.3 | 0 | 0 | 0 | 0 |
| Weave 300 m | 1.0 | off | 628.3 | 0.0 | 100.0 | 18.8 | 255.7 | 0.0 | 0.0 | 649.2 | 0 |  |  |  |
| Weave 500 m | 1.0 | off | 635.9 | 0.0 | 100.0 | 21.3 | 272.1 | 0.0 | 0.0 | 670.0 | 0 |  |  |  |
| Weave 150 m | 1.0 | on | 639.0 | 0.0 | 100.0 | 87.3 | 269.4 | 0.2 | 1.3 | 711.3 | 0 | 0 | 0 | 0 |
| Weave 300 m | 1.0 | on | 628.3 | 0.0 | 100.0 | 43.7 | 255.7 | 0.1 | 0.7 | 672.7 | 0 |  |  |  |
| Weave 500 m | 1.0 | on | 635.9 | 0.0 | 100.0 | 34.6 | 272.1 | 0.0 | 0.0 | 691.3 | 0 |  |  |  |
| Weave 150 m | 1.2 | off | 772.3 | 0.0 | 100.0 | 71.4 | 319.3 | 0.0 | 0.0 | 704.0 | 0 | 0 | 0 | 0 |
| Weave 300 m | 1.2 | off | 757.5 | 0.0 | 100.0 | 56.5 | 310.1 | 0.0 | 0.0 | 713.4 | 0 |  |  |  |
| Weave 500 m | 1.2 | off | 758.9 | 0.0 | 100.0 | 51.1 | 308.7 | 0.0 | 0.0 | 733.1 | 0 |  |  |  |
| Weave 150 m | 1.2 | on | 704.7 | 67.6 | 91.2 | 148.4 | 291.8 | 0.5 | 2.4 | 855.9 | 0 | 0 | 0 | 0 |
| Weave 300 m | 1.2 | on | 753.6 | 3.9 | 99.5 | 84.8 | 307.1 | 0.1 | 0.5 | 779.5 | 0 |  |  |  |
| Weave 500 m | 1.2 | on | 758.9 | 0.0 | 100.0 | 72.4 | 308.7 | 0.0 | 0.0 | 791.9 | 0 |  |  |  |
