# Stopping at the line: options measured — 2026-11-24

Evidence for [LINE_STOP](../reference/LINE_STOP.md) (D651).

**What this is and isn't:**
- It measures temporary patches against today's engine, and every patch was reverted.
- Nothing here is calibration or validation (rule 4, M6). The unvalidated marker stays.
- The owner chooses the option. No engine change is committed.

## Inputs and method

- **Build:** commit `c193433` plus [`line-stop-options.patch`](line-stop-options.patch), applied, measured, then
  reverted. Linux, GCC 13.3.0, Release (`build/perf`, `-O3 -DNDEBUG -g1 -fno-fast-math -ffp-contract=off`).
- **Environment switches:** the patch adds three, read once per process.
  - **`ZZ_L1`.** Every line is handed to the model as D596's zone line. For the prototype that is a desired gap
    of 0. For W74 and W99 it is `gap + standstillGap`, so the model's own rest point lands on the line. The
    switch covers signal heads, give-way and Stop lines, zones and dead ends, including the dead end as a
    second obstacle.
  - **`ZZ_L3`.** The model follows vehicles only. A separate line law (SUMO's `stopSpeed` idea) brakes at the
    type's `comfortableDeceleration` so that the speed after the tick still stops exactly at the nearest line
    (ballistic tick). The kept acceleration is the lower of the two.
  - **`ZZ_FLOOR`.** For W74 and W99, while braking toward a standing obstacle (the line, or a stopped vehicle at
    `gap − standstillGap`), the acceleration is at most `−v²/(2·gap)`, capped at the type's maximum: D258's
    floor, extended.
- **The finish.** Under L1 and L3 a move that reaches a line cap finishes as GIVE_WAY_STOP §2's `stopAtLine`
  does. A rounding residue of up to 10⁻⁶ m at the line is rest (D596's finish), not a clamp. Without the finish,
  the first run of the matrix counted rounding residues as clamps: for example 93 per run for W74 under L3 on
  the four-leg, every one about 10⁻⁷ m past its line.
- **Runs:**
  - five shipped projects, seeds 42–51, at the project's own dt (0.1 s);
  - the project's catalogs captured as owned (`tools/w74_fixture_document.hpp`);
  - each W74 or W99 arm puts every vehicle type on that behaviour: the BA27 fixture, the SUMO-default W74
    preset, or the BA231 W99 fixture.
- **Measures, per vehicle with a line as its nearest obstacle within 15 m:**
  - **Rest:** the last distance to the line while slower than 0.1 m/s.
  - **Crawl:** the seconds spent at 0.1–1 m/s.
  - **Hard-braking ticks:** ticks below −2 m/s² after it had stood.
  - **Clamps:** every `SafetyClampEvent`, per run.
  - **Delay:** `travelTime − freeFlowTime` over arrivals.
  - **Discharge:** headway and startup lost time from `DischargeAccumulator` with the CLI default spec. Only the
    four-leg and M2.6 have queue counters.
- **Guards:**
  - Under L0 the patched engine gives `multra-cli 42` byte for byte, and so does the reverted one.
  - With every switch on, bare M0 changes: 1 → 5 clamps, mean delay 31.759 → 31.616 s. So any adopted option
    must be scoped away from bare scenarios (LINE_STOP §5).
- **Cost (callgrind):** `multra-cli --project four-leg-signalised --seed 42`, prototype, instructions:

  | Option | Instructions | Against L0 |
  |---|---|---|
  | L0 | 1,079,092,701 | — |
  | L1 | 1,078,547,767 | −0.05 % |
  | L3 | 1,091,356,088 | +1.14 % |

  The patch's own observation hook is in all three.

## Results (seeds 42–51)

### Four-leg (signal heads)

| Model | Option | Rest at the line, median / p90 / max (m) | Crawl < 1 m/s, median / max (s) | Hard-braking ticks | Clamps per run | Delay per vehicle (s) | Headway / startup lost time (s) |
|---|---|---|---|---|---|---|---|
| prototype | L0 | 2.20 / 2.43 / 3.32 | 11.2 / 14.7 | 1 | 0.0 | 47.28 | 2.052 / -0.108 |
| prototype | L1 | 0.00 / 0.00 / 0.00 | 0.6 / 2.9 | 72 | 0.0 | 47.08 | 1.977 / 2.014 |
| prototype | L1F | 0.00 / 0.00 / 0.00 | 0.6 / 2.9 | 72 | 0.0 | 47.08 | 1.977 / 2.014 |
| prototype | L3 | 0.00 / 0.00 / 0.00 | 0.4 / 1.2 | 652 | 0.0 | 47.64 | 1.980 / 2.019 |
| prototype | L3F | 0.00 / 0.00 / 0.00 | 0.4 / 1.2 | 652 | 0.0 | 47.64 | 1.980 / 2.019 |
| W74 fixture | L0 | 2.94 / 3.22 / 3.55 | 10.2 / 26.9 | 110104 | 9.1 | 96.29 | 3.117 / -0.652 |
| W74 fixture | L1 | 0.96 / 1.23 / 1.55 | 10.4 / 30.6 | 98816 | 10.8 | 94.59 | 3.156 / -0.950 |
| W74 fixture | L1F | 0.96 / 1.23 / 1.55 | 10.3 / 28.6 | 73821 | 7.9 | 94.73 | 3.156 / -1.063 |
| W74 fixture | L3 | 0.00 / 0.00 / 0.00 | 0.5 / 1.2 | 701 | 9.0 | 94.26 | 3.087 / 2.075 |
| W74 fixture | L3F | 0.00 / 0.00 / 0.00 | 0.5 / 1.2 | 310 | 8.8 | 94.35 | 3.097 / 2.039 |
| W74 SUMO preset | L0 | 3.36 / 3.36 / 11.95 | 13.5 / 73.6 | 41581 | 3.8 | 116.86 | 3.442 / -1.222 |
| W74 SUMO preset | L1 | 1.36 / 1.36 / 10.01 | 13.5 / 81.6 | 34061 | 3.7 | 115.42 | 3.489 / -1.649 |
| W74 SUMO preset | L1F | 1.36 / 1.36 / 9.83 | 13.5 / 81.8 | 33158 | 3.7 | 115.66 | 3.481 / -1.626 |
| W74 SUMO preset | L3 | 0.00 / 0.00 / 0.00 | 0.5 / 1.2 | 708 | 3.4 | 110.74 | 3.442 / 2.002 |
| W74 SUMO preset | L3F | 0.00 / 0.00 / 0.00 | 0.5 / 1.2 | 274 | 3.6 | 110.49 | 3.435 / 1.982 |
| W99 fixture | L0 | 1.40 / 1.40 / 1.55 | 1.3 / 19.3 | 186 | 322.6 | 50.79 | 1.961 / -0.107 |
| W99 fixture | L1 | 0.00 / 0.00 / 0.00 | 1.0 / 13.1 | 999 | 346.5 | 50.66 | 1.932 / 1.619 |
| W99 fixture | L1F | 0.00 / 0.00 / 0.00 | 1.3 / 13.1 | 236 | 344.6 | 50.62 | 1.932 / 1.620 |
| W99 fixture | L3 | 0.00 / 0.00 / 0.00 | 0.5 / 1.4 | 655 | 319.1 | 50.64 | 1.936 / 1.636 |
| W99 fixture | L3F | 0.00 / 0.00 / 0.00 | 0.5 / 1.4 | 133 | 337.8 | 50.63 | 1.939 / 1.628 |

### M2.6 study template (signal heads)

| Model | Option | Rest at the line, median / p90 / max (m) | Crawl < 1 m/s, median / max (s) | Hard-braking ticks | Clamps per run | Delay per vehicle (s) | Headway / startup lost time (s) |
|---|---|---|---|---|---|---|---|
| prototype | L0 | 2.20 / 2.44 / 4.99 | 11.1 / 15.8 | 279 | 0.1 | 46.15 | 2.442 / -0.375 |
| prototype | L1 | 0.00 / 0.00 / 0.00 | 0.6 / 2.1 | 757 | 224.7 | 44.23 | 2.336 / 1.954 |
| prototype | L1F | 0.00 / 0.00 / 0.00 | 0.6 / 2.1 | 757 | 224.7 | 44.23 | 2.336 / 1.954 |
| prototype | L3 | 0.00 / 0.00 / 0.00 | 0.5 / 14.2 | 2737 | 0.3 | 44.85 | 2.335 / 1.950 |
| prototype | L3F | 0.00 / 0.00 / 0.00 | 0.5 / 14.2 | 2737 | 0.3 | 44.85 | 2.335 / 1.950 |
| W74 fixture | L0 | 2.96 / 3.23 / 12.74 | 10.4 / 68.5 | 321662 | 29.9 | 145.97 | 3.766 / -1.393 |
| W74 fixture | L1 | 0.98 / 1.24 / 10.05 | 10.5 / 67.1 | 259102 | 337.6 | 132.07 | 3.785 / -1.998 |
| W74 fixture | L1F | 0.98 / 1.25 / 10.42 | 10.4 / 67.1 | 224259 | 371.0 | 131.40 | 3.811 / -2.077 |
| W74 fixture | L3 | 0.00 / 0.00 / 3.88 | 0.5 / 30.4 | 4492 | 27.9 | 124.11 | 3.664 / 1.681 |
| W74 fixture | L3F | 0.00 / 0.00 / 3.88 | 0.5 / 18.2 | 2519 | 30.1 | 125.51 | 3.641 / 1.748 |
| W74 SUMO preset | L0 | 3.36 / 3.36 / 14.18 | 13.8 / 82.1 | 215374 | 19.5 | 186.11 | 4.079 / -1.841 |
| W74 SUMO preset | L1 | 1.36 / 1.36 / 12.64 | 13.9 / 93.7 | 186363 | 38.5 | 171.15 | 4.139 / -2.437 |
| W74 SUMO preset | L1F | 1.36 / 1.36 / 14.83 | 13.9 / 90.8 | 193150 | 26.8 | 170.77 | 4.124 / -2.347 |
| W74 SUMO preset | L3 | 0.00 / 0.00 / 2.72 | 0.5 / 21.0 | 4457 | 24.0 | 149.71 | 3.991 / 1.583 |
| W74 SUMO preset | L3F | 0.00 / 0.00 / 3.34 | 0.5 / 25.9 | 2725 | 23.2 | 149.60 | 3.988 / 1.553 |
| W99 fixture | L0 | 1.40 / 1.40 / 1.51 | 1.3 / 21.6 | 1167 | 1205.9 | 48.24 | 2.412 / -0.737 |
| W99 fixture | L1 | 0.00 / 0.00 / 0.20 | 1.0 / 17.6 | 3921 | 1302.1 | 47.92 | 2.385 / 1.407 |
| W99 fixture | L1F | 0.00 / 0.00 / 0.00 | 1.3 / 17.6 | 1246 | 1282.4 | 47.91 | 2.383 / 1.410 |
| W99 fixture | L3 | 0.00 / 0.00 / 0.00 | 0.5 / 1.2 | 2805 | 1189.5 | 47.74 | 2.388 / 1.402 |
| W99 fixture | L3F | 0.00 / 0.00 / 0.00 | 0.5 / 1.6 | 701 | 1256.0 | 47.83 | 2.387 / 1.409 |

### T-junction (conflict zones)

| Model | Option | Rest at the line, median / p90 / max (m) | Crawl < 1 m/s, median / max (s) | Hard-braking ticks | Clamps per run | Delay per vehicle (s) |
|---|---|---|---|---|---|---|
| prototype | L0 | 0.00 / 0.00 / 0.00 | 0.6 / 1.5 | 97 | 0.0 | 6.03 |
| prototype | L1 | 0.00 / 0.00 / 0.00 | 0.6 / 1.5 | 97 | 0.0 | 6.03 |
| prototype | L1F | 0.00 / 0.00 / 0.00 | 0.6 / 1.5 | 97 | 0.0 | 6.03 |
| prototype | L3 | 0.00 / 0.00 / 0.00 | 0.5 / 0.5 | 170 | 0.0 | 5.94 |
| prototype | L3F | 0.00 / 0.00 / 0.00 | 0.5 / 0.5 | 170 | 0.0 | 5.94 |
| W74 fixture | L0 | 2.07 / 3.08 / 3.60 | 2.2 / 16.6 | 6745 | 0.0 | 10.81 |
| W74 fixture | L1 | 0.32 / 1.23 / 1.60 | 2.6 / 18.1 | 26 | 0.0 | 9.66 |
| W74 fixture | L1F | 0.31 / 1.19 / 1.60 | 2.3 / 18.1 | 25 | 0.0 | 9.68 |
| W74 fixture | L3 | 0.00 / 0.00 / 0.00 | 0.4 / 1.5 | 175 | 0.0 | 8.08 |
| W74 fixture | L3F | 0.00 / 0.00 / 0.00 | 0.5 / 1.4 | 83 | 0.0 | 8.09 |
| W74 SUMO preset | L0 | 2.10 / 3.36 / 3.87 | 1.0 / 20.0 | 8190 | 0.0 | 13.63 |
| W74 SUMO preset | L1 | 0.26 / 1.36 / 1.75 | 1.4 / 20.3 | 24 | 0.0 | 11.95 |
| W74 SUMO preset | L1F | 0.34 / 1.36 / 1.75 | 1.3 / 24.1 | 24 | 0.0 | 11.97 |
| W74 SUMO preset | L3 | 0.00 / 0.00 / 0.00 | 0.5 / 1.4 | 182 | 0.0 | 9.03 |
| W74 SUMO preset | L3F | 0.00 / 0.00 / 0.00 | 0.5 / 1.5 | 87 | 0.0 | 9.00 |
| W99 fixture | L0 | 1.40 / 1.40 / 1.42 | 0.5 / 4.4 | 123 | 15.1 | 6.90 |
| W99 fixture | L1 | 0.00 / 0.00 / 0.00 | 0.3 / 5.8 | 162 | 12.8 | 6.03 |
| W99 fixture | L1F | 0.00 / 0.00 / 0.00 | 0.5 / 6.3 | 106 | 9.4 | 6.27 |
| W99 fixture | L3 | 0.00 / 0.00 / 0.00 | 0.4 / 0.5 | 178 | 11.4 | 6.09 |
| W99 fixture | L3F | 0.00 / 0.00 / 0.00 | 0.5 / 0.5 | 90 | 10.1 | 6.09 |

### Priority-rule lab (give-way and Stop lines)

| Model | Option | Rest at the line, median / p90 / max (m) | Crawl < 1 m/s, median / max (s) | Hard-braking ticks | Clamps per run | Delay per vehicle (s) |
|---|---|---|---|---|---|---|
| prototype | L0 | 2.95 / 3.36 / 5.90 | 10.1 / 18.5 | 906 | 0.0 | 8.94 |
| prototype | L1 | 0.00 / 0.00 / 0.00 | 0.7 / 2.2 | 709 | 5.4 | 5.40 |
| prototype | L1F | 0.00 / 0.00 / 0.00 | 0.7 / 2.2 | 709 | 5.4 | 5.40 |
| prototype | L3 | 0.00 / 0.00 / 0.00 | 0.5 / 1.0 | 1809 | 0.0 | 5.16 |
| prototype | L3F | 0.00 / 0.00 / 0.00 | 0.5 / 1.0 | 1809 | 0.0 | 5.16 |
| W74 fixture | L0 | 2.94 / 3.31 / 3.58 | 11.2 / 23.9 | 5536 | 0.0 | 19.77 |
| W74 fixture | L1 | 1.31 / 1.70 / 2.33 | 11.7 / 31.7 | 504 | 0.0 | 17.13 |
| W74 fixture | L1F | 1.29 / 1.66 / 2.33 | 11.9 / 31.9 | 464 | 0.0 | 17.49 |
| W74 fixture | L3 | 0.00 / 0.09 / 2.04 | 0.5 / 1.6 | 2970 | 0.0 | 5.75 |
| W74 fixture | L3F | 0.00 / 0.09 / 1.85 | 0.5 / 3.8 | 1656 | 0.0 | 5.77 |
| W74 SUMO preset | L0 | 3.42 / 3.52 / 3.91 | 13.8 / 23.6 | 5297 | 0.0 | 24.69 |
| W74 SUMO preset | L1 | 1.77 / 1.92 / 1.94 | 14.2 / 28.1 | 891 | 0.0 | 17.74 |
| W74 SUMO preset | L1F | 1.79 / 1.92 / 1.94 | 14.2 / 30.3 | 853 | 0.0 | 17.69 |
| W74 SUMO preset | L3 | 0.00 / 0.04 / 1.77 | 0.5 / 3.9 | 3123 | 0.0 | 5.89 |
| W74 SUMO preset | L3F | 0.00 / 0.02 / 2.01 | 0.5 / 2.7 | 2249 | 0.0 | 5.90 |
| W99 fixture | L0 | 1.40 / 1.41 / 1.45 | 1.0 / 5.2 | 928 | 5.5 | 6.41 |
| W99 fixture | L1 | 0.00 / 0.00 / 0.08 | 1.2 / 10.3 | 849 | 14.6 | 6.19 |
| W99 fixture | L1F | 0.00 / 0.01 / 0.03 | 1.6 / 10.3 | 837 | 4.5 | 6.34 |
| W99 fixture | L3 | 0.00 / 0.00 / 0.00 | 0.5 / 1.0 | 1953 | 1.3 | 5.13 |
| W99 fixture | L3F | 0.00 / 0.00 / 0.00 | 0.5 / 1.0 | 909 | 1.1 | 5.17 |

### Lane-change lab (dead ends)

| Model | Option | Rest at the line, median / p90 / max (m) | Crawl < 1 m/s, median / max (s) | Hard-braking ticks | Clamps per run | Delay per vehicle (s) |
|---|---|---|---|---|---|---|
| prototype | L0 | — / — / — | — / — | 0 | 0.0 | 8.13 |
| prototype | L1 | — / — / — | — / — | 0 | 0.0 | 8.13 |
| prototype | L1F | — / — / — | — / — | 0 | 0.0 | 8.13 |
| prototype | L3 | 0.00 / 0.00 / 0.00 | 0.4 / 0.5 | 3 | 0.0 | 8.15 |
| prototype | L3F | 0.00 / 0.00 / 0.00 | 0.4 / 0.5 | 3 | 0.0 | 8.15 |
| W74 fixture | L0 | — / — / — | 2.9 / 4.5 | 0 | 0.0 | 9.30 |
| W74 fixture | L1 | — / — / — | 5.5 / 7.3 | 0 | 0.0 | 9.30 |
| W74 fixture | L1F | — / — / — | 5.5 / 7.3 | 0 | 0.0 | 9.30 |
| W74 fixture | L3 | 0.00 / 0.00 / 0.00 | 0.5 / 0.5 | 4 | 0.0 | 9.30 |
| W74 fixture | L3F | 0.00 / 0.00 / 0.00 | 0.5 / 0.5 | 0 | 0.0 | 9.31 |
| W74 SUMO preset | L0 | — / — / — | 4.7 / 4.7 | 0 | 0.0 | 9.13 |
| W74 SUMO preset | L1 | — / — / — | 4.8 / 4.8 | 0 | 0.0 | 9.13 |
| W74 SUMO preset | L1F | — / — / — | 4.8 / 4.8 | 0 | 0.0 | 9.13 |
| W74 SUMO preset | L3 | 0.00 / 0.00 / 0.00 | 0.4 / 0.5 | 7 | 0.0 | 9.12 |
| W74 SUMO preset | L3F | 0.00 / 0.00 / 0.00 | 0.5 / 0.5 | 0 | 0.0 | 9.13 |
| W99 fixture | L0 | 1.40 / 1.40 / 1.40 | 1.4 / 1.4 | 0 | 0.0 | 8.49 |
| W99 fixture | L1 | 0.00 / 0.00 / 0.00 | 1.1 / 1.1 | 1 | 0.0 | 8.49 |
| W99 fixture | L1F | 0.00 / 0.00 / 0.00 | 1.4 / 1.4 | 0 | 0.0 | 8.49 |
| W99 fixture | L3 | 0.00 / 0.00 / 0.00 | 0.5 / 0.5 | 8 | 0.0 | 8.50 |
| W99 fixture | L3F | 0.00 / 0.00 / 0.00 | 0.5 / 0.5 | 0 | 0.0 | 8.50 |

## Reading

- **L3 (and L3F) puts every model at the line, on every line type.** Rest is 0 m at the median and p90.
  - The maximum is up to 3.9 m on M2.6, where a vehicle stands short of a line it is still releasing from.
  - Crawls fall from 10–14 s to about 0.5 s.
  - Startup lost time moves from about −0.1 to −1.8 s to about +1.4 to +2.1 s, toward HCM's default 2.0 s. The
    estimator's ranks differ from HCM's definition, so this is a direction, not a match.
  - W74's delay at the priority lab falls from 19.8 / 24.7 s to 5.8 / 5.9 s: its 11–14 s crawl at the line is
    gone.
- **L1 is the prototype's D596, extended.** It brings the prototype to the line, but on M2.6 it clamps 225 times
  per run, where L3 clamps 0.3. W74 still rests about 1 m short (its `ABX(0.1)` band, W74 §7) and still crawls
  10–14 s.
- **The floor (F)** roughly halves the hard-braking ticks for W74 and W99 under L3, with no clamp change. It does
  nothing for the prototype, which has its own floor (D258).
- **Harder stops, later.** L3 stops later and at the type's comfortable deceleration, as SUMO does. That is why
  the prototype's hard-braking ticks rise on the four-leg (1 → 652 over ten seeds). They are not clamps.
- **W99's arrival clamps are not lines' clamps.** They stay about 320 per run on the four-leg and about 1,200
  on M2.6 under every option. W99 §4's creep keeps `max(aPrev, CC7) > 0` into the band, so the car is still
  accelerating when the stop becomes infeasible. Braking floors cannot reach that. It is W99 §4's question
  (LINE_STOP §6).
- **Cost:** L3 costs +1.1 % instructions on the four-leg. L1 costs nothing.
