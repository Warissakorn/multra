# M6_S2_RUN — M6 kind S2: the delay runs, S2-c (D538), S2-b and S2-a (D539)

Contract for S2-c ([m6-benchmark-options](../evidence/m6-benchmark-options.md) §6). The engine's
delay on a one-lane signalised approach is compared with van Leeuwaarden's FCTL mean delay
([FCTL_REFERENCE](FCTL_REFERENCE.md), D529). Written before code and before any engine figure;
rows S2C1–S2C6 fail first. A miss is reported, never tuned away (rule 4).

## 1. The projects (built in memory, not shipped)

`tools/m6_s2_network.hpp` builds `m6s2::approach(v, signal)` through the editor commands.

- **Network.** The S1 layout ([M6_S1_RUN](M6_S1_RUN.md) §1), left-hand:
  - one lane, 3.5 m wide;
  - an approach Link 301 m long, with the head 1 m before its end;
  - a 20 m straight Connector to a 100 m exit Link.
- **Signal.** One fixed-time controller, one group: cycle **60.6 s**, green **0–30.3 s**, amber 0, red for the rest.
  - The slot model's 16 slots of `h` = 3600 / 1900 = 1.8947 s are 30.316 s each.
  - Timings must lie on the 0.1 s grid (`signal_control.cpp`), so each phase is 0.016 s shorter. This is stated beside the figure.
- **No-signal twin** (`signal` false): the same document without the head and the controller.
- **Travel-time section.** From the approach at station 50 m to the exit at station 29 m: route distances 50 m and 350 m, that is, 250 m upstream to 50 m past the line ([TRAVEL_TIME_SECTIONS](TRAVEL_TIME_SECTIONS.md)).
- **Demand.** One input, `car`, `v` veh/h from 0 to 4,500 s. Sources are Poisson processes (SIMULATION).
- **Run.**
  - dt 0.1 s, authored duration 4,500 s;
  - evaluation warm-up 900 s, window end 4,500 s, cool-down 300 s: trips released in the window count (D146);
  - the cool-down adds ticks, not demand ([BATCH](BATCH.md) §5), so the run lasts 4,800 s and the input ends at 4,500 s;
  - `perInput` streams;
  - `default.json` behaviour, no assignment.

| `x` | 0.5 | 0.7 | 0.85 |
|---|---|---|---|
| μ = `x`/2 per slot | 0.25 | 0.35 | 0.425 |
| `v` = μ · 1900 (veh/h) | 475.0 | 665.0 | 807.5 |

## 2. The command

`multra-m6-s2-run c <data-dir> <compile_commands.json> <new-output-dir>` (named `multra-m6-s2c-run` until D539)

- **Refusals.** The tool refuses an existing output directory. Exit status 1 with a message.
- **Runs.** For each `x`, the signal and no-signal documents run seeds 42–81 with `runSeeds`, aggregated by `aggregate` (`src/runner/batch.hpp`). The figure reads each document's `sections[0].meanTravelTime.perSeed`.
- **Outputs**, in the new directory:
  - the six project files;
  - `runs.csv`: one row per `x` and seed, with `x, seed, ttSignal, ttNone, delta, overloadedSignal, unfinishedSignal`;
  - `summary.json`: §3 and §4;
  - `metadata.json`, written **last** with `complete: true`. It holds the build commit, compiler command, tool source hashes, input manifest, the six documents' hashes and the output hashes.

## 3. The reduction

- **Per seed:** `Δ = TT_signal − TT_none`, where each term is the section's mean travel time in that run.
  - Under `perInput` the two runs of a seed share their input's arrivals, so they are a pair (D526).
  - A seed missing either value is listed and left out of `n`.
- **The figure:** the mean of Δ over seeds, with `n`, `sd` and the 95 % half-width `tQuantile975(n − 1) · sd / √n`.
- **Shown beside it:**
  - both mean travel times;
  - the seeds the batch flags as overloaded;
  - the mean unfinished trips in the signal run.

## 4. Reference and verdict

