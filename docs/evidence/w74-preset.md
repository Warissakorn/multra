# M3.3.3a: a cited, uncalibrated `w74` preset and its discharge and clamps — 2026-10-15

NEXT item 1 (D176): a `w74` preset whose values cite a source, run through BA27's discharge
sweep and the [clamp trace](w74-clamps.md). **No engine change; nothing tuned or calibrated;
not validated.** The not-yet-validated marker stays. The values are SUMO's defaults written
in our formulation ([W74.md](../reference/W74.md)); SUMO does not validate them for this
network, and neither does this record.

## Source and value mapping

Eclipse SUMO, `src/microsim/cfmodels/MSCFModel_Wiedemann.cpp` (EPL-2.0 OR GPL-2.0-or-later;
the model W74.md §1 already names as the structure it follows), read on 2026-10-15 at `main`
= `79e42b50b221eeab8379b7e7513786a0c0bcdf4c` (`git ls-remote`). SUMO's two driver
parameters keep their defaults, `security = estimation = 0.5`. Where SUMO multiplies by the
vehicle's acceleration, its passenger default `accel = 2.6 m/s²` is used, so every value
traces to that one source. Preset: [`w74-sumo-defaults.json`](../../data/driver-behaviour/presets/w74-sumo-defaults.json).

| Key | Value | SUMO expression |
|---|---|---|
| `ax` | 2 | `myAX = length + 1 + 2·security` (gross distance) → 2 m net gap (W74.md §2) |
| `bxAdd`, `bxMult` | 4.5, 0 | `bx = (1 + 7·security)·√v`; no per-driver draw |
| `exAdd`, `exMult` | 1.5, 0 | `ex = 2 − estimation`; the random term is commented out |
| `cxAdd`, `cxMult` | 50, 0 | `myCX = 25·(1 + security + estimation)` |
| `opdvAdd`, `opdvMult` | 1, 2 | `opdv = cldv·(−1 − 2·N(0.5, 0.15))` |
| `dMax` | 150 | `D_MAX = 150` |
| `bMaxAdd`, `bMaxMult`, `bMaxSpeedRoot` | 0.2, 2.08, 7 | `bmax = 0.2 + 0.8·accel·(7 − √v)` |
| `bNullAdd`, `bNullMult` | 0.52, 0 | `myMinAccel = 0.2·accel` |
| `bMinAdd` | −1 | `B_MIN_ADD −1` (`B_MIN_MULT 0`) |
| `leaderAccelerationWeight` | 0.5 | `PRED_DECEL_MULT 0.5` |
| `emergencyLeaderWeight` | 0.55 | `PRED_DECEL_MULT_EMERGENCY 0.55` |

