# M6 S2 — the engine's effective green, and the references at its own values (D541), 2026-11-13

S2-b and S2-a miss below HCM `d1 + d2` ([m6-s2ab-first-run](m6-s2ab-first-run.md)), and the arrivals are
bunched rather than smoothed ([m6-s2-arrivals](m6-s2-arrivals.md)). This record measures the engine's
capacity, effective green and lost time, and recomputes the references at those values. Contract:
[M6_S2_RUN](../reference/M6_S2_RUN.md) §8 (`03b9915`), rows S2E1–S2E4 tested before the run. **No band;
it decides nothing; no model, setup or verdict was changed.**

## Run

```bash
./build/desktop/bin/multra-m6-s2-run green data build/desktop/compile_commands.json <new-output-dir>
```

- **Runs:** build `db17252`, the saturated S1 project (90/40/3 s), seeds 42–81, 40 cycles each. The run took 2 min 15 s.
- **Count:** every crossing at the line, amber and red included.
- **Committed here:** [summary](m6-s2-green-summary.json) and [metadata](m6-s2-green-metadata.json). The per-cycle `green.csv` (43 KB, `d0d481cd…`) is identified by hash.

## A. Capacity, effective green and lost time (mean of 40 seeds, 95 % CI)

| Quantity | Engine | HCM 7 base (§6) |
|---|---|---|
| Vehicles per cycle `N` | **22.82** (± 0.04) | 21.1 (= 844.4 · 90 / 3600) |
| Of which in amber | 1.57 (± 0.03) | — |
| Saturation headway `h_S`, ranks 5–12 | 1.852 s (± 0.002) | 1.895 s (1,900 veh/h) |
| Effective green `g_e = N · h_S` | **42.27 s** (± 0.10) | 40 s |
| Lost time `43 − g_e` | **0.73 s** (± 0.10) | 3 s (`l1` 2 s + `l2` 1 s) |
| Capacity `N · 3600 / 90` | **913.0 veh/h** (± 1.6) | 844.4 veh/h |

The engine's approach carries **8.1 % more** than the capacity HCM's reference assumes.

- **About 2 points come from the faster saturation headway**, the S1-a finding.
- **About 6 points come from lost time.** The engine loses only 0.73 s of the 43 s green plus amber, against HCM's 3 s:
  - its first vehicle crosses 1.38 s after Go (S1-d);
  - about 1.6 vehicles cross in each amber.

So the nominal `x` = 1.0 of S2-a is only 0.925 for the engine.

## B. The references at the engine's own values (s/veh, shown only)

| `x` (nominal) | `X` engine | Engine Δ (D539) | HCM declared | HCM at `g_e`, `c_e` | FCTL declared (21/26 slots) | FCTL engine (23/26 slots of 1.852 s) |
|---|---|---|---|---|---|---|
| 0.5 | 0.462 | 18.51 | 19.98 | 17.86 (Δ +3.6 %) | 18.40 | 17.12 (Δ +8.1 %) |
| 0.7 | 0.647 | 21.04 | 25.09 | 21.79 (−3.4 %) | 21.44 | 19.56 (+7.6 %) |
| 0.85 | 0.786 | 24.61 | 33.90 | 27.19 (−9.5 %) | 27.97 | 23.21 (+6.0 %) |
| 0.95 | 0.879 | 32.09 | 54.34 | 35.00 (−8.3 %) | 52.46 | 30.12 (+6.5 %) |
| 1.0 | 0.925 | 42.42 | 86.94 | 43.40 (−2.3 %) | 414.47 | 40.12 (+5.7 %) |
| 1.05 | 0.971 | 69.55 | 147.81 | 61.52 (+13.1 %) | unstable | 83.19 (−16.4 %) |

- **At the engine's own green and capacity, HCM `d1 + d2` lies within ±15 % of the engine's delay at all six levels**, from −9.5 % to +13.1 %.
- **The exact Poisson queue at the engine's own values** lies 6–8 % below the engine at the five stationary levels below 1.05. That is roughly the deceleration and acceleration that Δ holds and FCTL does not, the same sign as S2-c. Its 1.05 level is a stationary queue at `X` 0.97, which a 1 h window does not reach.
- **The declared FCTL column** shows how far HCM's `d2` sits from an exact Poisson queue at the declared capacity: close at 0.5 to 0.95, while a 47-slot cycle with 21 green slots is near instability at 1.0. So `d2` itself is not what makes the engine low.

## What it shows

- **The S2-b and S2-a miss comes from the engine's capacity, not from its queueing.** The engine discharges an 8 % larger stream per cycle than the reference's 844 veh/h, through a faster saturation headway and, mainly, a lost time of 0.73 s against HCM's 3 s. At its own capacity the engine's delay follows both HCM and the exact queue.
- **The verdicts of D539 stand as recorded.** The pre-registered reference is HCM at `s` 1,900 and `g` 40 s, and nothing here changes it.
- **For the owner:** the S1 and S2 findings now meet in one question. Should the engine's discharge be calibrated?
  - jam spacing at Go: 7.95 m against 6.9 m (S1-f);
  - start-up response: rank 1 at 1.38 s;
  - amber use: about 1.6 vehicles per amber;
  - together these set lost time and capacity.

  That remains the single calibration decision held by D540, after U's first run.
