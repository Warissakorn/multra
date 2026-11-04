# Weaving lab: first measurement, lateral off and on — 2026-11-04

Branch `claude/relaxed-einstein-nfmzno` on `66e40e3` plus this change (D449). Local host: Linux,
GCC 13.3.0, Ninja, `--preset headless` (Debug). **Linux only; no desktop build.** Contract:
[WEAVING_FIXTURE](../reference/WEAVING_FIXTURE.md). Nothing here is validated (rule 4): these are
properties of a synthetic fixture and this build, not weaving capacities of a road.

## What the lab is

`data/projects/weaving-lab.traffic.json`:
- three Type A weaves of 150, 300 and 500 m;
- each weave is the entry Link, and the origin is the entry lane;
- mainline 3000 veh/h and ramp 800 veh/h, with 30 % of each crossing;
- inputs 0–600 s, and the run lasts 900 s.

No engine or schema change: `git diff -- src` is empty, so every existing project's run is
unchanged.

## Finding 1: a weave drawn with two entries does not compile

A 2-lane mainline Link and a 1-lane ramp Link joining a 3-lane weave Link (`weavingTwoEntries()`)
refuse the two crossing routes (`UNSUPPORTED_ROUTE_TOPOLOGY`, `routes[1]` and `routes[2]`). A change
is compiled only on a family's entry Link (M3_8_CONTRACT §2). A downstream routing decision on the
weave does not help: no walk from the ramp reaches lanes 1–2, so its stubs are not kept (§2 rule 4).
BA179 pins this. The lab therefore starts the weave at the network edge.

## Finding 2: the weave end locks for good

Scratch probe (not committed): every vehicle of one scene at a given time, for seeds 42, 47 and 49.

- **300 m weave, off, ×1.0, seed 42, t = 450 s and 899 s, unchanged:**
  - lane 0's head (to the mainline, a stub) stands at 298 m;
  - lane 1's head (to the exit, a stub) stands at 292.9 m;
  - lane 2's head (to the exit, two changes) stands at 297.9 m;
  - every vehicle behind them is standing.
- **Why neither can change:** at 4.5 m long, each head's target place overlaps the other head or
  falls inside its standstill distance (about 0.6 m against 2 m). Every lane behind is full, so no
  gap ever opens.
- **The 150 m weave, lateral on, seed 42:** the same lock at 148 m, the heads 6 m apart.
- **The 500 m weave, off, ×0.6, seeds 47 and 49:** the same lock at 495–500 m, the heads 2.3 m and
  4.9 m apart.
- **Why it is permanent:**
  - D207's rear-yields rule acts while a pair approaches; once both stand, it has nothing to give.
  - D90 brakes a follower for a waiting changer, but here every follower is a waiting changer too.
  - MULTRA never removes a vehicle (M3 §4). Vissim's diffusion would delete the pair after 60 s.
- **Accounting holds:** no overlap, no removal, and every vehicle is served or `unfinished`.
- **What it means:** a weave's served flow measures when it locks, not its capacity. That is why
  the factor columns below are not monotone.

## Sweep (`multra-weaving-sweep . --seeds 42-51 --factors 0.3,0.4,0.5,0.6,0.8,1.0`)

