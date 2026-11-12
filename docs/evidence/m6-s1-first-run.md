# M6 kind S1 — the first comparable run (D535), 2026-11-12

The first comparison of the engine with S1's references ([m6-benchmark-options](m6-benchmark-options.md)
§6). Contract: [M6_S1_RUN](../reference/M6_S1_RUN.md), written and committed (`fcb640b`) before the
run. The reductions were tested on synthetic cycles (rows MS1–MS7) before any engine figure was seen.
**Nothing was tuned after the figures were seen.** The verdicts below are reported as they fell.

## Run

```bash
./build/desktop/bin/multra-m6-s1-run data/projects/m6-s1-approach.traffic.json data \
    build/desktop/compile_commands.json <new-output-dir>
```

- **Inputs:**
  - build `2a9e2cc` (Linux, GCC);
  - project `data/projects/m6-s1-approach.traffic.json` (one lane, 300 m, cycle 90 s, green 40 s, amber 3 s, 1,200 cars/h);
  - `default.json` behaviour, `perInput` streams, dt 0.1 s;
  - seeds 42–81, warm-up 900 s, window 3,600 s.
- **Copied here:** [m6-s1-first-run-summary.json](m6-s1-first-run-summary.json) and
  [m6-s1-first-run-metadata.json](m6-s1-first-run-metadata.json). The metadata holds the tool sources' and inputs' SHA-256 and the compiler command.
- **Not vendored:** the raw `runs.jsonl` (3.8 MB) and `cycles.csv` (342 KB). They are identified by the SHA-256 in the metadata:
  - `runs.jsonl`: `6052cff0…`;
  - `cycles.csv`: `2c41d3e8…`;
  - `summary.json`: `87c8c776…`.

  Rerunning the command reproduces them.
- **Per run** (means over 40 seeds): 1,504.1 vehicles generated, 1,122.8 completed, 348.1 pending at the source.
  - Demand is above the approach's capacity, as §6 intends: every cycle starts with a standing queue.
- **Cycles over all seeds:**
  - **1,600 usable** (40 per seed, every cycle in the window);
  - 360 outside the window (the warm-up);
  - 40 initial partial cycles;
  - 40 partial cycles at the end.
  - No seed lacks usable cycles. Nothing was dropped.

## S1-a and S1-f: saturation flow, `3600 /` the mean headway of ranks 5–12

| Figure | Engine, mean of 40 seeds (95 % CI) | Reference | Difference | Verdict (±5 %) |
|---|---|---|---|---|
| S1-a | **1,944.0** veh/h (± 2.6) | 1,900 (HCM 7 Exhibit 19-11) | +2.3 % | **pass** |
| S1-a, beside | | 1,875 ([Zegeer86]) | +3.7 % | shown |
| S1-f | **1,944.0** veh/h (± 2.6) | 2,083 ([AkcBes02] Table 2) | −6.7 % | **miss** |

- The engine's steady headway is 1.852 s.
- Seeds range from 1,927.4 to 1,966.2 veh/h (sd 8.1):

| Seed | 42 | 43 | 44 | 45 | 46 | 47 | 48 | 49 | 50 | 51 |
|---|---|---|---|---|---|---|---|---|---|---|
| `s` | 1952.9 | 1966.2 | 1943.0 | 1940.7 | 1948.6 | 1945.9 | 1954.9 | 1938.4 | 1934.2 | 1951.2 |
| Seed | 52 | 53 | 54 | 55 | 56 | 57 | 58 | 59 | 60 | 61 |
| `s` | 1943.3 | 1945.9 | 1952.9 | 1931.9 | 1939.1 | 1940.4 | 1952.5 | 1931.6 | 1947.3 | 1941.0 |
| Seed | 62 | 63 | 64 | 65 | 66 | 67 | 68 | 69 | 70 | 71 |
| `s` | 1945.3 | 1956.9 | 1946.9 | 1930.0 | 1941.4 | 1937.1 | 1943.6 | 1946.9 | 1942.3 | 1947.6 |
| Seed | 72 | 73 | 74 | 75 | 76 | 77 | 78 | 79 | 80 | 81 |
| `s` | 1950.2 | 1947.3 | 1933.9 | 1941.4 | 1927.4 | 1939.7 | 1951.9 | 1944.3 | 1951.6 | 1934.5 |

