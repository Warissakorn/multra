# M6 kind S1 — the S1 diagnostic: `t_x`, `L_hj` and `v_S` (D530, D536, D537), 2026-11-12

S1-f missed its band in the [first run](m6-s1-first-run.md), at 1,944 against 2,083 veh/h, −6.7 %. §6 calls for the
D530 diagnostic in that case. It splits the engine's saturation headway as [AkcBes02] Eq. 9 does:
`h_S = t_x + 3.6 L_hj / v_S`. Contract: [M6_S1_RUN](../reference/M6_S1_RUN.md) §7, with rows MS8–MS11
written before code. **It decides nothing and has no band.** No model parameter was changed.

## 1. D536's rule measured nothing

D536 called a vehicle standing below 0.1 m/s. Its rerun (build `81193b6`) reproduced every S1 figure
exactly, yet all 1,600 usable cycles were unavailable for the diagnostic.

- **Why:** the engine's queued vehicles never fall below 0.1 m/s. A trace of seed 42, vehicle 217 (rank 5 at Go 900 s) shows it closing on its leader through the whole red. Its speed falls to 0.11 m/s by Go and stays near 0.10 m/s for the first seconds of green.
- **The traced cycle:** only ranks 11–12 ever recorded a stand. They stood at the source, just after they entered.
- **Owner's choice:** read the parts at Go (D537), with the queue definition's 5 km/h as the moving threshold. That threshold is the one DISCHARGE already uses for "queued at Go".

## 2. The run (D537)

```bash
./build/desktop/bin/multra-m6-s1-run data/projects/m6-s1-approach.traffic.json data \
    build/desktop/compile_commands.json <new-output-dir>
```

- **Build:** `5015d4e`, seeds 42–81.
- **Copied here:** [summary](m6-s1-diagnostic-summary.json) and [metadata](m6-s1-diagnostic-metadata.json).
- **S1 figures:** identical to the first run. `runs.jsonl` (`6052cff0…`) and `cycles.csv` (`2c41d3e8…`) have the same SHA-256 as before.
- **Not vendored:** the new `diagnostic.csv` (1.96 MB, `852fd3cd…`).
- **Cycles:** of the 1,600 usable cycles, **1,565** gave the diagnostic.
  - The other **35** are unavailable for one reason: rank 1 crossed the line before reaching 5 km/h, so it has no `moveTime`.
  - They still count for §3.

## 3. The split (mean of 40 seeds, 95 % CI)

| Part | Engine | [AkcBes02] Table 2, Through (isolated) | Difference |
|---|---|---|---|
| `L_hj`, front-to-front spacing at Go | **7.95 m** (± 0.01) | 6.9 m | +15.2 % |
| `t_x`, measured (5 km/h, ranks 5–12) | **1.195 s** (± 0.001) | 1.17 s | +2.2 % |
| `t_x` by Eq. 10, `H − 3.6 L_hj / v_S` | **1.194 s** (± 0.002) | | |
| `v_S`, speed at the line, ranks 5–12 | **43.5 km/h** (± 0.1) | 45.1 km/h | −3.5 % |
| `t_r`, Go to rank 1 reaching 5 km/h | 0.65 s (± 0.01) | | |

The measured and the Eq. 10 `t_x` agree within 0.001 s. Eq. 9 therefore describes the engine's
discharge closely: 1.195 + 3.6 × 7.948 / 43.52 = 1.853 s against its `h_S` of 1.852 s.

**Where the 0.13 s goes.** The engine's `h_S` is 1.852 s. [AkcBes02]'s parts give 1.721 s
(1.17 + 0.551), and their `s` gives 3600 / 2,083 = 1.728 s.

| Term | Engine | Paper | Difference |
|---|---|---|---|
| `3.6 L_hj / v_S` | 0.657 s | 0.551 s | +0.107 s |
| `t_x` | 1.195 s | 1.17 s | +0.025 s |

The first row splits further:
- the longer spacing gives +0.083 s, taken at the paper's `v_S`;
- the lower line speed gives +0.023 s.

So **the jam spacing is the largest part of the S1-f gap**, about two thirds of it:
- the shipped car is 4.5 m long (`data/vehicle-types/car.json`);
- the standing gap at Go is about 3.45 m;
- the behaviour's `standstillDistance` is 2 m (`data/driver-behaviour/default.json`);
- the queue is still creeping at Go (below).

These are the engine's measured parts. **This record claims no cause in the model's code** and proposes no change.

## 4. Speeds by rank (km/h, mean of 40 seeds)

| Rank | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| At the line | 12.1 | 23.5 | 31.4 | 37.1 | 40.9 | 42.9 | 43.6 | 43.9 | 44.2 | 44.3 | 44.3 | 44.3 |
| At Go | 0.02 | 0.05 | 0.10 | 0.17 | 0.24 | 0.33 | 0.43 | 0.54 | 0.66 | 0.79 | 0.93 | 1.08 |

- **At the line:** the speed levels off near 44 km/h from rank 7. [AkcBes02]'s `v_S` is a fitted maximum (`v_n`, Table 1); the engine's 43.5 is the mean over ranks 5–12, still rising at rank 5.
- **At Go:** the queue is not standing. Speed grows with rank to about 1 km/h at rank 12, still below the 5 km/h threshold that counts a vehicle as queued.

## 5. What this decides

Nothing by itself (D530). It gives the owner three facts:

- **The S1-f miss is mostly in the jam spacing:** 7.95 m against 6.9 m.
- **The response time and the line speed are near the paper:** +2.2 % and −3.5 %.
- **The engine's queue creeps** through the red rather than standing.

Whether any of these should change is a separate decision for the owner. It would also move S1-a,
which passes now at 1,944 against 1,900.
