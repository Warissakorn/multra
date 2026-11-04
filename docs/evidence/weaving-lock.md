# The weave-end lock: a swapping pair at a shared dead end — 2026-11-04

Branch `claude/relaxed-einstein-nfmzno` on `5f25007` (D449) plus this change (D450). Local host:
Linux, GCC 13.3.0, Ninja, `--preset headless` (Debug). **Linux only; no desktop build.** Contract:
[M3_8_CONTRACT](../reference/M3_8_CONTRACT.md) §2 "Cooperation", "At a shared dead end (D450)".
It follows BA183 of the [weaving lab](weaving-lab.md). Nothing here is validated (rule 4).

## Diagnosis

Scratch probes (not committed) printed every vehicle of one weave at given times.

- **The shape:** the two stub routes of a weave share one dead end, the weave's end. A mutual pair
  stands at that line with too little stagger:
  - X stands on lane 0 needing lane 1;
  - Y stands on lane 1 needing lane 0.
  Each target place then lies inside the other's standstill distance, or overlaps it. Every lane
  behind is full, so nobody can yield, and nothing is removed (M3 §4).
- **Mode (a): the rear partner drives back up.** Seed 42, 300 m, off, ×1.0, vehicles 898 and 881:
  32 m apart at 300 s, standing 0.6 m short at 310 s. The unit row BA184 reproduces it, and its
  trace shows the cause:
  - D207 made Y yield, but it dropped the pair each time Y fell more than its standstill distance
    behind X's place (`hold = inf` at 1–4 s and 10–11 s).
  - Once X stood, Y drove back up and stood 1.3 m short of 2 m.
  - The braking cap was not the cause: an experiment that also capped Y's move changed nothing
    until the pair was kept.
- **Mode (b): an intermediate change into a waiting partner's place.** Seed 47, 500 m, off, ×0.6.
  975, a two-change stub on lane 2, changed into lane 1 right beside 994. 994 stood in lane 0
  waiting for exactly that place.

## Change (D450)

