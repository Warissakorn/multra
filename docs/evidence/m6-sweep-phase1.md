# M6 sensitivity sweep, phase 1: candidates B and C (D545), 2026-11-13

The pre-registered sweep ([M6_SWEEP](../reference/M6_SWEEP.md)) measures two data-only levers:
- **B:** `standstillDistance` 2 → 0.95 m;
- **C:** `followingTime` 1.5 → 1.0 s.

D544 ([options sheet](m6-calibration-options.md)) chose no calibration on M6.

**A sweep figure decides nothing and is never adopted.** The "would pass" and "would miss" marks
use the first runs' bands. They are not M6 verdicts.

## How the runs were made

- **Build:** commit `2bf0927df160`, seeds 42–81.
- **Commands:** each tool with `--candidate B` or `--candidate C` as its last argument.
- **Recorded files:** each run's `summary.json` and `metadata.json` (with `complete: true` and the
  candidate named) are copied as `m6-sweep-{s1,s2,u}-{B,C}-{summary,metadata}.json`.
- **Base:** the first runs. A `base` rerun of S1 without the flag reproduced
  [m6-s1-diagnostic-summary.json](m6-s1-diagnostic-summary.json) byte for byte.

## S1 — saturation flow (D535, D536)

| | Base | B | C |
|---|---|---|---|
| Saturation flow (veh/h, mean of 40 seeds) | 1,944.0 | 2,029.8 | 2,109.5 |
| S1-a against 1,900 (±5 %) | +2.3 %, pass | +6.8 %, would miss | +11.0 %, would miss |
| S1-f against 2,083 (±5 %) | −6.7 %, miss | −2.6 %, would pass | +1.3 %, would pass |
| S1-d ranks 2–9 in band | 3 of 8 | 0 of 8 | 1 of 8 |
| Jam spacing `L_hj` (m; paper 6.9) | 7.95 | 6.92 | 7.90 |
| First response (s) | 0.65 | 0.77 | 0.60 |
| Response, Eq. 10 `t_x` (s; paper 1.17) | 1.194 | 1.192 | 1.058 |
| Saturation speed `v_S` (km/h; paper 45.1) | 43.5 | 42.8 | 43.8 |

## S2-b and S2-a — delay against HCM `d1 + d2` (D539)

The reference uses the declared saturation flow, 1,900 veh/h, so it is the same in every column.

| `x` | HCM (s) | Base Δ (s) | B Δ (s) | C Δ (s) |
|---|---|---|---|---|
| 0.50 | 19.98 | 18.51 (−7.4 %) | 18.16 (−9.1 %) | 18.05 (−9.7 %) |
| 0.70 | 25.09 | 21.04 (−16.2 %) | 20.31 (−19.0 %) | 20.04 (−20.1 %) |
| 0.85 | 33.90 | 24.61 (−27.4 %) | 22.74 (−32.9 %) | 22.12 (−34.7 %) |
| 0.95 | 54.34 | 32.09 (−40.9 %) | 27.23 (−49.9 %) | 25.65 (−52.8 %) |
| 1.00 | 86.94 | 42.42 (−51.2 %) | 31.49 (−63.8 %) | 28.92 (−66.7 %) |
| 1.05 | 147.81 | 69.55 (−52.9 %) | 40.27 (−72.8 %) | 34.66 (−76.6 %) |

- **S2-b** (levels 0.5–0.95): every column passes only at `x` = 0.5, so 1 of 4.
- **S2-a** (levels 0.85–1.05): 0 of 4 in every column.
- Both levers raise capacity, so the delay drops and the misses grow, most near and above
  saturation.

## U-a and U-c — Stop-controlled minor left turn (D542)

| `v_c` (veh/h) | HCM U-a | Base | B | C |
|---|---|---|---|---|
| 200 | 846.1 | 228.5 (−73.1 %) | 242.3 (−71.5 %) | 238.9 (−71.9 %) |
| 400 | 654.3 | 196.0 (−70.2 %) | 209.2 (−68.2 %) | 201.1 (−69.4 %) |
| 600 | 504.6 | 159.5 (−68.4 %) | 172.4 (−65.8 %) | 162.5 (−67.8 %) |
| 800 | 388.1 | 119.2 (−69.1 %) | 134.1 (−65.3 %) | 121.1 (−68.6 %) |
| 1,000 | 297.7 | 81.6 (−72.4 %) | 97.3 (−67.1 %) | 81.8 (−72.3 %) |
| Pooled follow-up `t_f` (s; HCM 3.3) | | 13.67 | 12.89 | 12.98 |

- **U-a and U-c** would miss at every level in every column.
- **U-c** differences:
  - base −21 % to −71 %;
  - B −17 % to −59 %;
  - C −18 % to −66 %.
- The guard share is at least 0.99 everywhere, so every level is available.

## Measured effect against the D544 estimate

The D544 estimates were arithmetic, not runs. They were written before these figures and are not
revised.

| Lever | Kind | D544 estimate | Measured |
|---|---|---|---|
| B | S1 | about 2,035 veh/h; S1-a misses, S1-f passes | 2,029.8 veh/h; S1-a would miss, S1-f would pass — **as estimated** |
| B | S2-b/a | capacity +4–5 %; misses grow | misses grow (−9 % to −73 %) — **as estimated** |
| B | U | `t_f` −1 to −2 s; miss | `t_f` −0.78 s; capacity +6 % to +19 %; would miss — **smaller than estimated** |
| C | S1 | above 2,083; both likely miss | 2,109.5 veh/h; S1-a would miss, **S1-f would pass** (+1.3 %) — **estimate wrong on S1-f** |
| C | S2-b/a | large capacity rise; worse | misses grow most (−10 % to −77 %) — **as estimated** |
| C | U | `t_f` about 9–10 s; miss | `t_f` 12.98 s (−0.69 s); capacity within +5 %; would miss — **estimate wrong by 3–4 s** |

**The D544 arithmetic held for S1 and S2, but overstated both levers on U.** U's 2πT reasoning
did not carry: cutting `T` by a third moved the follow-up headway by about 5 %. The move-up to the
Stop line ([split](m6-u-split.md)) is therefore not set by `followingTime` alone. Phase 2 (D and E,
the engine levers) measures it directly.

## What this changes

- **Nothing in the model.** No value is adopted, and the first runs' verdicts stand (D544).
- **Window check.** Neither lever lands in the 1,979–1,995 veh/h window where S1-a and S1-f both
  pass: B overshoots it by about 35 veh/h and C by about 115 veh/h.
- **No data-only lever closes U.** B, the larger of the two, moves U-a by about 5 percentage
  points at most.

Return to [the evidence index](README.md).
