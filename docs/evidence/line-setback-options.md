# A set-back at holding lines: options measured — 2026-11-26

Evidence for [LINE_SETBACK](../reference/LINE_SETBACK.md) (D669). The sources are in
[line-setback-sources](line-setback-sources.md) (D668). Nothing here is validated or calibrated (D544). No engine
change is committed, and the unvalidated marker stays.

## Method

- **The patch.** A temporary patch on `src/core/simulation.cpp` ([patch](../archive/patches/line-setback-options.patch), reverted).
  - `ZZ_SB` sets the set-back `s` in metres. Under `lineRest`, the line law (`applyLineStop` and
    `applyDelayedLineStop`) aims at `line − s` instead of the line. The result is floored at 0.
  - `ZZ_SC` sets the scope:
    - `0`: every holding line (signal heads, give-way and Stop lines, zone lines and gates);
    - `1`: signal heads only.
  - The caps, the line finish, the amber and red decisions, and the observers still use the real line. A vehicle at
    rest short of the line has not passed it.
  - With `s = 0` the patch changes nothing by construction. `multra-cli 42` was byte-identical with the patch
    applied and after the revert.
- **The probe.** It is D661's red-pass probe: red passes, clamps, hard braking (below −3 m/s² while moving), step
  jerk (|Δa| > 4 m/s² in a tick), mean delay, and discharge headway and startup lost time on the two signalised
  studies.
- **Inputs:**
  - five shipped projects, seeds 42–51, `lineRest` on, dt 0.1;
  - arms: the prototype, the W74 fixture and the W99 fixture (every vehicle type on that behaviour);
  - `s` = 0, 0.5 and 1 m, scope 0 and, for the give-way labs, scope 1;
  - build: Release, GCC 13.3.0, at `28f70bd`.

## One car at a red (prototype, from 15 m/s, head at 250 m)

| s (m) | Front short of the line at rest | Crosses the line after green | Clamps |
|---|---|---|---|
| 0 | 0.000 | at once (already at it) | 0 |
| 0.5 | 0.498 | 0.7 s | 0 |
| 1 | 0.999 | 0.9 s | 0 |
| 2 | 1.998 | 1.3 s | 0 |

## Signalised projects (per run; s = 0 / 0.5 / 1 m)

| Project, arm | Delay (s) | Startup lost time (s) | Headway (s) | Clamps | Hard braking |
|---|---|---|---|---|---|
| four-leg, prototype | 47.76 / 47.82 / 47.86 | −0.59 / −0.46 / −0.35 | 2.127 / 2.111 / 2.096 | 0 / 0 / 0 | 903 / 902 / 904 |
| four-leg, W74 | 95.15 / 96.01 / 96.95 | −1.78 / −1.68 / −1.58 | 3.256 / 3.246 / 3.234 | 9.2 / 10.7 / 11.0 | 220 / 230 / 244 |
| four-leg, W99 | 50.57 / 50.60 / 50.68 | −0.65 / −0.54 / −0.43 | 2.004 / 1.990 / 1.979 | 7.9 / 8.3 / 8.5 | 148 / 152 / 156 |
| M2.6, prototype | 45.38 / 45.44 / 45.53 | −1.22 / −1.09 / −0.99 | 2.580 / 2.564 / 2.554 | 0.2 / 0.2 / 0.4 | 3,360 / 3,388 / 3,422 |
| M2.6, W74 | 122.6 / 127.1 / 130.4 | −2.80 / −2.74 / −2.59 | 3.907 / 3.913 / 3.886 | 29.5 / 29.5 / 28.7 | 728 / 765 / 822 |
| M2.6, W99 | 47.84 / 47.90 / 48.01 | −1.31 / −1.19 / −1.08 | 2.456 / 2.444 / 2.433 | 32.9 / 33.2 / 33.7 | 646 / 649 / 658 |
| node analysis, prototype | 41.85 / 34.58 / 34.61 | — | — | 4.7 / 0 / 0 | 1,170 / 1,044 / 1,050 |
| node analysis, W74 | 97.04 / 87.34 / 89.81 | — | — | 19.0 / 15.7 / 16.3 | 310 / 301 / 324 |
| node analysis, W99 | 47.87 / 38.27 / 38.30 | — | — | 18.6 / 13.0 / 12.6 | 255 / 219 / 225 |

- No arm passes a red line at any `s` (0 red passes per run throughout).
- **Startup lost time rises by about 0.1 s per 0.5 m.** That is toward HCM's 2.0 s, but it is still negative in
  every arm. It is a geometric effect: the first car has `s` metres more to cover to its line.
- **Headway falls by about 0.01–0.03 s.** The queue's discharge is measured at the line, and the first cars reach it
  already moving.
- **On M2.6, W74's delay rises 4–6 %**, and its served trips change (1,724 / 1,736 / 1,798 per run). The other
  arms change by less than 0.4 %.

## Give-way labs: every line (scope 0) against signal heads only (scope 1)

| Project, arm | Delay, s = 0 | Scope 0: s = 0.5 / 1 | Scope 1: s = 0.5 / 1 |
|---|---|---|---|
| T-junction, prototype | 6.43 | 6.95 / 7.20 | 6.43 / 6.43 |
| T-junction, W74 | 8.06 | 9.31 / 9.83 | 8.06 / 8.06 |
| T-junction, W99 | 5.97 | 6.51 / 6.65 | 5.97 / 5.97 |
| priority lab, prototype | 5.32 | 5.43 / 5.48 | 5.32 / 5.32 |
| priority lab, W74 | 5.68 | 5.89 / 6.07 | 5.68 / 5.68 |
| priority lab, W99 | 5.11 | 5.23 / 5.30 | 5.11 / 5.11 |

- **At give-way lines a set-back costs 2–22 % of delay** (T-junction W74 at 1 m: +22 %), and hard braking rises.
  A vehicle at rest short of its line must start again to reach it before it may go.
- **With signal heads only, the give-way labs are unchanged to the digit.** These labs have no signal heads.

## A finding outside the question: resting exactly at the line on the node-analysis project

- On `signalised-node-analysis`, today's engine (`s = 0`) clamps 4.7 times per run for the prototype, and its delay
  is 41.9 s.
- A set-back of **0.01 m** removes all of it: 0 clamps, delay 34.5 s. At 0.001 m it is still 4.1 clamps and
  40.6 s. With `lineRest` off: 0 clamps, delay 34.3 s.
- The clamped vehicles are moving at 6–7 m/s, about 290 m into their routes, not at a line. This project's signal
  heads stand at position 0 of their connectors.
- **Hypothesis, not verified:** a vehicle resting with its front exactly at a connector's start is read by a
  conflict or occupancy check as inside the node, which holds crossing traffic. This is a defect of today's default,
  independent of the set-back. A set-back would hide it, not fix it.
