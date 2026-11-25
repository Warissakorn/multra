# M3.3.3b BA231: W99 queue-discharge timestep sensitivity — 2026-11-24

Row BA231 of [the delivery plan](../plans/DRIVING_BEHAVIOUR.md#3-failure-first-acceptance-rows):
queue discharge with a `w99` behaviour at dt 0.1 / 0.25 / 0.5, seeds 42–81, **sensitivity
recorded, no bound, no calibration or validation inference** (rule 4, [W99.md](../reference/W99.md) §8).
The not-yet-validated marker stays. The prototype arm is the same measurement for comparison. It is
the W74 record [BA27](w74-discharge.md) repeated for W99 (D649).

## Inputs

- Commit `d338d8c` plus this session's tools (no engine change):
  - `tools/discharge_sweep.hpp`, the BA27 loop moved out of `tools/w74_discharge_sweep.cpp`;
  - `tools/w99_discharge_sweep.cpp`.
- Build: GCC 13.3.0, Linux, CMake Release with `-O3 -DNDEBUG -g1 -std=c++20 -fno-fast-math -ffp-contract=off`.
  The tree is `-DMULTRA_BUILD_DESKTOP=OFF`.
- Command: `multra-w99-discharge-sweep docs/evidence/w99-discharge-behaviour.json
  data/projects/four-leg-signalised.traffic.json data <out.csv>`.
- What the tool does, as BA27's did:
  - captures the project's catalogs as owned;
  - for the `w99` arm, adds the behaviour file and points every vehicle type at it, heavy vehicles included;
  - `changeRunSettings` sets dt, and the run lasts 900 s;
  - the discharge spec is the CLI default: the whole run, ranks 3–5 steady, ranks 1–2 startup.

| Input | SHA-256 |
|---|---|
| `data/projects/four-leg-signalised.traffic.json` | `fca6bbfa1e541918319f3c7a6555d33830cba182387e019ff4c104a7b0e96222` |
| `docs/evidence/w99-discharge-behaviour.json` | `925aca50fc24cf4f00627c389e4ddb32191cfbeadb4c34237b54f4749505dc16` |
| `data/vehicle-types/car.json` | `c8d99f76ed01dec7d91866d239b40ed1e682a1f4358a9c8ca084fc4dec8d87e9` |
| `data/vehicle-types/heavy-vehicle.json` | `158e88a84ef482b054c727900a5c91d41732dae639c5804667f8fe6d73ddaca8` |
| `data/driver-behaviour/default.json` | `5c196dbad0fcead42ef31009d201430cf3c3314b006fecffb59bc323eff3f65f` |
| `data/evaluation/queue-counter.json` | `04bff220abb06e80db644e1b94afb53f3e0af790ffe2371e06eff5f2b82a1de9` |
| `data/compositions/car-only.json`, `urban-mixed.json` | `01e212b1…702dd`, `13618f4f…69531` |

**The W99 parameter values are the contract's fixture values, not a preset and not calibrated.**
They are `W99_ORIGINAL` from W99 §5: cc0 1.5 m, cc1 1.3 s (fixed), cc2 4 m, cc3 −12 s, cc4 −0.25 m/s,
cc5 0.35 m/s, cc6 6, cc7 0.25 m/s², cc8 2 m/s², cc9 1.5 m/s². Every number below depends on them. A CC1
curve (§5a) is **not** recorded here.

## Checks on the record itself

- **Refactor guard.** `multra-w74-discharge-sweep`, built before and after the loop moved, gives
  byte-identical per-cycle CSVs on BA27's inputs. SHA-256:
  `ed764e2a6b47957df5032abfecef7c40cff9d9b6d5949487471447d84bc72b2e`.
- **CLI agreement.** The sweep's prototype arm at dt 0.1, seed 42 equals `multra-cli --project … --seed 42
  --discharge` on the original project (external catalogs) in all 123 head/cycle rows. The rows agree on
  head, lane, go, reason, samples, mean headway and startup lost time.
- **Repeatability.** The whole sweep, run twice, gives byte-identical per-cycle CSVs. SHA-256:
  `a7d1408747aa5a25450ead725e9094a26d86823cb7c94c00f4315a72ae601ea1`.
- **What is committed.**
  - The per-cycle CSV (29,520 rows, 2.0 MB) is not committed; it regenerates from the tool and its inputs.
  - [`w99-discharge-runs.csv`](w99-discharge-runs.csv) has one row per model, dt and seed (240 rows), in the
    columns of BA27's runs file. A run's `meanOfMeanHeadway` and `meanStartupLostTime` are the plain means
    over that run's cycles that have a value.

## Result — 40 seeds × 123 head/cycles per arm

| Model | dt (s) | Available headways / cycles | Mean headway mean / median / SD (s) | Startup lost time mean / median / SD (s) | Safety clamps per run, mean (min–max) |
|---|---|---|---|---|---|
| prototype | 0.1 | 1,316 / 4,920 | 2.052 / 2.067 / 0.048 | −0.108 / −0.100 / 0.131 | 0.00 (0–0) |
| prototype | 0.25 | 1,296 / 4,920 | 2.070 / 2.083 / 0.057 | −0.076 / −0.167 / 0.221 | 0.00 (0–0) |
| prototype | 0.5 | 1,262 / 4,920 | 2.127 / 2.167 / 0.078 | −0.142 / −0.333 / 0.368 | 0.12 (0–2) |
| w99 | 0.1 | 1,341 / 4,920 | 1.959 / 1.967 / 0.059 | −0.116 / −0.267 / 0.577 | 7,870.18 (3,613–30,171) |
| w99 | 0.25 | 1,336 / 4,920 | 1.942 / 1.917 / 0.064 | 0.055 / −0.083 / 0.585 | 3,056.38 (1,485–6,626) |
| w99 | 0.5 | 1,283 / 4,920 | 1.901 / 1.833 / 0.085 | 0.341 / 0.333 / 0.499 | 1,512.15 (701–3,023) |

**Unavailable cycles in every arm:**

| Reason | Count per arm |
|---|---|
| `insufficient_crossings` | ≈2,840–2,900 |
| `queue_not_sustained` | ≈455–510 |
| `route_or_lane_change` | 12–43 |

The contract excludes the 120 initial and the 120 final partial cycles in each arm. The project has
changed since BA27 (123 cycles per run, not 90, and no prototype clamps at dt 0.1), so its prototype
numbers are not comparable row for row with the W74 record.

**Paired per-seed difference** of each seed's mean headway against dt 0.1 (n = 40):

| Model | dt 0.25 − 0.1 (s) mean ± SD [min, max] | dt 0.5 − 0.1 (s) mean ± SD [min, max] |
|---|---|---|
| prototype | +0.018 ± 0.013 [−0.009, +0.052], 38/40 positive | +0.075 ± 0.019 [+0.033, +0.110], 40/40 positive |
| w99 | −0.017 ± 0.012 [−0.039, +0.012], 3/40 positive | −0.058 ± 0.016 [−0.098, −0.030], 0/40 positive |

## The clamps: standing vehicles, every tick

The clamp counts are not collisions avoided at speed. A scratch probe, not committed, used seed 42 and
counted each `SafetyClampEvent` by the vehicle's speed before the tick.

**Standing vehicles carry almost all clamps:**

| dt (s) | Clamps on standing vehicles | Clamps in total (of these runs) | Vehicles clamped |
|---|---|---|---|
| 0.1 | 7,309 | 7,313 | 323 |
| 0.25 | 2,585 | 2,601 | 321 |
| 0.5 | 1,667 | 1,712 | 314 |

The count falls with dt because the cause repeats once per tick.

**The mechanism is the following-regime standstill creep (W99 §4) against the hard cap:**
1. A queued car stops behind its standing leader at a gap of cc0 (1.5 m) plus rounding: 3·10⁻⁹ m at seed 42,
   t = 95.4 s.
2. At rest `SDXc = cc0`. A gap above it by any amount is *following*, so the creep asks for +cc7.
3. D105 holds a stopped vehicle only when `g ≤ cc0`, which this gap misses by 3·10⁻⁹ m.
4. The phase-1 leader cap allows the tiny room and no more, so the move is clamped.
5. The leader creeps the same way, so the room grows: 3·10⁻⁹, then 8·10⁻⁸, then 8·10⁻⁷ m on successive
   ticks.
6. Every standing car in the queue is clamped each tick until the queue moves.

The prototype has no creep, and W74's creep sits in a wider band, so neither shows this.

## Reading, and what it is not

- **Headway falls with dt.** W99's measured headway falls as dt grows, for every seed at dt 0.5. The
  prototype's rises, as W74's did in BA27.
  - Part of any change is the measurement's own one-tick quantization (DISCHARGE.md), which both arms
    share.
  - W99's opposite sign is recorded, not explained. Candidates: following holds ±CC7 or `aPrev` for a whole
    tick (§8), and the creep above.
- **These values against the prototype.** With these fixture values W99 discharges at ≈1.96 s against the
  prototype's ≈2.05 s at dt 0.1. That is a property of `W99_ORIGINAL`, not a claim about W99 or about real
  traffic.
- **Startup lost time** is reported with its sign, as defined. Negative values are the estimator's
  definition at these ranks, not an error.
- **The clamps are counted, never hidden (D105).** The creep at `g` just above cc0 is a contract question
  for W99 §4, the next engine question before any preset. This record does not change it.
- Nothing here bounds the sensitivity or calibrates anything. The M0/M6 gates are unchanged.

## After D650

D650 holds a stopped W99 driver while one creep would not fit before CC0 (`g − CC0 ≤ CC7 · dt²`, W99 §4,
row BA238). The same sweep, the same inputs and build flags, re-run at that commit. The table above stays
as D649 recorded it. Runs: [`w99-discharge-runs-d650.csv`](w99-discharge-runs-d650.csv) (same columns).

- **Repeatability.** Two runs give byte-identical per-cycle CSVs. SHA-256:
  `4cf7b07f8dfbc22b3d6d9da5f5f7227ce1a0d7f3c05f02f16412c364be18fcb8`.
- **Other arms unchanged.** The prototype arm is unchanged, and the W74 sweep on BA27's inputs is
  byte-identical (`ed764e2a…72b2e`).

| Model | dt (s) | Mean headway mean / median / SD (s) | Startup lost time mean (s) | Safety clamps per run, mean (min–max) | D649's clamps |
|---|---|---|---|---|---|
| w99 | 0.1 | 1.959 / 1.967 / 0.059 | −0.116 | 330.73 (256–434) | 7,870.18 |
| w99 | 0.25 | 1.942 / 1.917 / 0.064 | 0.055 | 324.45 (242–382) | 3,056.38 |
| w99 | 0.5 | 1.901 / 1.833 / 0.085 | 0.342 | 329.60 (257–385) | 1,512.15 |

- **Headway is unchanged** to three decimals, with the same paired differences against dt 0.1. Startup lost
  time moves by at most 0.001 s, at dt 0.5. The hold changes where a standing car rests, by at most `CC7 · dt²`,
  not when the queue moves.
- **What the remaining clamps are.** The seed-42 probe now finds each clamped car clamped once:

  | dt (s) | Cars clamped once | Cars clamped more than once |
  |---|---|---|
  | 0.1 | 322 | 1 |
  | 0.25 | 317 | 4 |
  | 0.5 | 307 | 4 |

  Almost all are below 1 m/s before the tick: a car arriving at its line or at CC0 while still creeping.
  BA238's queued-car case shows one at 0.03 m/s with 0.9 mm left, after *closing* braked at only
  −0.005 m/s². That is the soft approach to a standing obstacle, not the hold. It is left for a
  stop-at-the-line contract covering every line and model (NEXT).
- Nothing is bounded or calibrated, and the unvalidated marker stays.

## After D652 and D653

D652 stops every W99 and project prototype at the line (LINE_STOP L3F). D653 corrects the measurement: a front
exactly at the line has not passed it (`src/eval/line_passage.hpp`). The same sweep, inputs and build flags are
re-run at that commit. The tables above stay as D649 and D650 recorded them. Runs:
[`w99-discharge-runs-d653.csv`](w99-discharge-runs-d653.csv) (same columns).

- **Repeatability.** Two runs give byte-identical per-cycle CSVs. SHA-256:
  `54cb2243901c959206c07382cf437d0b84f36039c503274de69fadc279ac2f84`.
- **Why D653 was needed.** Measured with the old rule, D652's queue head, standing exactly at the line, was
  counted as having crossed while the light was still red. It never appeared at Go. The ranks shifted by one car,
  available cycles fell (prototype 1,316 → 998), and startup lost time read about +1.6 to +2.0 s. That number
  was an artefact.

| Model | dt (s) | Available headways / cycles | Mean headway mean / median / SD (s) | Startup lost time mean / median / SD (s) | Safety clamps per run, mean (min–max) |
|---|---|---|---|---|---|
| prototype | 0.1 | 1,323 / 4,920 | 2.113 / 2.100 / 0.047 | −0.602 / −0.600 / 0.128 | 0.00 (0–0) |
| prototype | 0.25 | 1,309 / 4,920 | 2.119 / 2.083 / 0.053 | −0.511 / −0.417 / 0.146 | 0.00 (0–0) |
| prototype | 0.5 | 1,265 / 4,920 | 2.114 / 2.167 / 0.080 | −0.250 / −0.333 / 0.208 | 0.00 (0–0) |
| w99 | 0.1 | 1,342 / 4,920 | 2.002 / 2.000 / 0.050 | −0.650 / −0.633 / 0.106 | 344.82 (253–471) |
| w99 | 0.25 | 1,325 / 4,920 | 1.976 / 2.000 / 0.059 | −0.447 / −0.500 / 0.120 | 222.28 (154–274) |
| w99 | 0.5 | 1,300 / 4,920 | 2.016 / 2.000 / 0.051 | −0.522 / −0.500 / 0.132 | 145.05 (105–192) |

**Paired per-seed difference** against dt 0.1 (n = 40):

| Model | dt 0.25 − 0.1 (s) | dt 0.5 − 0.1 (s) |
|---|---|---|
| prototype | +0.007 ± 0.010 [−0.011, +0.035], 33/40 positive | +0.002 ± 0.016 [−0.034, +0.033], 24/40 positive |
| w99 | −0.026 ± 0.011 [−0.049, −0.002], 0/40 positive | +0.014 ± 0.013 [−0.015, +0.040], 34/40 positive |

**The switch off, measured with D653** (a project copy at schema 87 with `clampRules.lineRest: false`):

- **The engine is unchanged with the switch off.** The old rule on this copy gives D650's per-cycle CSV byte for
  byte (`4cf7b07f…`).
- **D653 alone moves cycles where a car happened to stand exactly at the line,** having been clamped there or
  having reached it in its last tick.
  - W99 at dt 0.1: startup lost time −0.116 ± 0.577 → −0.329 ± 0.148 s. Its wide spread was largely this
    artefact.
  - Headway: 1.966 / 1.948 / 1.910 s.
  - The paired dt 0.5 difference is still −0.056 s, 0/40 seeds positive.

**Reading:**
- **W99's headway no longer falls with dt once it stops at the line.** At dt 0.5 the paired difference is
  +0.014 s, against −0.056 s with the switch off. D649's candidate cause, the creep toward the line, is the one
  the switch removes. The prototype's headway is flat across dt as well, against +0.075 s with the switch off.
- **Startup lost time stays negative,** about −0.5 to −0.65 s, under the corrected measurement. A car at the line
  crosses one tick after Go, and no model has a reaction time at green (D369's reading). HCM's default 2.0 s is
  not approached by stopping at the line. A start-up reaction is the place for it, not the line.
- **The clamps are W99's arrival clamps** (LINE_STOP §6), about 345 per run at dt 0.1. Their fix is a W99 §4
  slice.
- Nothing is bounded or calibrated, and the unvalidated marker stays.

## After D654

D654 caps a moving W99 driver behind a standing obstacle at the exact stop at CC0 (W99 §4, rows BA249–BA251).
The same sweep, inputs and build flags are re-run at that commit. Runs:
[`w99-discharge-runs-d654.csv`](w99-discharge-runs-d654.csv) (switch on).

- **Repeatability.** Two runs give byte-identical per-cycle CSVs.
  - Switch on: `b274704bafbd97f3d2366b508a727954e951703e69254d017a7bada579002349`.
  - Switch off: `a7d313cf7b4c4a26c23df40b706343b6322676da3028b64dea3c6f37b2bb3a70`.
- **Other arms unchanged.**
  - The prototype arm's rows equal D653's.
  - The W74 sweep is byte-identical, on (`31aeb91d…`) and off (`cf733d09…`).
  - `multra-cli 42` is byte-identical.

| Switch | dt (s) | W99 headway mean / median / SD (s) | Startup lost time mean (s) | Clamps per run, mean (min–max) | D653's clamps |
|---|---|---|---|---|---|
| on | 0.1 | 2.002 / 2.000 / 0.050 | −0.650 | 41.83 (12–90) | 344.82 |
| on | 0.25 | 1.976 / 2.000 / 0.059 | −0.447 | 25.23 (10–40) | 222.28 |
| on | 0.5 | 2.016 / 2.000 / 0.051 | −0.522 | 31.57 (17–47) | 145.05 |
| off | 0.1 | 1.967 / 1.967 / 0.053 | −0.321 | 21.52 (3–90) | 330.73 |
| off | 0.25 | 1.950 / 1.917 / 0.060 | −0.169 | 17.82 (7–34) | 324.45 |
| off | 0.5 | 1.914 / 1.833 / 0.088 | +0.114 | 26.30 (11–40) | 329.60 |

- **Headway and startup lost time** are unchanged to three decimals with the switch on, and within 0.008 s with
  it off. The cap binds only in the last centimetres of an approach.
- **The four-leg and M2.6** (the LINE_STOP probe, seeds 42–51), W99 clamps per run:

  | Project | Switch on | Switch off |
  |---|---|---|
  | Four-leg | 337.5 → 39.2 | 322.6 → 15.9 |
  | M2.6 | 1,258.9 → 150.2 | 1,205.9 → 73.5 |

  - Delay is unchanged except on M2.6 with the switch off: 48.24 → 50.13 s.
  - The prototype and both W74 arms are unchanged.
- **What remains** (a seed-42 probe on the four-leg, 48 clamps, by the vehicle's state before the tick):
  - **Moving leaders:** 22 are behind a leader still moving at 0.0001–0.06 m/s, which then stops. The leader
    is itself creeping.
  - **Unidentified leaders:** about 25 are behind a leader on another route or behind a hold the probe could
    not see.
  - **The gap is not covered.** Capping behind a moving leader needs that leader's braking, which `Leader`
    does not carry. It is left open, not guessed.
- Nothing is bounded or calibrated, and the unvalidated marker stays.

## After D656

D656 (LINE_QUEUE Q2): under `lineRest`, a W99 vehicle behind a vehicle at rest stops with LINE_STOP's law at
its rear less the standstill gap. The same sweep is re-run. Runs:
[`w99-discharge-runs-d656.csv`](w99-discharge-runs-d656.csv).

| Model | dt (s) | Mean headway mean / median / SD (s) | Startup lost time mean (s) | Clamps per run, mean (min–max) | D654's clamps |
|---|---|---|---|---|---|
| w99 | 0.1 | 2.003 / 2.000 / 0.050 | −0.651 | 7.97 (1–14) | 41.83 |
| w99 | 0.25 | 1.976 / 2.000 / 0.060 | −0.448 | 8.18 (1–15) | 25.23 |
| w99 | 0.5 | 2.016 / 2.000 / 0.052 | −0.522 | 8.00 (2–19) | 31.57 |

- **Available headways** rise from 1,342 to 1,416 at dt 0.1: queued W99 cars now stand at rest at CC0.
- **Paired dt differences** are as D653's: −0.026 and +0.014 s.
- **Repeatability.** Two runs give byte-identical CSVs. SHA-256:
  `1ab4a0536c19d7bb2f32f56a5629528242227510345a137268fe2bd80a99e017`.
- **The prototype arm** is unchanged.