- **Reference.** `h · E D`, from `fctlPoisson({16, 16, x/2})` (`src/eval/fctl_reference.hpp`) times `h` = 3600 / 1900. The tool computes it, and S2C5 checks it against §6's 10.98, 13.49 and 19.57 s.
- **Verdict.** The figure **passes** when `|mean − reference| ≤ 0.15 · reference`, edge included (D516). Otherwise it **misses**. An absent figure is **unavailable**.
- **Shown beside the verdict, not added to the reference:**
  - the in-slot wait, at most `h` = 1.89 s per delayed vehicle (FCTL_REFERENCE §3);
  - the engine's measured saturation flow, 1,944 veh/h ([m6-s1-first-run](../evidence/m6-s1-first-run.md)), against the declared 1,900 that sets `h` and `v`. The engine's queue therefore clears about 2 % faster than the slot model's.
- **Also stated:**
  - Δ includes the deceleration and acceleration around the line, which a vertical-queue reference does not hold;
  - arrivals are Poisson at the source, 300 m upstream, and following smooths them before the line;
  - green and red are each 0.016 s shorter than 16 slots.

## 5. Acceptance rows

| Row | What must hold | Test |
|---|---|---|
| S2C1 | `approach(v, true)` compiles to one program with cycle 60.6 s and green 30.3 s, one input at `v`, `perInput`, a compiled run of 4,800 s, and a section whose lines sit at route distances 50 and 350 m (1e−6). `approach(v, false)` has no head and no program, and the same section and input | `m6s2` |
| S2C2 | `x` = 0.5, 0.7, 0.85 give μ 0.25, 0.35, 0.425 and `v` 475, 665, 807.5 | `m6s2` |
| S2C3 | Synthetic per-seed travel times give the hand Δ mean and half-width; a seed missing on either side is listed and excluded from `n` | `m6s2` |
| S2C4 | The verdict passes at exactly ±15 %, misses just beyond, and is unavailable without a figure | `m6s2` |
| S2C5 | The references equal 10.98, 13.49 and 19.57 s within 0.005 | `m6s2` |
| S2C6 | The command prints usage for `--help` and refuses an existing output directory | `cli-m6-s2-run-help`, `cli-m6-s2-run-refused` |

## 6. S2-b and S2-a: delay against HCM 7's `d1 + d2` (D539)

The same tool in mode `ab`: `multra-m6-s2-run ab <data-dir> <compile_commands.json> <new-output-dir>`.
§2's outputs, §3's paired reduction and §4's verdict rule apply, with the changes below. The
owner chose to lengthen the road and the cool-down: at `x` ≥ 1 the queue would spill past a section
that starts 250 m upstream, and past the source, and that delay would go uncounted.

- **Layout** (`m6s2::Layout`; S2-c keeps its own, so S2C1 is unchanged):
  - approach 2,001 m, head at 2,000 m;
  - the same 20 m Connector and 100 m exit;
  - the section from route distance 50 m (1,950 m upstream of the line) to 2,050 m (50 m past it);
  - the no-signal twin has the same length.
- **Signal:** cycle 90 s, green 0–40 s, amber 3 s, so `g` = 40 s (HCM 7 Eq. 19-1 and 19-3, §6).
- **Run:** authored duration 4,500 s, warm-up 900 s, window to 4,500 s, **cool-down 900 s**. The run lasts 5,400 s and the input ends at 4,500 s.
- **Levels:** `c` = 1,900 · 40 / 90 = 844.4 veh/h and `v = x · c`.

  | `x` | 0.5 | 0.7 | 0.85 | 0.95 | 1.0 | 1.05 |
  |---|---|---|---|---|---|---|
  | `v` (veh/h) | 422.2 | 591.1 | 717.8 | 802.2 | 844.4 | 886.7 |
  | `d1 + d2` (s) | 19.98 | 25.09 | 33.90 | 54.34 | 86.94 | 147.81 |
  | Row | S2-b | S2-b | S2-b, S2-a | S2-b, S2-a | S2-a | S2-a |