"Locked": vehicles remain, and nothing arrived in the last minute of demand (before 540 s).
Clamps, runs with a 2D overlap and removals are totals over the seeds, for the three scenes.

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
| Weave 150 m | 0.5 | off | 297.0 | 20.3 | 93.6 | 5.9 | 128.9 | 0.3 | 3.4 | 301.8 | 1 | 4 | 0 | 0 |
| Weave 300 m | 0.5 | off | 326.2 | 0.0 | 100.0 | 6.8 | 138.6 | 1.1 | 5.8 | 646.5 | 0 |  |  |  |
| Weave 500 m | 0.5 | off | 321.0 | 0.0 | 100.0 | 7.9 | 136.5 | 1.3 | 5.2 | 661.8 | 0 |  |  |  |
| Weave 150 m | 0.5 | on | 285.0 | 32.3 | 89.8 | 10.4 | 122.5 | 3.4 | 102.2 | 410.9 | 2 | 6 | 0 | 0 |
| Weave 300 m | 0.5 | on | 269.1 | 57.1 | 82.5 | 8.5 | 110.5 | 3.6 | 33.3 | 153.3 | 2 |  |  |  |
| Weave 500 m | 0.5 | on | 296.8 | 24.2 | 92.5 | 8.3 | 124.9 | 2.0 | 8.7 | 374.3 | 1 |  |  |  |
| Weave 150 m | 0.6 | off | 376.8 | 0.0 | 100.0 | 6.9 | 159.6 | 2.1 | 4.6 | 632.4 | 0 | 10 | 0 | 0 |
| Weave 300 m | 0.6 | off | 373.5 | 0.0 | 100.0 | 7.4 | 156.8 | 2.3 | 7.0 | 642.2 | 0 |  |  |  |
| Weave 500 m | 0.6 | off | 375.9 | 23.1 | 94.2 | 9.2 | 157.7 | 2.8 | 8.9 | 481.1 | 1 |  |  |  |
| Weave 150 m | 0.6 | on | 190.8 | 186.0 | 50.6 | 9.5 | 79.6 | 2.1 | 5.7 | 111.6 | 6 | 13 | 0 | 0 |
| Weave 300 m | 0.6 | on | 253.4 | 120.1 | 67.8 | 9.0 | 101.0 | 2.9 | 7.6 | 252.4 | 6 |  |  |  |
| Weave 500 m | 0.6 | on | 341.4 | 57.6 | 85.6 | 10.8 | 143.0 | 3.4 | 14.5 | 152.2 | 2 |  |  |  |
| Weave 150 m | 0.8 | off | 367.9 | 148.4 | 71.3 | 10.3 | 150.8 | 2.9 | 5.2 | 104.5 | 5 | 12 | 0 | 0 |
| Weave 300 m | 0.8 | off | 441.6 | 67.7 | 86.7 | 10.9 | 177.5 | 4.9 | 5.3 | 204.6 | 2 |  |  |  |
| Weave 500 m | 0.8 | off | 471.1 | 31.5 | 93.7 | 12.0 | 205.0 | 5.4 | 6.9 | 354.5 | 1 |  |  |  |
| Weave 150 m | 0.8 | on | 48.9 | 467.4 | 9.5 | 14.3 | 16.8 | 0.6 | 28.3 | 80.1 | 10 | 6 | 0 | 0 |
| Weave 300 m | 0.8 | on | 148.7 | 360.6 | 29.2 | 18.8 | 57.1 | 3.5 | 12.1 | 128.5 | 10 |  |  |  |
| Weave 500 m | 0.8 | on | 291.7 | 210.9 | 58.0 | 16.2 | 125.0 | 6.6 | 99.5 | 183.4 | 6 |  |  |  |
| Weave 150 m | 1.0 | off | 254.0 | 385.0 | 39.7 | 21.9 | 104.0 | 5.3 | 5.7 | 148.1 | 10 | 17 | 0 | 0 |
| Weave 300 m | 1.0 | off | 192.4 | 435.9 | 30.6 | 17.0 | 72.9 | 3.2 | 5.8 | 66.2 | 10 |  |  |  |
| Weave 500 m | 1.0 | off | 360.6 | 275.3 | 56.7 | 24.3 | 155.5 | 9.8 | 6.3 | 269.4 | 7 |  |  |  |
| Weave 150 m | 1.0 | on | 82.7 | 556.3 | 12.9 | 28.4 | 32.8 | 2.2 | 240.0 | 84.9 | 10 | 3 | 0 | 0 |
| Weave 300 m | 1.0 | on | 107.1 | 521.2 | 17.0 | 27.5 | 40.8 | 2.7 | 45.0 | 66.2 | 10 |  |  |  |
| Weave 500 m | 1.0 | on | 102.1 | 533.8 | 16.1 | 22.6 | 40.6 | 2.6 | 9.7 | 113.2 | 10 |  |  |  |

An earlier run of the same tool (before the "locked" column) at ×0.7, 0.9 and 1.2 fits between and
beyond these rows. Off at ×0.9 the 150 m weave served 36.8 % and the 300 m one 84.3 %; at ×1.2 every
scene served 5–35 %.

## Reading

- **Lock-free:** through ×0.4 (1520 veh/h, 456 veh/h weaving) in every seed, off and on. Below that,
  every vehicle is served, with no overlap and no removal. Clamps are 1–4 over 10 seeds; the lab's
  insertion at the gore is a likely source, not traced.
- **Locks:**
  - The first locks come at ×0.5: one seed of the 150 m weave off, and 1–2 seeds of every length on.
  - Off, by ×1.0, every seed of the 150 and 300 m weaves locks, and 7 of 10 of the 500 m weave.
- **Switched on locks more and earlier:** 6 of 10 seeds at ×0.6 (150 and 300 m), and every seed at
  ×0.8 (150 and 300 m). This is the expected direction: a changer occupies both lanes while it
  crosses, so gaps are scarcer (DRIVING_BEHAVIOUR_REVIEW §5 said instantaneous changes make weaving
  capacity too high).
- **Length:** off, longer weaves lock later and less often (×0.8: 5, 2, 1 seeds). That direction is
  plausible, but it is not a claim about a road.
- **Long waits without a lock:** at ×0.4 on, a 300 m stub waited 178.9 s before it got through.
  This is the gap-seeking case (NEXT).

## Rows (`weavinglab`, 7 tests)

| Row | Result |
|---|---|
| BA179 | Pass. Each scene: stubs only on the crossing lanes, dead ends at the weave's end, origins by entry lane. The two-entry drawing refuses `routes[1]` and `routes[2]` |
| BA180 | Pass. 300 m weave alone, off, ×0.6, seed 42, 450 s: changes made, no removal, no overlap, no clamp, little left |
| BA181 | Pass. The same, switched on |
| BA182 | Pass. Seed 43 twice, off and on: identical reports and arrival times |
| BA183 | **Open, pinned.** 300 m weave alone, off, ×1.0, seed 42: most demand unfinished, and nothing arrives in the last minute of the 450 s run. A fix must turn it around without removing vehicles |

The rows take 5.6 s together. `ctest --preset headless`: **175/175** pass, and `check` passes. The
shipped-project tests now include `weaving-lab`. SP1 counts 5 files, and N1, MA1, NA8 and the Meso
schema check include it.

## Next (owner's choice, NEXT)

The lock comes first: every weave result depends on it. It is an engine slice, contract first. The
other options are gap seeking, compiling two-entry weaves, and NGSIM at M6.