**Where our formulation differs from SUMO's** (recorded; W74.md stands, nothing is changed
to imitate SUMO): SUMO draws `opdv`'s normal on every call, ours is the per-vehicle trait
`zOp` with the same distribution; SUMO's emergency term uses gross `abx` against the net
`gap`, which adds the vehicle length to the deficit, ours is net throughout; SUMO brakes
above the desired speed, ours sets 0 and lets the desired-speed cap act; `bmax` and the
oscillation step scale with the vehicle's acceleration in SUMO but are behaviour keys here
(our `car` type's `maxAcceleration` 2.5 m/s² still caps); SUMO's approaching brake limit is
`myMaxApproachingDecel`, ours the type's `comfortableDeceleration`.

**Not in the source, so not in the preset:** `amberDeceleration` and
`maxDecelerationCooperativeBraking`. Without the first, amber holds like red (M4.2,
[AMBER](../reference/AMBER.md)); the prototype's `default.json` carries 3 m/s² since D147.
PTV's parameter page (`ax 2`, `bx_add 2`, `bx_mult 3`, used by the BA27 fixture) is not
mixed in: one coherent source, and values calibrated for Vissim do not transfer (rule 4).

## Inputs

- Commit `65c627a` plus this session's preset (no engine change). GCC 13.3.0, Linux, CMake
  Release `-O3 -DNDEBUG` with the project's `-std=c++20 -fno-fast-math -ffp-contract=off`
  (a separate `-DTRAFFICSIM_BUILD_DESKTOP=OFF` tree).
- Commands, each run with the preset and, for comparison on the same build, with BA27's
  fixture:
  `trafficsim-w74-discharge-sweep <behaviour> data/projects/four-leg-signalised.traffic.json data <out.csv>`
  and `trafficsim-w74-clamp-trace <behaviour> data/projects/four-leg-signalised.traffic.json data <out.jsonl>`.
  Prototype and `w74` × dt 0.1 / 0.25 / 0.5 × seeds 42–81.

| Input | SHA-256 |
|---|---|
| `data/driver-behaviour/presets/w74-sumo-defaults.json` | `c67bf20d93b1afe6897131a9f1241ca525a18a7d68c0fbb64dc56728f1dab859` |
| `docs/evidence/w74-discharge-behaviour.json` (fixture) | `fdc205f8b4b9adaa7fd10d53b21f41eab574ee0ea45ba5bb5e87dede5f64c63c` |
| `data/driver-behaviour/default.json` | `5c196dbad0fcead42ef31009d201430cf3c3314b006fecffb59bc323eff3f65f` |
| Project, vehicle types, queue counter, compositions | as in [BA27](w74-discharge.md#inputs) (unchanged) |

| Output (not committed; the tools regenerate them) | SHA-256 |
|---|---|
| Preset sweep, 21,600 cycle rows | `abd25a151d64127603f8e3c3b24ba4756b136768de865fbf1ccbbe0b0a893378` |
| Preset trace, 21,350 lines (470 clamps, 20,880 onsets) | `6621962a6f232b7ae1f51fadda172e7f9f77a893c83685b6929a1b02ca9983b8` |
| Fixture sweep (rerun) | `42e033653d0fe7550eb859384c9488ab5ad63c3e8cfd67422b77f27cbd173e11` |
| Fixture trace (rerun), 22,198 lines (1,318 clamps) | `9fa2de9c77cb92f9700c96f6ab652445394539cac06de61c93d22bf6e0a3e6af` |

[`w74-preset-runs.csv`](w74-preset-runs.csv) commits one row per model, dt and seed (240 rows,
BA27's columns).

## Checks on the record itself

- A second run of the preset sweep and trace is byte-identical.
- The prototype arm is identical in the preset and fixture sweeps (it never reads the file).
- Every clamp in both traces is attributed (amber head, red head, vehicle leader or dead end):
  **none is `phase-2` or `unexplained`.**
- **The fixture no longer reproduces BA27's hashes** (sweep `14c4146d…` → `42e03365…`; trace
  22,933 → 22,198 lines). The engine changed after 2026-10-08: D159's seed mapping and
  M4.2's amber rule (the prototype arm now has no amber clamps, since `default.json` carries
  `amberDeceleration`). BA27's tables describe their own date; the comparison below is
  preset against fixture on one build.

## Result — 40 seeds × 90 head/cycles per arm

| Arm | dt (s) | Available headways / cycles | Mean headway mean / median / SD (s) | Startup lost time mean / median / SD (s) | Safety clamps per run, mean (min–max) |
|---|---|---|---|---|---|
| prototype | 0.1 | 1,130 / 3,600 | 2.052 / 2.067 / 0.047 | −0.112 / −0.100 / 0.129 | 0.00 (0–0) |
| prototype | 0.25 | 1,151 / 3,600 | 2.070 / 2.083 / 0.056 | −0.088 / −0.167 / 0.188 | 0.05 (0–1) |
| prototype | 0.5 | 1,120 / 3,600 | 2.130 / 2.167 / 0.077 | −0.161 / −0.333 / 0.344 | 0.20 (0–2) |
| w74 fixture | 0.1 | 1,251 / 3,600 | 3.111 / 3.100 / 0.119 | −0.654 / −1.033 / 1.130 | 8.22 (3–15) |
| w74 fixture | 0.25 | 1,292 / 3,600 | 3.167 / 3.167 / 0.123 | −0.570 / −1.000 / 1.234 | 10.25 (5–17) |
| w74 fixture | 0.5 | 1,236 / 3,600 | 3.259 / 3.333 / 0.137 | −0.361 / −0.833 / 1.473 | 14.22 (6–22) |
| **w74 preset** | 0.1 | 1,221 / 3,600 | 3.437 / 3.433 / 0.042 | −1.372 / −1.367 / 0.577 | 3.20 (0–7) |
| **w74 preset** | 0.25 | 1,250 / 3,600 | 3.496 / 3.500 / 0.055 | −1.328 / −1.250 / 0.717 | 3.65 (0–8) |
| **w74 preset** | 0.5 | 1,199 / 3,600 | 3.527 / 3.500 / 0.077 | −1.175 / −1.000 / 0.738 | 4.65 (1–9) |

Clamps by obstacle (40 runs each; prototype rows are the same in both traces):

| Arm, dt 0.1 / 0.25 / 0.5 | Amber head | Vehicle leader | Red head | Dead end |
|---|---|---|---|---|
| prototype | 0 / 0 / 0 | 0 / 0 / 4 | 0 / 2 / 3 | 0 / 0 / 1 |
| w74 fixture | 314 / 381 / 486 | 15 / 28 / 83 | 0 / 0 / 0 | 0 / 1 / 0 |
| w74 preset | 125 / 125 / 134 | 3 / 21 / 52 | 0 / 0 / 0 | 0 / 0 / 0 |

| Amber onsets, 3,480 per arm, dt 0.1 / 0.25 / 0.5 | Vehicles upstream (mean) | Queued (mean) | Onsets with a queue | "Cannot stop" onsets |
|---|---|---|---|---|
| prototype | 0.60 / 0.60 / 0.61 | 0.00 / 0.00 / 0.00 | 0 / 3 / 0 | 195 / 217 / 300 |
| w74 fixture | 1.77 / 1.86 / 2.10 | 0.67 / 0.73 / 0.93 | 516 / 540 / 680 | 340 / 452 / 664 |
| w74 preset | 2.09 / 2.14 / 2.22 | 0.67 / 0.72 / 0.86 | 536 / 556 / 668 | 144 / 159 / 203 |

## Reading, and what it is not

- With SUMO's defaults our W74 discharges more slowly than with the fixture (≈3.44 s against
  ≈3.11 s at dt 0.1) and much more slowly than the prototype (≈2.05 s). SUMO's `BX` is
  4.5·√v against the fixture's mean 3.5·√v, so `ABX` (the gap a queued vehicle must exceed
  before it starts, W74.md §4) is larger. This is a property of the values in our
  formulation, not a statement about SUMO, Vissim or real saturation flow.
- The preset's headway is less sensitive to dt (arm means +0.06 / +0.09 s at dt 0.25 / 0.5 over dt 0.1,
  against the fixture's +0.06 / +0.15 s) and spreads less across cycles (SD 0.04–0.08 s).
- **The preset clamps less:** 3.2–4.7 per run against the fixture's 8.2–14.2. The amber
  clamps (98 % / 86 % / 72 % of its clamps) come from `w74` behaviours holding amber
  as red, which neither file overrides. The cause follows [the clamp trace](w74-clamps.md):
  queues at amber are as common as with the fixture, but far fewer onsets have a vehicle
  that cannot stop (144 against 340 at dt 0.1), consistent with SUMO's larger following gaps. That reading rests on these counts, not on a
  controlled experiment.
- No engine defect is suggested: every clamp is one of the two conservative rules the clamp
  trace already identified (amber held as red with no commitment test; the standstill cap
  treating a moving leader as standing).
- Nothing here bounds the sensitivity, calibrates or validates anything; M0/M6 gates are
  unchanged. A preset in the auto-loaded catalog, or an `amberDeceleration` for it, would be
  an owner decision (OWNER_SITTING B13).
