# M6_S2_RUN — M6 kind S2-c, the fixed-cycle queue delay run (D538)

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
- **Demand.** One input, `car`, `v` veh/h over the whole run. Sources are Poisson processes (SIMULATION).
- **Run.**
  - dt 0.1 s, duration 4,800 s;
  - evaluation warm-up 900 s, window end 4,500 s, cool-down 300 s: trips released in the window count (D146);
  - `perInput` streams;
  - `default.json` behaviour, no assignment.

| `x` | 0.5 | 0.7 | 0.85 |
|---|---|---|---|
| μ = `x`/2 per slot | 0.25 | 0.35 | 0.425 |
| `v` = μ · 1900 (veh/h) | 475.0 | 665.0 | 807.5 |

## 2. The command

`multra-m6-s2c-run <data-dir> <compile_commands.json> <new-output-dir>`

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
| S2C1 | `approach(v, true)` compiles to one program with cycle 60.6 s and green 30.3 s, one input at `v`, `perInput`, and a section whose lines sit at route distances 50 and 350 m (1e−6). `approach(v, false)` has no head and no program, and the same section and input | `m6s2` |
| S2C2 | `x` = 0.5, 0.7, 0.85 give μ 0.25, 0.35, 0.425 and `v` 475, 665, 807.5 | `m6s2` |
| S2C3 | Synthetic per-seed travel times give the hand Δ mean and half-width; a seed missing on either side is listed and excluded from `n` | `m6s2` |
| S2C4 | The verdict passes at exactly ±15 %, misses just beyond, and is unavailable without a figure | `m6s2` |
| S2C5 | The references equal 10.98, 13.49 and 19.57 s within 0.005 | `m6s2` |
| S2C6 | The command prints usage for `--help` and refuses an existing output directory | `cli-m6-s2c-run-help`, `cli-m6-s2c-run-refused` |
