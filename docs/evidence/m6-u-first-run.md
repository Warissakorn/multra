# M6 kind U — first comparable run, U-c and U-a (D542), 2026-11-13

The engine's capacity for the Stop-controlled minor left turn (rank 2) of a left-hand T, at five
major flows, against Eq. 20-18 at the engine's own `t_c`/`t_f` (U-c) and at HCM 7's 6.2 / 3.3 s
(U-a). Contract: [M6_U_RUN](../reference/M6_U_RUN.md), rows U1–U6 written before code. Band
±15 % (D516). **Nothing was tuned.**

## 1. The run

```bash
./build/desktop/bin/multra-m6-u-run run data build/desktop/compile_commands.json <new-output-dir>   # no mode at build a880783
```

- **Build and seeds:** build `a880783`, seeds 42–81, five levels, 200 runs (about 18 minutes).
- **Copied here:** [summary](m6-u-first-run-summary.json), [metadata](m6-u-first-run-metadata.json)
  and [per-seed rows](m6-u-first-run-runs.csv).
- **The drawing's translation (§1.3):**
  - the merge area is 1.0 m long on the major side, so `τ` = 0.396 s;
  - `gapTime` = **5.804 s**, `headway` 0.1 m.
- **Saturation guard:** passed at every level. The saturated share of major gaps is 0.989 to 0.998,
  so every level is available.

## 2. Capacity (veh/h, mean of 40 seeds, 95 % CI)

| `v_c` nominal | `v_c` counted | Engine | U-a ref. (6.2 / 3.3 s) | Diff. | U-a | U-c ref. | Diff. | U-c |
|---|---|---|---|---|---|---|---|---|
| 200 | 197.5 | **228.5** ± 0.8 | 848.8 | −73.1 % | miss | 290.0 | −21.2 % | miss |
| 400 | 396.7 | **196.0** ± 1.4 | 657.1 | −70.2 % | miss | 305.3 | −35.8 % | miss |
| 600 | 600.4 | **159.4** ± 1.9 | 504.4 | −68.4 % | miss | 307.9 | −48.2 % | miss |
| 800 | 803.8 | **119.2** ± 2.2 | 386.2 | −69.1 % | miss | 298.6 | −60.1 % | miss |
| 1,000 | 1,005.9 | **81.6** ± 2.2 | 295.4 | −72.4 % | miss | 280.3 | −70.9 % | miss |

- **Counted `v_c`:** within 1.3 % of nominal. The far lane carries the same flow, and it is not
  counted (U5's forcing holds at full length).
- **Result:** every level misses both references, by about 70 % against HCM.

## 3. The engine's headways

| | Engine (pooled) | HCM 7 base |
|---|---|---|
| `t_f`, follow-up mean | **13.67 s** | 3.3 s |
| Siegloch slope `t_f` | 14.63 s | |
| Siegloch `t_0` | −2.66 s | |
| Siegloch `t_c = t_0 + t_f/2` | 4.65 s | 6.2 s |

- **Per level:** the follow-up mean falls from 13.9 s at `v_c` 200 to 12.5 s at 1,000.
- **What the figure says:** the minor queue leaves the line at **one vehicle per 13–14 s**,
  even with no major vehicle between two departures.
  - That alone caps capacity near 3600 / 13.7 ≈ 263 veh/h.
  - It is why the engine's capacity barely falls from `v_c` 200 to 400.
- **Siegloch's regression:** its intercept is negative and `t_c` < `t_f`.
  - This is the idealised queue's assumption turned inside out: the gap test is not what limits
    the movement.
  - So U-c's closed form, fed with these values, does not describe the engine either. That is
    why U-c misses as well.

## 4. What this decides

U-c and U-a are **miss** at all five levels; the verdicts stand as measured. The run itself does
not say **why** the Stop service takes about 14 s per vehicle. Candidates, none tested here:
- the move-up from second place to the line;
- the requirement to stand below 0.1 m/s within reach of the line, given that S1 found the
  engine's queues creep near 0.1 m/s rather than stop ([m6-s1-diagnostic](m6-s1-diagnostic.md));
- the one-tick service;
- the merge's receiving-space test.

Splitting `t_f` into those parts is a separate diagnostic with its own contract, before any
change. It joins the held calibration question (D540).
