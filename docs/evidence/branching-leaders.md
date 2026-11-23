# Branching leaders — D623, 2026-11-22

The remedy for A64 ([diverge-acceptance](diverge-acceptance.md)): a body whose rear is inside its
branching area also occupies the sibling Connector (M3_CONTRACT, branching). This note records what it
changed. Linux, GCC 13.3; not a validation and not an owner verdict.

## The rows

`multra-tests diverge` (`tests/diverge_tests.cpp`), seeds 42–51: A62–A70 pass. A64, with B's queue
reaching back over the split: **0 footprint overlaps in every seed** (before: 8 of 10 seeds, up to
1.69 m), 0 clamps, departure order kept (`MULTRA_DIVERGE_REPORT=1` prints the table). A68: a car standing
with its rear past the area holds nobody to C; one standing inside it holds them.

## Shipped projects

`multra-cli --project P --seeds 42-51`, before (`468fa43`) and after, Debug `-Og`. `multra-cli 42`
(M0's `crossing.json`) is byte-identical, and the `core` group's four TS baselines pass unchanged.
Every Micro project moved, by less than the run-to-run noise except three movements:

| Project | Completed (mean of 10 seeds) | Network mean delay, s | Clamps, 10 seeds | Movements whose delay moved beyond the before run's 95 % half-width |
|---|---|---|---|---|
| four-leg-signalised | 504.7 → 504.7 | 47.48 → 47.58 | 0 → 0 | 0 of 12 |
| lane-change-lab | 873.3 → 873.3 | 9.03 → 9.03 | 0 → 0 | 0 of 5 |
| lateral-lab | 1112.6 → 1112.6 | 13.14 → 13.14 | 0 → 0 | 0 of 6 |
| m2.6-study-template | 1959.1 → 1959.1 | 46.37 → 46.51 | 1 → 1 | 0 of 12 |
| m6-s1-approach | 1123.7 → 1123.7 | 566.04 → 566.04 | 1 → 1 | 0 of 1 |
| priority-rule-lab | 272.3 → 272.3 | 9.46 → 9.43 | 0 → 0 | 0 of 4 |
| ratchada-latphrao (study) | 6569.4 → 6468.2 | 655.18 → 664.70 | 20 → 20 | 2 of 12 |
| signalised-node-analysis | 669.2 → 669.2 | 34.59 → 34.59 | 0 → 0 | 0 of 12 |
| t-junction-priority | 272.3 → 272.3 | 6.05 → 6.51 | 0 → 0 | 1 of 4 |
| weaving-lab | 2489.0 → 2489.0 | 20.16 → 20.16 | 0 → 0 | 0 of 10 |
| yield-impatience-lab | 394.8 → 394.8 | 7.07 → 7.33 | 1 → 1 | 0 of 4 |

The three: the study's West → South 897.5 → 966.3 s (±43.9) and East → North 730.8 → 756.4 s (±21.3),
in an oversaturated network kept as a failing case (D578); and the T-junction's Minor approach → Major
eastbound 9.9 → 14.2 s (±2.1). The minor approach is one lane splitting into its two turns: a car
waiting at its line inside the split now holds a car for the other turn behind it, where before that
car drove through its rear. That is single-lane first-in, first-out, as Vissim's red-red branching and
Meso's diverge (ME21) have it; a flared approach where the second turn can pass is not modelled.

## Cost (callgrind, Release `-O3 -g1`, seed 42; instructions)

The first version compiled the branchings through `automaticConflicts`, which measures every pair of
objects: 14.4 % of the 48-crossing benchmark. The compiler now measures only Connector paths leaving the
same lane, with the same `classifiedOverlaps` (A67 checks the two agree); the study and M2.6 outputs are
byte-identical to the first version, and the benchmark differs only in its wall-clock lines.

| Run | Before | After, first version | After |
|---|---|---|---|
| Study (`ratchada-latphrao`) | 33,705 M | 34,660 M (+2.8 %) | 34,548 M (+2.5 %) |
| M2.6 | 3,910 M | 4,101 M (+4.9 %) | 4,032 M (+3.1 %) |
| `multra-engine-benchmark 48 900` | 10,025 M | 11,914 M (+18.8 %) | 10,226 M (+2.0 %) |

Per tick (`stepSimulation` inclusive) the copies cost +2.7 % on the study, +3.2 % on M2.6 and +1.8 % on
the benchmark: every one of these networks has diverges, so every vehicle is checked against its
segment's siblings. A network with no diverge builds no sibling table and takes no extra branch.