The two references are 9.6 % apart (1,900 against 2,083). No single figure can lie within ±5 % of both unless it falls between 1,979 and 1,995 veh/h. The engine sits nearer HCM 7's base value and Zegeer's measurement than Akçelik & Besley's Australian average.

## S1-d: headway by queue position (mean over cycles, then over 40 seeds)

| Rank | Engine (s) | 95 % CI | [LeeChen86] 40 mph | Difference | Verdict | [Bonneson92] Eq. 20 | Difference |
|---|---|---|---|---|---|---|---|
| 1 | 1.379 | ± 0.024 | 3.70 | −62.7 % | shown | 3.22 | −57.2 % |
| 2 | 2.669 | ± 0.010 | 2.46 | +8.5 % | **miss** | 2.10 | +27.1 % |
| 3 | 2.209 | ± 0.004 | 2.28 | −3.1 % | **pass** | 2.03 | +8.8 % |
| 4 | 2.021 | ± 0.004 | 2.17 | −6.9 % | **miss** | 1.97 | +2.6 % |
| 5 | 1.923 | ± 0.005 | 2.07 | −7.1 % | **miss** | 1.93 | −0.3 % |
| 6 | 1.879 | ± 0.006 | 2.00 | −6.0 % | **miss** | 1.89 | −0.6 % |
| 7 | 1.852 | ± 0.007 | 1.93 | −4.0 % | **pass** | 1.85 | +0.1 % |
| 8 | 1.844 | ± 0.008 | 1.91 | −3.5 % | **pass** | 1.83 | +0.7 % |
| 9 | 1.832 | ± 0.010 | 1.93 | −5.1 % | **miss** | 1.81 | +1.2 % |
| 10 | 1.833 | ± 0.009 | 1.78 | +3.0 % | shown | 1.79 | +2.4 % |
| 11 | 1.830 | ± 0.012 | 1.64 | +11.6 % | shown | 1.78 | +2.8 % |
| 12 | 1.821 | ± 0.012 | 1.76 | +3.5 % | shown | 1.76 | +3.5 % |

Banded ranks 2–9: **3 pass, 5 miss.**

- **Ranks 4–6 and 9** sit below Lee & Chen by 5–7 %. The contract expected this direction:
  - every S1 cycle holds more than 12 vehicles;
  - Lee & Chen average platoons of every length, and longer platoons discharge faster (their Table 13).
- **Rank 2** sits 8.5 % above.
- **From rank 4 on**, the engine follows Bonneson's curve within 3.5 %. That curve has no band (D532).
- **Rank 1** (1.38 s from Go to the first front crossing) is far below both references, and is shown only:
  - Lee & Chen time the rear bumper, and Bonneson times an axle just past the line;
  - the engine times the front of a vehicle stopped close to the line, so rank 1 holds little travel.

  Part of the start-up delay appears to show in rank 2 instead. That is not measured here.

## S1-f: crossing time after Go, against Eq. 7 (shown, no band)

| Rank `k` | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Engine (s) | 1.38 | 4.05 | 6.26 | 8.28 | 10.20 | 12.08 | 13.93 | 15.78 | 17.61 | 19.44 | 21.27 | 23.09 |
| Eq. 7 `t(k)` (s) | 3.76 | 5.85 | 7.73 | 9.53 | 11.30 | 13.04 | 14.78 | 16.51 | 18.24 | 19.97 | 21.69 | 23.42 |

The engine's queue reaches the line earlier: 2.4 s earlier at rank 1, narrowing to 0.3 s at rank 12. The gap comes from rank 1's short start and shrinks with each later rank. The paper's average row stands for no site.

## What this decides and what it does not

- **S1-a passes** its band: 1,944 veh/h against HCM 7's 1,900.
- **S1-f misses**: −6.7 % against 2,083.
- **S1-d** passes 3 of its 8 banded ranks.
- These are the verdicts of D516's bands, recorded as they fell. They neither calibrate nor change any model parameter (rule 4).
- **Per §6**, a miss of S1-a or S1-f calls for the D530 diagnostic: `t_x`, `L_hj` and `v_S` against [AkcBes02]'s 1.17 s, 6.9 m and 45.1 km/h. Its measurement contract is still to write. Until then, the cause of the S1-f miss is not claimed.
- **Owner's call:** whether S1's gate reads "S1-a passes" (the primary row, §6) or needs S1-f and S1-d as well. Recorded in NEXT.
