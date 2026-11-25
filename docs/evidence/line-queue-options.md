# Queues behind a standing car: options measured — 2026-11-25

Evidence for [LINE_QUEUE](../reference/LINE_QUEUE.md) (D655). The owner's F9 look on package #149 found followers
crawling without end behind a standing car (LINE_STOP §9). Nothing here is validated, and the unvalidated
marker stays.

## Method

- **The patch.** A temporary patch on `src/core/simulation.cpp` ([patch](line-queue-options.patch), reverted)
  selects an option with the variable `ZZ_Q`. Every branch requires `lineRest`, so bare M0 is untouched by
  construction.
- **The probe.** A scratch probe, not committed. It is LINE_STOP's D651 probe with three measures added:
  - **crawl:** each vehicle's total seconds at 0.01–1 m/s;
  - **hard braking:** ticks below −3 m/s² per run;
  - **jerk:** ticks whose acceleration changed by more than 4 m/s² from the vehicle's previous tick, per run.

  Plus clamps and delay, and headway / startup lost time (`DischargeAccumulator`, CLI spec) where queue counters
  exist.
- **Inputs.**
  - Six shipped projects;
  - the prototype (the project's, with `amberDeceleration`), the W74 fixture, the W74 SUMO preset and the W99
    fixture;
  - seeds 42–51, dt 0.1, `lineRest` on.
- **Build:** Release, GCC 13.3.0, at `fe8a37b`.

## Options

| Option | What |
|---|---|
| Q0 | Today (D652–D654) |
| Q1 | Behind a vehicle at exactly 0 m/s, the model sees no leader and the stop law (LINE_STOP §2) stops at its rear less the standstill gap; plus a safe-speed bound behind moving leaders (stop at the maximum short of where the leader would stand braking at the maximum). LINE_STOP §9's first attempt |
| Q2 | Q1's standing-vehicle law without the safe-speed bound, plus a hold: at rest within 10⁻⁶ m of the target behind a standing vehicle, no acceleration |
| Q3 | Keep the model; below 0.1 m/s behind a leader below 0.1 m/s, the stop law finishes the stop |
| Q4 | Q2 for W74 and W99 only; the prototype gets D546's lever `standingTarget` (the standstill gap alone behind a leader below 0.1 m/s) |
| Q5 | Q2 with "standing" as below 0.1 m/s (`kStoppedSpeed`), the target moved by the leader's comfortable stopping distance |

## Results (per run, seeds 42–51)

Crawl is the median / p90 per vehicle in seconds, then vehicles with 10 s or more.

### Four-leg

| Model | Option | Crawl med / p90 (s) | ≥ 10 s | Clamps | Hard braking | Jerk | Delay (s) | Headway / startup (s) |
|---|---|---|---|---|---|---|---|---|
| prototype | Q0 | 17.0 / 53.0 | 2,814 / 5,047 | 0.0 | 417 | 23 | 47.53 | 2.112 / −0.600 |
| prototype | Q1 | 2.7 / 3.4 | 13 | 434.7 | 1,210 | 795 | 47.63 | 2.126 / −0.642 |
| prototype | Q2 | 2.7 / 3.3 | 13 | 303.2 | 1,207 | 762 | 47.63 | 2.126 / −0.642 |
| prototype | Q3 | 12.9 / 23.8 | 2,806 | 757.4 | 783 | 1,115 | 47.57 | 2.125 / −0.640 |
| prototype | Q4 | 3.0 / 6.2 | 26 | 48.6 | 515 | 80 | 47.64 | 2.126 / −0.643 |
| prototype | Q5 | 3.2 / 8.8 | 336 | 295.4 | 974 | 290 | 47.66 | 2.126 / −0.642 |
| W74 fixture | Q0 | 30.2 / 106.2 | 3,680 | 8.9 | 486 | 181 | 95.01 | 3.235 / −1.714 |
| W74 fixture | Q2 (= Q4) | 3.1 / 10.6 | 538 | 9.2 | 416 | 625 | 95.15 | 3.256 / −1.776 |
| W74 fixture | Q5 | 4.0 / 18.2 | 1,018 | 102.4 | 416 | 610 | 95.11 | 3.257 / −1.777 |
| W74 SUMO preset | Q0 | 36.3 / 168.9 | 3,788 | 3.1 | 509 | 166 | 113.91 | 3.566 / −2.144 |
| W74 SUMO preset | Q2 (= Q4) | 3.1 / 9.8 | 481 | 2.3 | 498 | 686 | 111.29 | 3.567 / −2.133 |
| W99 | Q0 | 2.6 / 3.8 | 55 | 39.2 | 189 | 83 | 50.65 | 2.004 / −0.652 |
| W99 | Q2 (= Q4) | 2.1 / 2.5 | 1 | 7.9 | 149 | 266 | 50.57 | 2.004 / −0.652 |
| W99 | Q3 | 2.9 / 4.6 | 11 | 925.5 | 256 | 181 | 50.57 | 2.002 / −0.649 |
| W99 | Q5 | 2.8 / 8.0 | 203 | 562.8 | 168 | 322 | 50.50 | 2.001 / −0.647 |

### M2.6

| Model | Option | Crawl med / p90 (s) | Clamps | Hard braking | Jerk | Delay (s) |
|---|---|---|---|---|---|---|
| prototype | Q0 | 11.3 / 50.2 | 0.3 | 1,841 | 144 | 44.88 |
| prototype | Q2 | 2.3 / 3.8 | 1,099.6 | 4,702 | 3,095 | 45.02 |
| prototype | Q4 | 2.7 / 6.4 | 208.0 | 2,025 | 354 | 45.01 |
| W74 fixture | Q0 | 37.0 / 203.2 | 27.4 | 2,386 | 766 | 131.93 |
| W74 fixture | Q2 | 3.6 / 16.6 | 29.5 | 12,335 | 2,842 | 122.55 |
| W74 SUMO preset | Q0 | 47.5 / 302.6 | 20.9 | 2,905 | 790 | 171.48 |
| W74 SUMO preset | Q2 | 3.4 / 15.7 | 14.3 | 41,925 | 2,256 | 130.21 |
| W99 | Q0 | 2.6 / 4.1 | 150.2 | 853 | 425 | 47.82 |
| W99 | Q2 | 2.1 / 3.0 | 32.9 | 647 | 1,102 | 47.84 |

### Other projects

- **T-junction and priority lab.**
  - Q2 shortens the longest crawls (W74 88–102 s → 15–16 s).
  - Clamps stay at 0–0.2 per run. Delay is lower by 0–0.45 s.
- **Lane lab:** unchanged under every option.
- **Weaving lab.**
  - **W99 under Q0 clamps 2,987 per run.** Q2 takes it to 30.7. Delay is 18.90 → 18.68 s.
  - The prototype and W74 are unchanged.

## Reading

- **Q2 removes the crawl for W74 and W99 and is clean for them.**
  - Clamps: W74 equal; W99 −80 % to −99 %.
  - Delay: equal or lower, and M2.6's W74 delay is down 9–41 s.
  - Headway: within 0.03 s.
- **Q2's costs for W74 and W99.**
  - More jerk ticks (about 3–4×): the hand-back from the law to the model when the leader moves.
  - On M2.6, more W74 hard-braking ticks (5–14×). They are not clamps, and the cause is not traced here.
- **No option is clean for the prototype.**
  - The law takes the crawl away, but the prototype's following law, handed back a slowly moving leader at
    standstill range, brakes up to the maximum. So it clamps (Q1, Q2: 300–1,600 per run) and jerks.
  - D546's lever cuts that to 49 / 208 per run (Q4), still far above Q0's 0 / 0.3.
  - The prototype's crawl is its M0 law's approach to rest. That law is frozen for M0, and a project
    prototype uses the same law.
- **Q3 and Q5 are worse everywhere:** a 0.1 m/s threshold creates the switch it was meant to smooth.
- **Bare M0** is untouched by construction, since every branch requires `lineRest`. `multra-cli 42` is
  byte-identical after the revert.

## After D656

The owner chose Q2 (LINE_QUEUE §6). The committed code, run with the same probe, reproduces Q2's crawl, clamps,
delay, headway and startup lost time for every project and behaviour above. The prototype keeps Q0's values.

**The hard-braking trace.** The probe now splits ticks below −3 m/s² by the vehicle's speed the tick before:

| Project | Model | Moving, Q0 → Q2 (per run) | At rest (published only), Q0 → Q2 |
|---|---|---|---|
| Four-leg | W74 fixture | 269 → 220 | 216 → 196 |
| Four-leg | W74 SUMO preset | 155 → 187 | 354 → 311 |
| M2.6 | W74 fixture | 801 → 728 | 1,585 → 11,607 |
| M2.6 | W74 SUMO preset | 746 → 592 | 2,159 → 41,333 |
| M2.6 | W99 | 851 → 646 | 2 → 2 |

- **The rise is in vehicles at rest.** W74's model at rest, at its exact target behind a leader that has just
  moved, publishes its emergency −8 m/s² request. The engine moves nothing on it.
- **Moving vehicles brake hard less often**, except the SUMO preset on the four-leg (+32 per run).
