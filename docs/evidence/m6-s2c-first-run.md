# M6 kind S2-c — the first comparable delay run (D538), 2026-11-12

The engine's signal delay on one lane is compared with van Leeuwaarden's fixed-cycle
traffic-light queue (FCTL), as §6 of [m6-benchmark-options](m6-benchmark-options.md) asks.
Contract: [M6_S2_RUN](../reference/M6_S2_RUN.md), committed (`c7a4192`) before code; rows
S2C1–S2C6 were tested before any engine figure. **Nothing was tuned.**

## Run

```bash
./build/desktop/bin/multra-m6-s2c-run data build/desktop/compile_commands.json <new-output-dir>
```

- **Build and seeds:** build `7e2d9ab`, seeds 42–81 for each of six in-memory projects: `x` = 0.5, 0.7, 0.85, each with and without the signal. That is 240 runs in 4 min 47 s.
- **Projects:**
  - cycle 60.6 s, green 30.3 s, no amber;
  - section from 250 m upstream to 50 m past the line;
  - `perInput` streams, so each seed's signal and no-signal runs share their arrivals;
  - warm-up 900 s, window to 4,500 s, then a 300 s cool-down.
- **Committed here:**
  - [summary](m6-s2c-first-run-summary.json);
  - [per-seed values](m6-s2c-first-run-runs.csv);
  - [metadata](m6-s2c-first-run-metadata.json), with the tool sources, the six documents, the inputs, and the outputs' SHA-256.
- **Completeness:** no seed was missing a value, none was overloaded, and no window trip was unfinished.

## Result (mean of 40 seeds, 95 % CI)

| `x` | `v` (veh/h) | Δ = TT with signal − TT without | Reference `h · E D` | Difference | Verdict (±15 %) |
|---|---|---|---|---|---|
| 0.5 | 475.0 | **11.83 s** (± 0.31) | 10.98 s | +7.7 % | **pass** |
| 0.7 | 665.0 | **14.13 s** (± 0.38) | 13.49 s | +4.7 % | **pass** |
| 0.85 | 807.5 | **19.79 s** (± 1.02) | 19.57 s | +1.1 % | **pass** |

The section travel times behind each Δ:

| `x` | With signal (s) | Without signal (s) | Vehicles in the section per seed |
|---|---|---|---|
| 0.5 | 34.56 | 22.74 | 477.5 |
| 0.7 | 37.17 | 23.04 | 670.4 |
| 0.85 | 43.05 | 23.26 | 814.6 |

## Stated beside it

- **The in-slot wait.** The reference leaves out each delayed vehicle's wait inside its arrival slot, which is at most `h` = 1.89 s per vehicle (FCTL_REFERENCE §3). The figure is shown against the reference without it.
- **Deceleration and acceleration.** Δ holds the time lost slowing to the line and accelerating away from it. A vertical-queue reference does not hold this time. It plausibly explains why the engine sits above the reference, and most at the lowest `x`, but it is not measured here.
- **Two opposing effects:**
  - The engine discharges at 1,944 veh/h ([m6-s1-first-run](m6-s1-first-run.md)), against the 1,900 that sets the slot. On that count it should sit slightly below the reference.
  - Green and red are each 0.016 s shorter than 16 slots.
- **Arrivals.** They are Poisson at the source, 300 m upstream. Following smooths them before the line, while the reference assumes Poisson arrivals at the line.

## What this decides

**S2-c passes at all three degrees of saturation** within D516's ±15 %. The next §6 rows are:
- **S2-b:** HCM `d1 + d2` below saturation;
- **S2-a:** across saturation;
- **U-c and U-a:** minor-movement capacity.
