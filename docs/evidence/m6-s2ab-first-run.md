# M6 kinds S2-b and S2-a — delay against HCM 7's `d1 + d2` (D539), 2026-11-12

The engine's signal delay on one lane is compared with HCM 7's uniform and incremental delay, as
§6 of [m6-benchmark-options](m6-benchmark-options.md) asks. Contract: [M6_S2_RUN](../reference/M6_S2_RUN.md)
§6, committed (`129f6fe`) before code; rows S2AB1–S2AB5 were tested before any engine figure.
**Nothing was tuned.**

## Run

```bash
./build/desktop/bin/multra-m6-s2-run ab data build/desktop/compile_commands.json <new-output-dir>
```

- **Build and size:** build `c312146`, seeds 42–81 for each of 12 in-memory projects: `x` = 0.5, 0.7, 0.85, 0.95, 1.0, 1.05, each with and without the signal. The 480 runs took 40 min.
- **Projects:**
  - 2,001 m approach;
  - cycle 90 s, green 40 s, amber 3 s;
  - section from 1,950 m upstream to 50 m past the line;
  - warm-up 900 s, window to 4,500 s, cool-down 900 s;
  - `perInput` streams, so each pair of runs shares its arrivals.
- **Committed here:** [summary](m6-s2ab-first-run-summary.json), [per-seed values](m6-s2ab-first-run-runs.csv) and [metadata](m6-s2ab-first-run-metadata.json).
- **Completeness:** at every level, no window trip was unfinished, no vehicle was pending at a source, and no seed was overloaded. **Every level is available.**
- **Check on the tool rename:** a rerun of mode `c` reproduced S2-c's first run exactly (same documents, per-seed values and figures).

## Result (mean of 40 seeds, 95 % CI)

| `x` | `v` (veh/h) | Δ engine (s) | `d1 + d2` (s) | Difference | S2-b | S2-a |
|---|---|---|---|---|---|---|
| 0.5 | 422.2 | **18.51** (± 0.73) | 19.98 | −7.4 % | **pass** | |
| 0.7 | 591.1 | **21.04** (± 0.80) | 25.09 | −16.2 % | **miss** | |
| 0.85 | 717.8 | **24.61** (± 1.18) | 33.90 | −27.4 % | **miss** | **miss** |
| 0.95 | 802.2 | **32.09** (± 2.43) | 54.34 | −40.9 % | **miss** | **miss** |
| 1.0 | 844.4 | **42.42** (± 4.58) | 86.94 | −51.2 % | | **miss** |
| 1.05 | 886.7 | **69.55** (± 11.65) | 147.81 | −52.9 % | | **miss** |

**S2-b passes 1 of 4 levels; S2-a passes 0 of 4.** The engine's delay sits below HCM's, and
further below as `x` rises.

The section travel times behind Δ (s):

| `x` | 0.5 | 0.7 | 0.85 | 0.95 | 1.0 | 1.05 |
|---|---|---|---|---|---|---|
| With signal | 177.10 | 182.52 | 187.88 | 196.42 | 207.27 | 234.87 |
| Without signal | 158.59 | 161.48 | 163.27 | 164.33 | 164.85 | 165.32 |

## Context, shown only: HCM's terms and the engine's capacity

The values below are not the reference and decide nothing. They show where the gap lies.

| `x` (nominal) | 0.5 | 0.7 | 0.85 | 0.95 | 1.0 | 1.05 |
|---|---|---|---|---|---|---|
| `d1` at `s` 1,900 | 17.86 | 20.16 | 22.32 | 24.04 | 25.00 | 25.00 |
| `d2` at `s` 1,900 | 2.13 | 4.93 | 11.58 | 30.30 | 61.94 | 122.81 |
| `d1 + d2` at the engine's `s` 1,944 (own `X`) | 19.73 | 24.43 | 31.92 | 46.61 | 68.06 | 114.99 |
| Engine Δ | 18.51 | 21.04 | 24.61 | 32.09 | 42.42 | 69.55 |

- **The engine's higher capacity explains only part of the miss.** At the engine's own measured `s` (1,944 veh/h, [m6-s1-first-run](m6-s1-first-run.md)), HCM still gives 31.9 s at the nominal 0.85 and 68.1 s at 1.0. The engine gives 24.6 s and 42.4 s.
- **The engine's Δ sits close to the uniform delay `d1`**, plus an incremental part much smaller than `d2`. The gap is in the random-overflow term.
- **A hypothesis, not measured here.** HCM's `d2` assumes random arrivals at the line. The engine's arrivals are Poisson at a source 2,000 m upstream, and car-following on the way can only lengthen short headways, so arrivals reach the line smoother than Poisson.
  - S2-c ran on a 300 m road against an exact Poisson queue model and passed.
  - A measurement of the arrival headways at the line in the no-signal runs would test this hypothesis. That is not done here.
- **Initial queue.** The 900 s warm-up's initial queue at `x` ≥ 1 would pull the engine up, against the direction of the gap.

## What this decides

- **S2-b:** passes at `x` = 0.5 and misses at 0.7, 0.85 and 0.95.
- **S2-a:** misses at 0.85, 0.95, 1.0 and 1.05.
- These are D516's verdicts, recorded as they fell. They change no model parameter (rule 4).
- **Owner's call:** whether to measure the arrival process at the line (a diagnostic like D530), to rerun with the source closer to the line, or to accept the record as it stands.
