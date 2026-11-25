# The prototype behind a standing car: options measured — 2026-11-25

Evidence for [PROTOTYPE_QUEUE](../reference/PROTOTYPE_QUEUE.md) (D657). LINE_QUEUE (D655–D656) stopped W74 and W99 queues
at rest and left the prototype's crawl as its own question. Nothing here is validated, and the unvalidated marker
stays.

## Method

- **The patch.** A temporary patch on `src/core/simulation.cpp` ([patch](prototype-queue-options.patch), reverted)
  selects an option with the variable `ZZ_P`. It adds two traces: `ZZ_TRACE` for each clamp and `ZZ_HARD` for
  each moving tick below −3 m/s². Every branch requires `lineRest` and a prototype behaviour, so W74, W99 and bare
  M0 are untouched by construction.
- **The probe.** The D655 probe ([LINE_QUEUE evidence](line-queue-options.md#method)), not committed. It measures:
  - **crawl:** each vehicle's total seconds at 0.01–1 m/s;
  - **hard braking:** moving ticks below −3 m/s² per run, where moving means faster than 0.1 m/s the tick before;
  - **jerk:** ticks whose acceleration changed by more than 4 m/s² from the vehicle's previous tick, per run;
  - clamps, delay, and headway / startup lost time (`DischargeAccumulator`, CLI spec).

  A variable `ZZ_DT` sets the run's timestep.
- **Inputs.**
  - The six shipped projects of D655;
  - the project's prototype (`data/driver-behaviour/default.json`: standstill 2 m, additive 2, multiplicative 3,
    following time 1.5 s, `speedThreshold` 0.2 m/s, with `amberDeceleration`);
  - seeds 42–51, `lineRest` on, dt 0.1 unless stated.
- **Build:** Release, GCC 13.3.0, at `faae950`. `multra-cli 42` is byte-identical after the revert.

## Why the prototype crawls, and why an exact stop clamps

**The crawl.** The prototype's desired gap is `s0 + k·√v`, with `k = additive + multiplicative · driverFactor`
(about 5 m·(s/m)^½ by default).
- Near rest in its following mode, the law settles where `gap − s0 ≈ k·√v`, so `v ≈ ((gap − s0)/k)²`.
- Then `gap − s0 ≈ k²/t`: a hyperbolic approach that never reaches `s0`. With `k = 5`, a car is still 0.6 m short
  after 40 s and moving at about 0.014 m/s. D655 measured 0.013 m/s for vehicle 8 at 40 s.

**The clamps of Q2 for the prototype (P1).** The trace (`ZZ_TRACE`, four-leg, 3,032 clamps over ten seeds) finds
every one in the same situation:
- the follower is at 0.0002–0.09 m/s, in braking mode;
- its gap is within 3 mm of `s0`;
- its leader is moving at 0.00001–0.01 m/s, so it is not standing;
- the cap allows less than a millimetre.

Resting exactly at `s0` is a state the law cannot restart from. The leader edges off, and the follower's model
accelerates. At once `k·√v` exceeds the gap (√v has an unbounded slope at 0), so it brakes. It cannot shed
millimetres per second within micrometres, so it clamps, and the queue repeats this in micro-motion. Q0 never meets
the state, because its crawling follower is still about half a metre back.

## Options

| Option | What |
|---|---|
| P0 | Today (D656): the prototype keeps its law behind a standing car |
| P1 | LINE_QUEUE Q2 for the prototype: the stop law at the rear less `standstillDistance`, and the hold at the target |
| Q4 | D655's: P1 with D546's lever `standingTarget` |
| R5 | P1 plus D258's floor behind a slower vehicle (braking mode, leader slower than the follower): at least the deceleration that stops at `s0` |
| P2 | P1 plus a start rule: at rest behind a vehicle, no acceleration while the gap is below the model's desired gap at the first tick's speed `a·dt` |
| P2 + R5 | Both |
| P6 | P2, with the stop law only once the model, behind the standing car, is in its following or braking mode |
| P7 | P1 plus a start bound: the first tick's speed no more than the gap admits, `((gap − s0)/k)²` |
| P14 | P1, with a cap hit behind a slower leader finished by `stopAtLine` as braking (D583 widened) |
| P16 | P1 plus a start rule at the leader's speed: no acceleration while the gap is below the desired gap at the leader's speed |
| **P17** | P1 plus a start rule at the behaviour's `speedThreshold`: no acceleration while the gap is below the desired gap at `speedThreshold` |
| P18 | P17 plus P7's bound |

## Results (per run, seeds 42–51)

Crawl is the median / p90 per vehicle in seconds, then vehicles with 10 s or more.

### Four-leg, dt 0.1

| Option | Crawl med / p90 (s) | ≥ 10 s | Clamps | Hard braking | Jerk | Delay (s) | Headway / startup (s) |
|---|---|---|---|---|---|---|---|
| P0 | 17.0 / 53.0 | 2,814 / 5,047 | 0.0 | 417 | 23 | 47.53 | 2.112 / −0.600 |
| P1 | 2.7 / 3.3 | 13 | 303.2 | 1,207 | 762 | 47.63 | 2.126 / −0.642 |
| Q4 | 3.0 / 6.2 | 26 | 48.6 | 515 | 80 | 47.64 | 2.126 / −0.643 |
| R5 | 2.7 / 3.4 | 13 | 0.1 | 1,513 | 1,363 | 47.64 | 2.126 / −0.642 |
| P2 | 1.2 / 1.3 | 13 | 0.0 | 1,033 | 298 | 47.82 | 2.132 / −0.585 |
| P2 + R5 | 1.2 / 1.3 | 13 | 0.0 | 1,320 | 856 | 47.82 | 2.132 / −0.585 |
| P6 | 4.7 / 5.9 | 40 | 0.0 | 564 | 519 | 47.82 | 2.132 / −0.585 |
| P7 | 6.0 / 7.9 | 177 | 5.2 | 530 | 510 | 47.63 | 2.126 / −0.641 |
| P14 | 6.0 / 7.9 | 116 | 216.0 | 515 | 621 | 47.62 | 2.126 / −0.642 |
| P16 | 5.2 / 20.2 | 1,112 | 0.0 | 878 | 1,312 | 97.94 | 3.507 / −1.993 |
| **P17** | 4.7 / 6.0 | 40 | 0.0 | 556 | 516 | 47.72 | 2.128 / −0.594 |
| P18 | 4.7 / 6.0 | 41 | 0.0 | 555 | 515 | 47.72 | 2.129 / −0.595 |

### Four-leg, other timesteps

| dt | Option | Crawl med / p90 (s) | Clamps | Hard braking | Jerk | Delay (s) | Headway / startup (s) |
|---|---|---|---|---|---|---|---|
| 0.25 | P0 | 17.8 / 56.2 | 0.0 | 226 | 19 | 48.35 | 2.116 / −0.497 |
| 0.25 | P1 | 2.8 / 4.2 | 205.3 | 521 | 418 | 48.45 | 2.130 / −0.535 |
| 0.25 | P2 | 1.0 / 1.2 | 0.0 | 494 | 255 | 51.18 | 2.362 / −0.627 |
| 0.25 | **P17** | 2.5 / 3.8 | 0.0 | 317 | 267 | 48.61 | 2.138 / −0.530 |
| 0.25 | P18 | 2.8 / 3.8 | 0.0 | 321 | 271 | 48.82 | 2.166 / −0.566 |
| 0.5 | P0 | 26.0 / 46.0 | 0.0 | 187 | 190 | 48.44 | 2.115 / −0.250 |
| 0.5 | P1 | 2.5 / 4.5 | 36.4 | 310 | 233 | 48.49 | 2.127 / −0.276 |
| 0.5 | P2 | 0.5 / 2.0 | 0.1 | 357 | 206 | 59.52 | 2.832 / −1.159 |
| 0.5 | P14 | 4.5 / 7.0 | 29.7 | 208 | 208 | 48.49 | 2.124 / −0.270 |
| 0.5 | P16 | 3.5 / 14.0 | 5.4 | 391 | 669 | 106.31 | 3.762 / −2.061 |
| 0.5 | **P17** | 3.0 / 4.0 | 0.0 | 209 | 178 | 49.41 | 2.304 / −0.626 |
| 0.5 | P18 | 3.5 / 4.5 | 0.0 | 229 | 195 | 51.15 | 2.342 / −0.413 |

### M2.6

| dt | Option | Crawl med / p90 (s) | Clamps | Hard braking | Jerk | Delay (s) | Headway / startup (s) |
|---|---|---|---|---|---|---|---|
| 0.1 | P0 | 11.3 / 50.2 | 0.3 | 1,841 | 144 | 44.88 | 2.522 / −1.176 |
| 0.1 | P1 | 2.3 / 3.8 | 1,099.6 | 4,702 | 3,095 | 45.02 | 2.550 / −1.240 |
| 0.1 | P2 | 1.2 / 1.5 | 0.2 | 3,826 | 1,172 | 45.33 | 2.575 / −1.203 |
| 0.1 | P2 + R5 | 1.2 / 1.5 | 0.1 | 5,151 | 3,755 | 45.33 | 2.575 / −1.203 |
| 0.1 | P6 | 4.5 / 6.3 | 0.1 | 2,370 | 1,957 | 45.27 | 2.577 / −1.206 |
| 0.1 | P7 | 5.7 / 8.6 | 21.3 | 2,260 | 1,900 | 45.02 | 2.553 / −1.237 |
| 0.1 | **P17** | 4.6 / 6.3 | 0.2 | 2,368 | 1,951 | 45.29 | 2.581 / −1.220 |
| 0.5 | P0 | 23.0 / 45.0 | 0.3 | 720 | 723 | 43.83 | 2.517 / −0.790 |
| 0.5 | **P17** | 2.5 / 4.0 | 0.5 | 869 | 889 | 45.63 | 2.727 / −1.159 |

### Other projects, dt 0.1

Delay in seconds; clamps stay at 0–0.2 per run under P0, P1, P2, P6, P7 and P17.

| Project | P0 | P1 | P2 | P17 | Notes |
|---|---|---|---|---|---|
| T-junction | 5.94 | 5.90 | 6.58 | 6.39 | Longest crawl 41 s → 6–23 s |
| Priority lab | 5.16 | 5.14 | 5.28 | 5.30 | |
| Lane lab | 8.13 | 8.13 | 8.19 | 8.19 | R5 adds 2,478 hard-braking ticks per run |
| Weaving lab | 9.62 | 9.62 | 9.08 | 9.17 | R5 adds 4,978 hard-braking ticks per run |

### The hard braking under P2 (`ZZ_HARD`, four-leg)

The extra ticks are in the approaching mode, behind a moving leader, and at −3.0 to −3.4 m/s² mostly.

| | Per run | Below −4 | Below −5 | Below −6 |
|---|---|---|---|---|
| P0 | 417 | 45 | 17 | 13 |
| P2 | 1,033 | 96 | 28 | 21 |
| P6 | 564 | 124 | 78 | 66 |

- **The leader is braking under the stop law.** It brakes at a constant comfortable 2 m/s² and stops late, where
  the prototype's law braked earlier and more softly. So a follower arriving at 8–10 m/s closes faster.
- This is LINE_STOP's stop profile, so it is B's question (a softer, cited profile), not this contract's.

## Reading

- **No option keeps P0's figures and removes the crawl.**
- **P1 and Q4 clamp** (P1: 303 and 1,100 per run, four-leg and M2.6), as D655 found.
- **The start rules decide.** The clamps are a restart from an exact rest at `s0`. A vehicle that waits until its
  gap admits a speed has none.
  - P2 (`a·dt`) reads the timestep. At dt 0.5 it waits for the gap at 1.25 m/s, so headway is 2.832 s and delay is
    11 s higher. It is out.
  - P16 (the leader's speed) starts at once, then lags. Its delay doubles, so it is out.
  - P7 bounds the start instead of holding it. It crawls out again and clamps (5 and 21 per run).
  - **P17 (`speedThreshold`) reads no timestep.** It clamps 0–0.5 per run at every dt. Its crawl is 2.5–4.7 s at
    the median against 11–26 s.
- **Floors and finishes do not help.** R5 clamps nothing but brakes hard thousands of times on the lane and
  weaving labs. P14 still clamps 216 per run.
- **P17's costs:**
  - jerk ticks 23 → 516 per run on the four-leg, 144 → 1,951 on M2.6;
  - hard braking +33 % on the four-leg and +29 % on M2.6 at dt 0.1;
  - T-junction delay +0.45 s per trip;
  - headway at dt 0.5 +0.19–0.21 s (dt 0.1: +0.02–0.06 s).
- **P17 gives `speedThreshold` a second meaning.** Today it is the closing speed below which the prototype does
  not enter its approaching mode. Under P17 it is also the speed whose desired gap a vehicle at rest waits for. The
  value 0.2 m/s is the behaviour file's, uncited like every prototype value.
