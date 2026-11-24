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
