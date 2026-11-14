# M6 S2 — the arrival process at the line (D540), 2026-11-13

S2-b and S2-a miss below HCM's `d2` ([m6-s2ab-first-run](m6-s2ab-first-run.md)). The hypothesis was
that Poisson arrivals at a source 2 km upstream are smoothed by following before they reach the line.
Contract: [M6_S2_RUN](../reference/M6_S2_RUN.md) §7 (`0538b69`), rows S2D1–S2D4 tested before the
run. **No band; it decides nothing, and no model or setup was changed.**

## Run

```bash
./build/desktop/bin/multra-m6-s2-run arrivals data build/desktop/compile_commands.json <new-output-dir>
```

- **Runs:** build `f71cf5f`; the §6 no-signal projects at the six `x`; seeds 42–81. The 240 runs took 17 min.
- **Measured:** stamps at route distances 100 m and 2,000 m, the line's position, counted over [900, 4500) s.
- **Committed here:** [summary](m6-s2-arrivals-summary.json), [per-seed values](m6-s2-arrivals.csv) and [metadata](m6-s2-arrivals-metadata.json).

## Result (mean of 40 seeds, 95 % CI; Poisson gives 1)

| `x` | `v` (veh/h) | IDC at 100 m | IDC at the line | Headway CV at 100 m | Headway CV at the line |
|---|---|---|---|---|---|
| 0.5 | 422.2 | 0.96 (± 0.08) | **1.25** (± 0.08) | 0.95 (± 0.02) | **1.41** (± 0.01) |
| 0.7 | 591.1 | 0.89 (± 0.06) | **1.23** (± 0.08) | 0.91 (± 0.01) | **1.51** (± 0.02) |
| 0.85 | 717.8 | 0.92 (± 0.07) | **1.25** (± 0.09) | 0.88 (± 0.02) | **1.53** (± 0.02) |
| 0.95 | 802.2 | 0.87 (± 0.07) | **1.18** (± 0.08) | 0.84 (± 0.02) | **1.53** (± 0.01) |
| 1.0 | 844.4 | 0.85 (± 0.07) | **1.18** (± 0.08) | 0.82 (± 0.01) | **1.53** (± 0.01) |
| 1.05 | 886.7 | 0.86 (± 0.07) | **1.19** (± 0.08) | 0.80 (± 0.01) | **1.52** (± 0.01) |

## What it shows

- **The hypothesis is not supported.** Arrivals at the line are **more** dispersed than Poisson, not less.
  - The 90 s counts' index of dispersion is about 1.2.
  - The headway CV is about 1.5: platoons with long gaps between them, as faster drivers close up behind slower ones over 2 km.
- **Near the source the stream is slightly smoother than Poisson.** IDC is 0.85–0.96 and CV 0.80–0.95, falling as demand rises. This fits a source that cannot release a vehicle into an occupied entry.
- **The direction is the opposite of the gap.** Bunched arrivals would raise the overflow delay above `d2` with `I` = 1, not lower it. So the S2-b and S2-a gap is **not** a smoothing artefact of the setup's distance. It belongs to the queue and discharge side, or to how HCM's `d2` relates to this queue.
- **Candidates on that side, not measured here:**
  - The engine's effective green may exceed the 40 s assumed: it uses part of the amber, and its rank-1 crossing comes 1.38 s after Go ([m6-s1-first-run](m6-s1-first-run.md)).
  - The engine's saturation flow is 1,944 veh/h.
  - HCM's `d2` is a time-dependent approximation. S2-c compared the engine with an exact Poisson queue model, on a short road with no amber, and passed.

## For the owner

- Moving the source closer to the line would **not** be expected to close the gap. It would remove the bunching, which works the other way.
- **Two next measurements would split the remaining candidates:**
  - the engine's effective green and lost time per cycle;
  - FCTL's exact delay at the S2-b settings (whole slots near 90/40/3 s), to show how far `d2` sits from an exact Poisson queue there.
- Both are diagnostics. Any change to the model stays one calibration decision with S1's jam spacing (D540).