- **(a)** `yieldMutual` keeps Y as X's partner at any distance behind X's place, once X brakes for
  its dead end (`deadEndGoverns`, D90's look-ahead) or waits there. The yield is D207's, unchanged.
- **(b)** `landsInWaitingPlace` refuses a mandatory change that would land, still a stub, inside
  the target place, widened by the standstill distance, of a stub on its next target lane. That stub
  must need the landing lane and be waiting or inside its look-ahead.
- No parameter, no state, no schema change. `lanes.cpp` gains one call (468 lines).

## Rows

| Row | Before | After |
|---|---|---|
| BA184 (`laneswap`): two lanes, a swapping pair inside mixed queues | Fails: 16 vehicles stand for good | Every vehicle arrives within 90 s; no clamp, no 2D overlap |
| BA185 (`laneswap`): three lanes; Z (two changes, lower id) beside W waiting | Fails: Z lands in W's place and both stand | W changes first, then Z twice; no clamp, no overlap |
| BA183 (`weavinglab`): 300 m weave alone, off, ×1.0, seed 42 | Fails: nothing arrives in the last minute | Still arriving; no removal, no overlap |

- **Each rule is needed:** without (b), BA185 and BA183 fail; without (a), BA184 fails.
- **D207's rows and BA161 are unchanged.** `ctest --preset headless`: **175/175** pass, and `check`
  passes.

## Product runs, seeds 42–51, `--threads 4`

Every batch of all four projects, off and on, is identical to D448's apart from the build commit:
completed trips, delay and clamps. No shipped project has the pairs D450 acts on.

## Weaving lab, seeds 42–51 (`multra-weaving-sweep`)

Locked seeds per 10, before (D449) → after (D450), for the 150 / 300 / 500 m weaves:

| Factor | Lateral off | Lateral on |
|---|---|---|
| ×0.3, ×0.4 | 0 / 0 / 0 → 0 / 0 / 0 | 0 / 0 / 0 → 0 / 0 / 0 |
| ×0.5 | 1 / 0 / 0 → **0 / 0 / 0** | 2 / 2 / 1 → 2 / 2 / 0 |
| ×0.6 | 0 / 0 / 1 → **0 / 0 / 0** | 6 / 6 / 2 → 7 / 5 / 1 |
| ×0.8 | 5 / 2 / 1 → **1 / 1 / 0** | 10 / 10 / 6 → 10 / 9 / 5 |
| ×1.0 | 10 / 10 / 7 → **7 / 8 / 5** | 10 / 10 / 10 → 10 / 10 / 10 |

The full table:

| scene | factor | lateral | served | unfinished | served % | mean delay (s) | changes | waiting at dead end | longest wait (s) | last arrival (s), earliest seed | locked seeds (of the range) | clamps | 2D overlap | removed |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Weave 150 m | 0.3 | off | 189.7 | 0.0 | 100.0 | 4.5 | 81.7 | 0.1 | 2.4 | 634.5 | 0 | 2 | 0 | 0 |
| Weave 300 m | 0.3 | off | 189.2 | 0.0 | 100.0 | 5.0 | 80.2 | 0.1 | 2.0 | 645.6 | 0 |  |  |  |
| Weave 500 m | 0.3 | off | 201.0 | 0.0 | 100.0 | 6.1 | 83.0 | 0.1 | 1.1 | 647.4 | 0 |  |  |  |
| Weave 150 m | 0.3 | on | 189.7 | 0.0 | 100.0 | 4.9 | 81.7 | 1.1 | 5.2 | 634.0 | 0 | 4 | 0 | 0 |
| Weave 300 m | 0.3 | on | 189.2 | 0.0 | 100.0 | 5.2 | 80.2 | 1.0 | 4.8 | 645.6 | 0 |  |  |  |
| Weave 500 m | 0.3 | on | 201.0 | 0.0 | 100.0 | 6.2 | 83.0 | 1.1 | 5.2 | 647.4 | 0 |  |  |  |
| Weave 150 m | 0.4 | off | 258.8 | 0.0 | 100.0 | 5.2 | 110.9 | 0.4 | 4.9 | 632.3 | 0 | 1 | 0 | 0 |
| Weave 300 m | 0.4 | off | 256.1 | 0.0 | 100.0 | 6.0 | 108.8 | 0.5 | 2.6 | 650.2 | 0 |  |  |  |
| Weave 500 m | 0.4 | off | 257.8 | 0.0 | 100.0 | 7.1 | 111.1 | 0.5 | 3.5 | 654.5 | 0 |  |  |  |
| Weave 150 m | 0.4 | on | 258.8 | 0.0 | 100.0 | 6.3 | 110.9 | 1.8 | 12.2 | 632.3 | 0 | 3 | 0 | 0 |
| Weave 300 m | 0.4 | on | 256.1 | 0.0 | 100.0 | 11.1 | 108.8 | 2.3 | 178.9 | 650.2 | 0 |  |  |  |
| Weave 500 m | 0.4 | on | 257.8 | 0.0 | 100.0 | 7.3 | 111.1 | 1.5 | 8.1 | 654.5 | 0 |  |  |  |
| Weave 150 m | 0.5 | off | 317.3 | 0.0 | 100.0 | 6.0 | 138.7 | 0.4 | 3.4 | 630.9 | 0 | 4 | 0 | 0 |
| Weave 300 m | 0.5 | off | 326.2 | 0.0 | 100.0 | 6.8 | 138.6 | 1.1 | 5.8 | 646.5 | 0 |  |  |  |
| Weave 500 m | 0.5 | off | 321.0 | 0.0 | 100.0 | 7.9 | 136.5 | 1.3 | 5.2 | 661.8 | 0 |  |  |  |
| Weave 150 m | 0.5 | on | 285.0 | 32.3 | 89.8 | 10.4 | 122.5 | 3.4 | 101.9 | 410.9 | 2 | 6 | 0 | 0 |
| Weave 300 m | 0.5 | on | 269.1 | 57.1 | 82.5 | 8.8 | 110.5 | 3.6 | 33.3 | 153.3 | 2 |  |  |  |
| Weave 500 m | 0.5 | on | 321.0 | 0.0 | 100.0 | 8.4 | 136.5 | 2.3 | 8.7 | 662.1 | 0 |  |  |  |
| Weave 150 m | 0.6 | off | 376.8 | 0.0 | 100.0 | 6.9 | 159.6 | 2.1 | 4.6 | 632.4 | 0 | 9 | 0 | 0 |
| Weave 300 m | 0.6 | off | 373.5 | 0.0 | 100.0 | 7.4 | 156.8 | 2.2 | 7.0 | 642.2 | 0 |  |  |  |
| Weave 500 m | 0.6 | off | 386.8 | 12.2 | 96.9 | 9.3 | 161.7 | 3.1 | 8.9 | 551.5 | 0 |  |  |  |
| Weave 150 m | 0.6 | on | 166.6 | 210.2 | 44.2 | 9.5 | 71.4 | 2.0 | 5.7 | 111.6 | 7 | 13 | 0 | 0 |
| Weave 300 m | 0.6 | on | 287.0 | 86.5 | 76.8 | 9.2 | 116.0 | 3.1 | 7.8 | 307.3 | 5 |  |  |  |
| Weave 500 m | 0.6 | on | 347.4 | 51.6 | 87.1 | 10.6 | 145.0 | 3.5 | 14.5 | 152.2 | 1 |  |  |  |
| Weave 150 m | 0.8 | off | 482.4 | 33.9 | 93.4 | 11.4 | 201.8 | 4.7 | 67.0 | 409.4 | 1 | 14 | 0 | 0 |
| Weave 300 m | 0.8 | off | 492.1 | 17.2 | 96.6 | 10.9 | 198.5 | 5.3 | 5.3 | 451.0 | 1 |  |  |  |
| Weave 500 m | 0.8 | off | 492.0 | 10.6 | 97.9 | 26.7 | 214.2 | 6.2 | 313.8 | 668.3 | 0 |  |  |  |
| Weave 150 m | 0.8 | on | 69.5 | 446.8 | 13.5 | 20.9 | 25.8 | 1.8 | 132.8 | 80.1 | 10 | 6 | 0 | 0 |
| Weave 300 m | 0.8 | on | 188.7 | 320.6 | 37.1 | 23.1 | 73.6 | 4.6 | 109.7 | 130.0 | 9 |  |  |  |
| Weave 500 m | 0.8 | on | 314.5 | 188.1 | 62.6 | 16.9 | 134.9 | 7.1 | 99.5 | 183.4 | 5 |  |  |  |
| Weave 150 m | 1.0 | off | 383.2 | 255.8 | 60.0 | 32.9 | 161.6 | 10.0 | 13.5 | 138.9 | 7 | 24 | 0 | 0 |
| Weave 300 m | 1.0 | off | 278.1 | 350.2 | 44.3 | 20.0 | 107.5 | 5.8 | 8.1 | 66.2 | 8 |  |  |  |
| Weave 500 m | 1.0 | off | 435.5 | 200.4 | 68.5 | 25.7 | 189.2 | 11.7 | 45.7 | 269.4 | 5 |  |  |  |
| Weave 150 m | 1.0 | on | 92.9 | 546.1 | 14.5 | 32.9 | 37.3 | 2.3 | 226.3 | 84.9 | 10 | 3 | 0 | 0 |
| Weave 300 m | 1.0 | on | 113.0 | 515.3 | 18.0 | 28.8 | 44.3 | 3.1 | 39.3 | 66.2 | 10 |  |  |  |
| Weave 500 m | 1.0 | on | 114.4 | 521.5 | 18.0 | 26.3 | 45.4 | 2.9 | 9.7 | 144.7 | 10 |  |  |  |

- **Off:** lock-free through ×0.6 now (before: through ×0.4). Locks remain at ×0.8 and above.
- **On:** almost unchanged. Clamps are about as before (off at ×1.0: 17 → 24 over 30 scene-runs);
  no removal and no overlap anywhere.

## What remains (traced; not changed here)

- **Off, seed 43, 150 m, ×1.0:** the same shape. 693 holds for 730 under (a), but stands 1.34 m
  inside the standstill distance. Braking no harder than comfortable (2 m/s²), it cannot match
  730's hard stop at its dead end.
- **On, seed 42, 150 m, ×0.6:** 401 (lane 0, the rear partner) stands 0.3 m from 392's place
  instead of 2 m. It overran by about 1.7 m; not traced further.
- **Two experiments, not kept:**

  | Experiment | ×0.6 on | ×1.0 off | ×1.0 on |
  |---|---|---|---|
  | (1) The hold also targets X's standing place, a standstill distance short of its dead end | 7 / 4 / 1 | 7 / 7 / 4 | unchanged |
  | (2) The D207 yield brakes up to D90's `maxDecelerationCooperativeBraking` (3 m/s²) instead of comfortable | 5 / 2 / 1 | **4 / 2 / 3** (clamps 26) | unchanged (10 / 10 / 10) |

  Experiment (1) is marginal. Experiment (2) helps switched off and redefines D207's braking limit,
  so it is the owner's choice. Neither touches the switched-on locks at ×1.0. Those need their own
  trace: a change under way occupies both lanes, so the pair geometry differs.
