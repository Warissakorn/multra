# M3.3.3a BA27: W74 queue-discharge timestep sensitivity — 2026-10-08

Row BA27 of [the delivery plan](../plans/DRIVING_BEHAVIOUR.md#3-failure-first-acceptance-rows):
queue discharge with a `w74` behaviour at dt 0.1 / 0.25 / 0.5, seeds 42–81, **sensitivity
recorded, no bound, no calibration or validation inference** (rule 4, W74.md §8). The
not-yet-validated marker stays. The prototype arm is the same measurement for comparison.

## Inputs

- Commit `0b75653` plus this session's tool `tools/w74_discharge_sweep.cpp` (no engine
  change). GCC 13.3.0, Linux, CMake Release: `-O3 -DNDEBUG -std=c++20 -fno-fast-math
  -ffp-contract=off` (a separate `-DTRAFFICSIM_BUILD_DESKTOP=OFF` build tree).
- Command: `trafficsim-w74-discharge-sweep docs/evidence/w74-discharge-behaviour.json
  data/projects/four-leg-signalised.traffic.json data <out.csv>`.
- The tool captures the project's catalogs as owned (`putDemandCatalog`), and for the `w74`
  arm adds the behaviour file and points every vehicle type at it; `changeRunSettings`
  sets dt; the discharge spec is the CLI default (whole run, ranks 3–5 steady, 1–2 startup).

| Input | SHA-256 |
|---|---|
| `data/projects/four-leg-signalised.traffic.json` | `3b0eb4450904ed66ef6f698b24280f7318a6031d6423dc60e16a2ba11542c016` |
| `docs/evidence/w74-discharge-behaviour.json` | `fdc205f8b4b9adaa7fd10d53b21f41eab574ee0ea45ba5bb5e87dede5f64c63c` |
| `data/vehicle-types/car.json` | `c8d99f76ed01dec7d91866d239b40ed1e682a1f4358a9c8ca084fc4dec8d87e9` |
| `data/vehicle-types/heavy-vehicle.json` | `158e88a84ef482b054c727900a5c91d41732dae639c5804667f8fe6d73ddaca8` |
| `data/driver-behaviour/default.json` | `8aca6246993f61865e4b7a67d655cfccf3a0914de123dabf7e45735c336d3cdc` |
| `data/evaluation/queue-counter.json` | `04bff220abb06e80db644e1b94afb53f3e0af790ffe2371e06eff5f2b82a1de9` |
| `data/compositions/car-only.json`, `urban-mixed.json` | `01e212b1…702dd`, `13618f4f…69531` |

**The W74 parameter values are fixture choices, not a preset and not calibrated.** `ax 2`,
`bxAdd 2`, `bxMult 3` are the values PTV's W74 parameter page uses as its defaults for the
same names; the other 15 keys (`exAdd 1.5`, `exMult 1`, `cxAdd 25`, `cxMult 15`,
`opdvAdd 1.5`, `opdvMult 0.5`, `dMax 150`, `bMaxAdd 1.5`, `bMaxMult 0.5`,
`bMaxSpeedRoot 4`, `bNullAdd 0.2`, `bNullMult 0.2`, `bMinAdd −1`, both weights 0.5) are
uncited choices made for this record. Every number below depends on them.

## Checks on the record itself

- The sweep's prototype arm at dt 0.1, seed 42 equals the Debug CLI's `--discharge` on the
  original project (external catalogs) in all 90 head/cycle rows: start, reason, mean
  headway and startup lost time. So catalog capture, the tool and Release vs Debug agree.
- The whole sweep run twice gives byte-identical per-cycle CSVs, SHA-256
  `14c4146d310d8b6e80814de926adf68b5da9b408d5b4f10dcbbdb2ecd7a2ba7e`.
- The per-cycle CSV (21,600 rows, 1.47 MB) is not committed; it regenerates from the
  tool and inputs. [`w74-discharge-runs.csv`](w74-discharge-runs.csv) commits one row per
  model, dt and seed (240 rows).

## Result — 40 seeds × 90 head/cycles per arm

| Model | dt (s) | Available headways / cycles | Mean headway mean / median / SD (s) | Startup lost time mean / median / SD (s) | Safety clamps per run, mean (min–max) |
|---|---|---|---|---|---|
| prototype | 0.1 | 1,201 / 3,600 | 2.045 / 2.033 / 0.055 | 0.108 / −0.100 / 0.667 | 5.50 (1–11) |
| prototype | 0.25 | 1,183 / 3,600 | 2.064 / 2.083 / 0.063 | 0.097 / −0.167 / 0.631 | 5.35 (1–12) |
| prototype | 0.5 | 1,169 / 3,600 | 2.121 / 2.167 / 0.084 | 0.054 / −0.333 / 0.731 | 5.93 (1–11) |
| w74 | 0.1 | 1,295 / 3,600 | 3.119 / 3.133 / 0.116 | −0.631 / −1.033 / 1.167 | 9.38 (3–16) |
| w74 | 0.25 | 1,333 / 3,600 | 3.165 / 3.167 / 0.121 | −0.525 / −1.000 / 1.269 | 10.43 (5–17) |
| w74 | 0.5 | 1,266 / 3,600 | 3.268 / 3.333 / 0.139 | −0.375 / −1.167 / 1.476 | 14.75 (8–21) |

Unavailable cycles are mostly `insufficient_crossings` (≈1,600–1,700 per arm) and
`queue_not_sustained` (≈380–460); 120 initial and ≈116 final partial cycles per arm are
excluded by the contract.

