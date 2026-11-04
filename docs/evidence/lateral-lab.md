# The lateral lab (D457)

2026-11-04. Not validated; development evidence (rule 4). Linux, headless build.
[LATERAL_LAB](../reference/LATERAL_LAB.md) is the contract.

## Full project, seeds 42–51

`multra-cli --data-dir data --project data/projects/lateral-lab.traffic.json --seeds 42-51`:

- **Totals:** mean delay 30.3 s; 1112.6 trips completed per seed.
- **Problems:** no safety clamp, and no seed is overloaded.

| Movement | Vehicles per seed | Mean delay (s, ±95 %) | Unfinished |
|---|---|---|---|
| Timed lane change → exit A | 66.8 | 7.2 ± 0.6 | 0 |
| Timed lane change → exit B | 188.9 | 9.2 ± 0.6 | 0 |
| Lane drop | 144.9 | 8.8 ± 0.5 | 0 |
| Short weave → mainline | 312.8 | 62.4 ± 19.9 | 0 |
| Short weave → exit | 190.8 | 46.6 ± 15.9 | 0 |
| Keep-left | 208.4 | 8.2 ± 0.3 | 0 |

**The short weave's delay is high and varies a lot between seeds.** That is the dense-stream case
gap seeking slice 2 targets (NEXT). It serves everything; no seed locks.

## Each scene alone, seed 42, 300 s (the rows' runs)

| Scene | Departed | Arrived | Lane changes | Vehicle-ticks with a crossing under way | Straddled | Clamps | 2D overlap |
|---|---|---|---|---|---|---|---|
| Timed lane change | 157 | 114 | 80 | 2820 | yes | 0 | no |
| Lane drop | 85 | 63 | 47 | 1582 | yes | 0 | no |
| Short weave | 253 | 208 | 107 | 4196 | yes | 0 | no |
| Keep-left | 108 | 82 | 122 | 4359 | yes | 0 | no |
| Keep-left, bias removed | 108 | 82 | 78 | 2649 | yes | 0 | no |

**Keep-left, share of vehicle time in the kerb lane:**

| Vehicle | Bias removed | With the bias |
|---|---|---|
| Car | 0.49 | 0.68 |
| Heavy vehicle | 0.45 | 0.76 |
| Motorcycle | 0.45 | 0.80 |

The kerb lane is lane 0, which the compiled route `…/lane-1` runs on (checked against the Link's
lane order).

## After D458 (gap seeking, slice 2)

The same full-project run (seeds 42–51) after D458:
- mean delay 30.3 → 13.1 s;
- short weave 62.4/46.6 → 19.3/17.1 s (spread ±8.6/±6.0 s);
- the other scenes within 0.3 s;
- no clamp, nothing unfinished.

## Unchanged

- **Other projects:** every other shipped project still compiles switched off (BA199), and its
  pinned results are unchanged.
- **The weaving sweep** reads the same values through `labLateralKeys()`; only where they are
  defined has moved. The weaving rows (BA179–BA197) still pass. The sweep was not re-run.
- **The owner's check:** watching the scenes in the editor (the D203 display) cannot be done in this
  session.
