# M6 sensitivity sweep, phase 2: engine levers D, E and DE (D546), 2026-11-13

Phase 2 of the pre-registered sweep ([M6_SWEEP](../reference/M6_SWEEP.md) §4) measures the two
engine levers. Each is a runtime-only flag, off by default:

| Lever | Flag | Behind a standing target, or at a Stop |
|---|---|---|
| **D** | `standingTarget` | Behind a leader slower than 0.1 m/s, the desired gap is `standstillDistance` alone. |
| **E** | `stopArrival` | A Stop is made below 0.5 m/s within the reach. |
| **DE** | both | Both of the above. |

**A sweep figure decides nothing and is never adopted.**
- The "would pass" and "would miss" marks use the first runs' bands. They are not M6 verdicts.
- Base is the first runs. B and C are [phase 1](m6-sweep-phase1.md).

## How the runs were made

- **Build:** commit `92620c00c7b1`, seeds 42–81.
- **Recorded files:** each run's `summary.json` and `metadata.json` (with `complete: true` and the
  candidate named) are copied as `m6-sweep-{s1,s2,u}-{D,E,DE}-{summary,metadata}.json`.
- **Run matrix (§4):**
  - D on S1, S2-ab and U;
  - E and DE on U only.
- **Flags off by default:** the full suite (341 tests) passed with the flags off, so shipped
  outputs are unchanged.

## S1 — saturation flow

| | Base | B | C | **D** |
|---|---|---|---|---|
| Saturation flow (veh/h) | 1,944.0 | 2,029.8 | 2,109.5 | **1,910.2** |
| S1-a against 1,900 (±5 %) | +2.3 %, pass | would miss | would miss | **+0.5 %, would pass** |
| S1-f against 2,083 (±5 %) | −6.7 %, miss | would pass | would pass | **−8.3 %, would miss** |
| S1-d ranks 2–9 in band | 3 of 8 | 0 of 8 | 1 of 8 | **5 of 8** |
| Jam spacing `L_hj` (m; paper 6.9) | 7.95 | 6.92 | 7.90 | **6.50** |
| First response (s) | 0.65 | 0.77 | 0.60 | **0.68** |
| Response, Eq. 10 `t_x` (s; paper 1.17) | 1.194 | 1.192 | 1.058 | **1.345** |
| `v_S` (km/h; paper 45.1) | 43.5 | 42.8 | 43.8 | **43.3** |

**D's effect on S1:**
- **Jam spacing:** it falls to 6.50 m, as estimated.
- **Flow:** it falls, not rises.
- **Why the flow falls** (a reading of the diagnostic, not a separate measurement):
  - **Start-up:** a queue that stands at the bare standstill gap starts later, so `t_x` rises
    from 1.19 to 1.35 s.
  - **Spacing:** the shorter spacing does not make up for the later start.

## S2-b and S2-a — delay against HCM `d1 + d2`

| `x` | HCM (s) | Base Δ (s) | B Δ (s) | C Δ (s) | **D Δ (s)** |
|---|---|---|---|---|---|
| 0.50 | 19.98 | 18.51 (−7.4 %) | 18.16 (−9.1 %) | 18.05 (−9.7 %) | **18.57 (−7.1 %)** |
| 0.70 | 25.09 | 21.04 (−16.2 %) | 20.31 (−19.0 %) | 20.04 (−20.1 %) | **21.21 (−15.5 %)** |
| 0.85 | 33.90 | 24.61 (−27.4 %) | 22.74 (−32.9 %) | 22.12 (−34.7 %) | **25.15 (−25.8 %)** |
| 0.95 | 54.34 | 32.09 (−40.9 %) | 27.23 (−49.9 %) | 25.65 (−52.8 %) | **34.22 (−37.0 %)** |
| 1.00 | 86.94 | 42.42 (−51.2 %) | 31.49 (−63.8 %) | 28.92 (−66.7 %) | **48.97 (−43.7 %)** |
| 1.05 | 147.81 | 69.55 (−52.9 %) | 40.27 (−72.8 %) | 34.66 (−76.6 %) | **83.93 (−43.2 %)** |

- **Verdicts:** with D, S2-b still passes 1 of 4 and S2-a 0 of 4.
- **Direction:** D lowers capacity, as in S1, so its delay moves towards HCM. Its misses shrink by
  up to 10 percentage points. They do not grow.

## U-a and U-c — Stop-controlled minor left turn

| `v_c` (veh/h) | HCM U-a | Base | B | C | **D** | **E** | **DE** |
|---|---|---|---|---|---|---|---|
| 200 | 846.1 | 228.5 | 242.3 | 238.9 | **416.6** | 228.5 | **481.5** |
| 400 | 654.3 | 196.0 | 209.2 | 201.1 | **324.6** | 196.0 | **365.4** |
| 600 | 504.6 | 159.5 | 172.4 | 162.5 | **237.9** | 159.4 | **263.6** |
| 800 | 388.1 | 119.2 | 134.1 | 121.1 | **164.7** | 119.2 | **179.3** |
| 1,000 | 297.7 | 81.6 | 97.3 | 81.8 | **106.0** | 81.6 | **113.5** |
| Pooled `t_f` (s; HCM 3.3) | | 13.67 | 12.89 | 12.98 | **6.91** | 13.67 | **5.82** |

- **U-a** would miss at every level in every column. The differences are:
  - D: −51 % to −64 %;
  - DE: −43 % to −62 %;
  - base: −68 % to −73 %.
- **U-c, against the Siegloch form of each run's own fit:**
  - D would pass at `v_c` 200 and 400 (−3.2 %, −7.5 %);
  - DE would pass at 200, 400 and 600 (−1.8 %, −5.0 %, −11.4 %);
  - both would miss at 800 and 1,000;
  - base, B, C and E would miss at every level.
- **E alone equals base.**
  - Capacity is the same at every level.
  - `t_f` differs by under 0.0001 s.
  - **Why** (SW9's amendment): without D, a car enters the Stop reach already below 0.1 m/s.
- **Every level is available** (guard share ≥ 0.99).

## Measured effect against the D544 estimate (not revised)

| Lever | Kind | D544 estimate | Measured |
|---|---|---|---|
| D | S1 | jam spacing about 6.5 m; about 2,070 veh/h | 6.50 m **as estimated**; but 1,910.2 veh/h, **down, not up** (`t_x` +0.15 s) |
| D | S2-b/a | misses grow | misses **shrink** (capacity falls); verdicts unchanged |
| D | U | `t_f` about 10 s | `t_f` 6.91 s: **larger than estimated** |
| E | U | `t_f` about 9 s | 13.67 s: **no effect alone** |
| DE | U | none | `t_f` 5.82 s |

**Where the arithmetic failed.**
- **D on S1 and S2:** D544 counted only the spacing, and missed the slower start that a tight
  standing queue gives.
- **E:** D544 assumed the slow settle happens inside the Stop reach. It does not.
- **D on U is the largest single effect measured.** The creep accounts for about half of U's
  follow-up headway (13.67 → 6.91 s). DE is still 2.5 s, and D 3.6 s, above HCM's 3.3 s.

## What this changes

- **Nothing in the model.** The flags stay off, nothing is adopted, and the first runs' verdicts
  stand (D544).
- **No lever passes everything.**
  - D brings S1-a and U-c towards their references, but moves S1-f away from its own.
  - U-a misses under every lever.
- **For later calibration** (only with independent data: R7's inD trajectories and Thai data, Q4
  (d)):
  - the standing-target creep (D) and the start-up response are the parts that move the most;
  - E is useful only together with D.

Return to [the evidence index](README.md).