Paired per-seed difference of the seed's mean headway against dt 0.1 (n = 40):

| Model | dt 0.25 − 0.1 (s) mean ± SD [min, max] | dt 0.5 − 0.1 (s) mean ± SD [min, max] |
|---|---|---|
| prototype | +0.018 ± 0.013 [−0.008, +0.049] | +0.076 ± 0.020 [+0.031, +0.132] |
| w74 | +0.046 ± 0.019 [+0.001, +0.082] | +0.149 ± 0.031 [+0.084, +0.215] |

## Reading, and what it is not

- Both models' measured headway rises with dt; W74's rise is about twice the prototype's
  at both steps, and positive for every seed at dt 0.5. Part of any rise is the
  measurement's own one-timestep quantization (DISCHARGE.md), which both arms share.
- W74 with these values discharges at ≈3.1 s against the prototype's ≈2.0 s. That is a
  property of the fixture values and §4's emergency `dv²` term (a queued vehicle inside
  `ABX` starts only once `g > ABX`), not a claim about W74 or about real traffic.
- **W74 runs clamp more:** 9.4 per run at dt 0.1, rising to 14.8 at dt 0.5, against the
  prototype's 5.4–5.9 at every dt. Clamps are counted, never hidden (D105); this record
  does not trace them. Their cause is the next engine question for W74 on this network
  (NEXT), before any preset is shipped.
- Startup lost time is reported with its sign, as defined; negative values are the
  estimator's definition at these ranks, not an error.
- Nothing here bounds the sensitivity or calibrates anything; M0/M6 gates are unchanged.

## After D652 and D653 (2026-11-24)

The same tool, re-run on today's project:
- **Inputs:**
  - four-leg SHA-256 `fca6bbfa…96222`;
  - `default.json` `5c196dba…f65f`;
  - the same W74 fixture file.
- **Build:** Release, `-O3 -DNDEBUG -g1 -std=c++20 -fno-fast-math -ffp-contract=off`, GCC 13.3.0.
- **What changed since the table above:**
  - **D652:** every W74 behaviour, and the project's prototype, stops at the line.
  - **D653:** a front exactly at the line has not passed it.
- **Comparability.** The project now has 123 cycles per run, so the table above is not comparable row for row.
- **Runs:** [`w74-discharge-runs-d653.csv`](w74-discharge-runs-d653.csv).
- **Repeatability.** Two runs give byte-identical per-cycle CSVs. SHA-256:
  `31aeb91d6972be43cbc967a0061312aba8208dc01011c8c1a9faa9a814f18f00`.

| Model | dt (s) | Available headways / cycles | Mean headway mean / median / SD (s) | Startup lost time mean / median / SD (s) | Safety clamps per run, mean (min–max) |
|---|---|---|---|---|---|
| prototype | 0.1 | 1,323 / 4,920 | 2.113 / 2.100 / 0.047 | −0.602 / −0.600 / 0.128 | 0.00 (0–0) |
| prototype | 0.25 | 1,309 / 4,920 | 2.119 / 2.083 / 0.053 | −0.511 / −0.417 / 0.146 | 0.00 (0–0) |
| prototype | 0.5 | 1,265 / 4,920 | 2.114 / 2.167 / 0.080 | −0.250 / −0.333 / 0.208 | 0.00 (0–0) |
| w74 | 0.1 | 1,617 / 4,920 | 3.229 / 3.233 / 0.124 | −1.699 / −1.700 / 0.304 | 9.35 (3–16) |
| w74 | 0.25 | 1,629 / 4,920 | 3.265 / 3.250 / 0.113 | −1.630 / −1.667 / 0.284 | 10.47 (2–19) |
| w74 | 0.5 | 1,595 / 4,920 | 3.330 / 3.333 / 0.162 | −1.582 / −1.667 / 0.398 | 15.07 (10–23) |

**Paired per-seed difference** against dt 0.1:

| Model | dt 0.25 − 0.1 (s) | dt 0.5 − 0.1 (s) |
|---|---|---|
| prototype | +0.007 ± 0.010 [−0.011, +0.035], 33/40 positive | +0.002 ± 0.016 [−0.034, +0.033], 24/40 positive |
| w74 | +0.036 ± 0.024 [−0.028, +0.082], 37/40 positive | +0.100 ± 0.033 [+0.033, +0.179], 40/40 positive |

**The switch off** (a schema-87 copy with `lineRest: false`):
- **Engine unchanged.** With the old measurement it gives the D650-era per-cycle CSV byte for byte
  (`ed764e2a…72b2e`).
- **With D653's measurement:**

  | dt (s) | W74 headway (s) | W74 startup lost time (s) | Prototype startup lost time (s) |
  |---|---|---|---|
  | 0.1 | 3.132 | −1.117 | −0.108 |
  | 0.25 | 3.198 | −1.157 | −0.086 |
  | 0.5 | 3.307 | −1.246 | −0.153 |

- **Unchanged with the switch on:** W74's clamps are 9.4–15.1 per run either way, and its headway still rises
  with dt. Stopping at the line does not touch the emergency `dv²` start inside `ABX`.
- **Changed with the switch on:**
  - The prototype's headway is flat across dt (it was +0.075 s at dt 0.5 with the switch off).
  - W74's discharge is slower by about 0.1 s.
  - Startup lost time is more negative for both: a car at the line crosses one tick after Go, with no reaction
    time at green (D369).

Nothing here bounds or calibrates anything; the unvalidated marker stays.