- **Reference.** `hcmDelay(x)` computes `d1 = 0.5C(1 − g/C)² / [1 − min(1, X)g/C]` (Eq. 19-19, `PF` = 1) and `d2 = 900T[(X − 1) + √((X − 1)² + 8kIX/(cT))]` (Eq. 19-26, 19-27), with `T` = 1 h, `k` = 0.5, `I` = 1 and `d3` = 0. These are §6's forms, read from the page images.
- **Verdict.** ±15 % (D516). Each level is judged once and reported under every row it belongs to.
- **Availability.** A level is **unavailable** when any seed of its signal run ends with a window trip unfinished in the section, or with a vehicle still pending at a source. It is then neither a pass nor a miss.
- **Stated beside the figures:**
  - **Initial queue.** At `x` ≥ 1 the 900 s warm-up builds a queue before the window, while the reference's `d3` = 0 assumes none. This pulls the engine up.
  - **Capacity.** The engine discharges at 1,944 veh/h ([m6-s1-first-run](../evidence/m6-s1-first-run.md)), so its capacity is about 864 veh/h, not 844. At the nominal `x` = 1.0 its own degree of saturation is about 0.977. This pulls `d2`, and so the engine, down near saturation.
  - **Δ** holds the deceleration and acceleration around the line (§4).

| Row | What must hold | Test |
|---|---|---|
| S2AB1 | The `ab` layout compiles to one program with cycle 90 s, green 40 s and amber 3 s; a section at route distances 50 and 2,050 m (1e−6); a run of 5,400 s. The twin has no program and the same section | `m6s2` |
| S2AB2 | `x` = 0.5 … 1.05 give `v` = 422.2, 591.1, 717.8, 802.2, 844.4, 886.7 (0.05) | `m6s2` |
| S2AB3 | `hcmDelay` equals 19.98, 25.09, 33.90, 54.34, 86.94, 147.81 s within 0.005 | `m6s2` |
| S2AB4 | `available` is false when any seed has unfinished > 0 or pending > 0, and the verdict is then unavailable | `m6s2` |
| S2AB5 | Mode `c` or `ab` is required; an unknown mode is refused with usage | `cli-m6-s2-run-mode` |

## 7. The arrival diagnostic at the line (D540)

S2-b and S2-a miss below HCM's `d2` ([m6-s2ab-first-run](../evidence/m6-s2ab-first-run.md)). The
hypothesis to test: Poisson arrivals at a source 2 km upstream are smoothed by following before
they reach the line. `d2` assumes random arrivals there (`I` = 1). This section **decides nothing
and has no band**; it changes no model or setup.

- **Command:** `multra-m6-s2-run arrivals <data-dir> <compile_commands.json> <new-output-dir>`. It runs the §6 no-signal layout at every §6 `x`, seeds 42–81, in its own tick loop.
- **Observer.** At route distances **100 m** (just after the source) and **2,000 m** (the stop-line position), it stamps each vehicle once, at the end-of-tick time of the first tick in which its front is at or past the point. This is DISCHARGE's convention; the quantisation is up to one 0.1 s step.
- **Per seed, over the window [900, 4500) s, at each point:**
  - **IDC**, the index of dispersion of counts: the sample variance (n − 1) over the mean of the counts in the 40 consecutive 90 s intervals;
  - **CV**, the sample standard deviation over the mean of the headways between consecutive stamps in the window.

  For Poisson arrivals both are 1. Below 1 is smoother than Poisson; above 1 is bunched.
- **Reduction.** The mean over seeds with the 95 % half-width, per point and `x`.
- **Output:** `arrivals.csv` (one row per `x`, seed and point, with `count`, `idc`, `cv`), `summary.json`, and `metadata.json` written last.
- **Reading.** If the line's IDC is well below 1 while the 100 m point's is near 1, the S2-b/S2-a gap is a property of the setup's distance and not of the queue model. Changing the setup is then a separate owner decision with its own contract. Otherwise the gap belongs to the queue or discharge model.

| Row | What must hold | Test |
|---|---|---|
| S2D1 | An observer fed hand-built ticks stamps each vehicle once per point, at the first tick at or past it, and never before | `m6s2` |
| S2D2 | IDC and CV of hand-made stamps give hand values; intervals start at 900 s and are 90 s long | `m6s2` |
| S2D3 | Synthetic exponential headways at 0.2 veh/s from a fixed splitmix64 draw give IDC and CV within 0.1 of 1, and evenly spaced stamps give 0 | `m6s2` |
| S2D4 | The usage line lists `c\|ab\|arrivals`; an unknown mode is refused with it | `cli-m6-s2-run-help`, `cli-m6-s2-run-mode` |

