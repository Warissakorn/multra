# A softer stop profile (B): options measured — 2026-11-25

Evidence for [STOP_PROFILE](../reference/STOP_PROFILE.md) (D663). The sources are in
[stop-profile-sources](stop-profile-sources.md). Nothing here is validated, and the unvalidated marker stays.

## Method

- **The patch.** A temporary patch on `src/core/line_stop.cpp` and `src/core/simulation.cpp`
  ([patch](stop-profile-options.patch), reverted). `ZZ_B` selects an option and `ZZ_T` sets T.
  - It changes only the first part of `applyLineStop`: the line law toward `line`, which is the nearest holding line
    or a vehicle at rest less the standstill gap.
  - D258's floor for W74 and W99 after it is unchanged, and so is W99 §4's own call of `lineStopAcceleration`.
  - So runs with `lineRest` off are unchanged by construction.
- **The IDM term.** It is Kesting & Treiber (2008), Eq. (2)–(3), toward a leader at rest at `line`, with `s0 = 0`
  (the target already holds the standstill gap) and `Δv = v`:

  `a_IDM = a·[1 − (v/v0)⁴ − (s*/line)²]`, with `s* = v·T + v²/(2√(a·b))`.

  - a is `maxAccelerationAt(type, v)` (D608).
  - b is the type's `comfortableDeceleration`.
  - v0 is the vehicle's `desiredSpeed`.
- **The probe.** D662's probe (`ZZ_RT`, `ZZ_DT`, `ZZ_LR`), not committed. It adds three measures:
  - **step jerk:** ticks whose acceleration changed by more than 4 m/s² from the vehicle's previous tick, per run
    (D657's "jerk");
  - **jerk > 10 m/s³:** moving ticks (faster than 0.1 m/s the tick before) whose `|Δa|/dt` exceeds 10, per run;
  - **mean deceleration:** the mean of moving ticks below −0.5 m/s².
- **Inputs.**
  - D655's six shipped projects;
  - the project prototype, the W74 fixture, the W74 SUMO preset and the W99 fixture;
  - seeds 42–51, `lineRest` on, dt 0.1;
  - the four-leg also at dt 0.05 and 0.2, and with `reactionTime` 0.5 s (R9).
- **Build:** Release, GCC 13.3.0, at `829e3d5`. `multra-cli 42` is byte-identical after the revert.
- **B0 reproduces D662** to the digit (four-leg prototype: crawl 1.3 s, 0 clamps, hard braking 1,027, step jerk
  296).

## Options

| Option | The line law |
|---|---|
| B0 | Today (D652): up to the curve `√(2b·gap)`, then braking at b on it, then rest |
| B1 | `min(B0, a_IDM)`, T = 1.03 s: IDM as published, never later than today |
| B2 | B1 with IDM bounded at the kinematic deceleration: `min(B0, max(a_IDM, −v²/(2·line)))`. T = 1.03 or 1.51 s |
| B3 | B2 with T = 0 (no headway term) |
| B4 | B2 (T = 1.03 s) without IDM's free-road term `(v/v0)⁴` |

- **T** has no MULTRA value. 1.03 and 1.51 s are the ends of Kesting & Treiber's Table 1 (p. 7), which is one car in
  city traffic, not a default. B3 needs no new value.
- **B2's bound** prevents IDM from braking harder than the stop needs. Without it (B1), IDM settles near `v ≈ s/T` and
  creeps toward the line.

## One car, computed (no engine; from 15 m/s, 200 m behind a line, a 1.4, b 2, v0 16, dt 0.1)

| Option | Brakes from | Peak deceleration | Rest | Crawl below 1 m/s |
|---|---|---|---|---|
| B0 | 64 m | 2.00 (constant) | 16.7 s, at the line | 0.5 s |
| B1, T 1.03 | 164 m | 8 (the final cap) | 22.8 s | 4.3 s |
| B2, T 1.03 | 164 m | 1.77 | 18.1 s, at the line | 0.6 s |
| B2, T 1.51 | 173 m | 1.52 | 18.8 s | 0.7 s |
| B3 | 144 m | 2.00 | 17.3 s | 0.5 s |

- Under B2 and B3 the deceleration builds up gradually from 0, instead of jumping from 0 to b in one tick.
- **The step to rest stays.** Every option that ends exactly at the line ends at a constant deceleration (b, or
  `v²/(2·gap)`), then rests. The size of the last step depends on where the last tick falls: 0.4–1.5 m/s² across
  dt 0.05–0.2.
- **IDM's soft end is its creep (B1).** The deceleration falls to about 0 only because the car approaches the line
  at about `s/T`, which takes seconds and never reaches the line exactly.

## SUMO 1.18, one car (sigma 0, accel 1.4, decel 2, tau 1.2, dt 0.1, a `<stop>`)

- **Krauss:**
  - brakes from 55 m;
  - −2.00 m/s² constant;
  - rests at the stop exactly after 7.5 s of braking, with a step from −2 to 0.
  - This is B0's profile.
- **IDM:**
  - from 14.9 m/s it eases from about 290 m: −0.1 at 290–150 m, −0.5 at 66 m, −1.0 at 42 m;
  - its peak is −1.9 at about 9 m, then −1.2 at 2.5 m and −0.4 at 0.5 m;
  - it **rests 0.11 m short of the stop**, with 3.0 s below 1 m/s.
- Speeds in `fcd-output` are rounded to 0.01 m/s, so these decelerations are coarse.

## Results (per run, seeds 42–51, dt 0.1)

### Four-leg

Clamps / hard braking / step jerk / delay (s).

| Option | Prototype | W74 fixture | W74 SUMO | W99 |
|---|---|---|---|---|
| B0 | 0.0 / 1,027 / 296 / 47.74 | 9.2 / 220 / 625 / 95.15 | 2.3 / 187 / 686 / 111.29 | 7.9 / 148 / 266 / 50.57 |
| B1 | 1.2 / 1,065 / 3,120 / 28.85 | 9.9 / 2,548 / 12,558 / 95.19 | 14.2 / 2,440 / 9,957 / 112.83 | 82.8 / 1,819 / 5,779 / 50.59 |
| B2, T 1.03 | 0.0 / 285 / 24 / 56.68 | 8.9 / 208 / 175 / 95.19 | 3.0 / 174 / 176 / 112.09 | 9.5 / 144 / 56 / 50.60 |
| B2, T 1.51 | 0.0 / 220 / 23 / 56.63 | 9.0 / 208 / 177 / 95.25 | 3.1 / 164 / 175 / 112.25 | 15.1 / 155 / 68 / 50.62 |
| B3 | 0.0 / 410 / 25 / 57.23 | 9.4 / 212 / 389 / 95.15 | 2.8 / 178 / 306 / 111.50 | 8.9 / 142 / 154 / 50.58 |
| B4 | 0.0 / 677 / 25 / 59.47 | 9.0 / 213 / 178 / 95.36 | 2.8 / 184 / 181 / 112.28 | 8.4 / 147 / 56 / 50.57 |

Jerk > 10 m/s³ / mean deceleration / crawl median (s):

| Option | Prototype | W74 fixture | W74 SUMO | W99 |
|---|---|---|---|---|
| B0 | 1,589 / −1.93 / 1.3 | 11,788 / −1.66 / 3.1 | 10,113 / −1.51 / 3.1 | 3,935 / −1.63 / 2.1 |
| B1 | 1,999 / −1.43 / 4.6 | 21,922 / −1.28 / 6.7 | 22,556 / −1.20 / 7.0 | 11,437 / −1.28 / 5.0 |
| B2, T 1.03 | 1,185 / −1.23 / 2.4 | 14,699 / −1.11 / 3.5 | 12,293 / −1.02 / 3.6 | 8,846 / −1.13 / 2.4 |
| B2, T 1.51 | 1,170 / −1.19 / 2.5 | 11,946 / −1.01 / 3.7 | 10,261 / −0.93 / 3.9 | 6,192 / −1.03 / 2.5 |
| B3 | 1,237 / −1.30 / 2.4 | 18,262 / −1.58 / 3.2 | 15,189 / −1.44 / 3.2 | 11,595 / −1.50 / 2.2 |
| B4 | 1,303 / −1.55 / 1.6 | 10,386 / −1.26 / 3.4 | 9,110 / −1.13 / 3.6 | 3,737 / −1.37 / 2.3 |

- Headway and startup lost time move by at most 0.01 s and 0.02 s under every option (prototype 2.12 s / −0.58 s,
  W74 fixture 3.26 / −1.78, W99 2.00 / −0.65).
- Trips are unchanged (±1).

### M2.6

Clamps / hard braking / step jerk / delay (s) / trips.

| Option | Prototype | W74 fixture | W74 SUMO | W99 |
|---|---|---|---|---|
| B0 | 0.3 / 3,814 / 1,170 / 45.35 / 1,958 | 29.5 / 728 / 2,842 / 122.55 / 1,724 | 14.3 / 592 / 2,256 / 130.21 / 1,400 | 32.9 / 646 / 1,102 / 47.84 / 1,957 |
| B1 | 2.3 / 3,536 / 10,425 / 26.80 / 1,968 | 40.1 / 10,898 / 51,599 / 126.37 / 1,656 | 60.7 / 8,193 / 33,740 / 138.09 / 1,446 | 345.2 / 6,741 / 21,520 / 47.91 / 1,957 |
| B2, T 1.03 | 0.0 / 1,261 / 162 / 54.41 / 1,952 | 25.9 / 580 / 576 / 115.58 / 1,534 | 12.0 / 599 / 597 / 133.27 / 1,454 | 35.3 / 629 / 253 / 47.91 / 1,957 |
| B2, T 1.51 | 0.0 / 977 / 139 / 53.83 / 1,952 | 24.7 / 564 / 584 / 116.39 / 1,518 | 11.8 / 582 / 599 / 138.68 / 1,494 | 54.4 / 662 / 294 / 47.92 / 1,957 |
| B3 | 0.1 / 1,727 / 168 / 54.91 / 1,951 | 29.6 / 698 / 1,512 / 127.18 / 1,702 | 13.6 / 674 / 1,071 / 147.03 / 1,559 | 34.4 / 627 / 629 / 47.86 / 1,957 |
| B4 | 0.1 / 2,552 / 176 / 56.36 / 1,949 | 24.0 / 567 / 578 / 103.37 / 1,499 | 12.1 / 579 / 595 / 132.48 / 1,411 | 35.8 / 636 / 272 / 47.99 / 1,957 |

- M2.6 is oversaturated for W74. Its trips swing by ±11 % between options, and its delay falls when fewer trips
  arrive. These two columns are not read as better or worse without a trace.

### The other four projects

- **`priority-rule-lab` and `t-junction-priority`:** step jerk falls under B2–B4 for every model (prototype 67 → 29–37
  and 50 → 18–24; W99 58 → 23–41 and 54 → 13–29). Delay rises by 0.1–0.4 s, and clamps stay at most 0.4 per run,
  except under B1 (up to 3.2).
- **`lane-change-lab`:** every option gives B0's figures. No line or standing car binds the law there.
- **`weaving-lab`:**
  - The prototype is unchanged.
  - W74's figures move by at most 4 %.
  - **W99's clamps rise:** 30.7 per run under B0, against 70.0 (B2, T 1.03), 86.0 (B2, T 1.51), 42.6 (B3), 72.9 (B4)
    and 153 (B1).

### Timestep and reaction delay (four-leg)

Clamps / hard braking / step jerk / delay (s):

| Case | Option | Prototype | W74 fixture | W99 |
|---|---|---|---|---|
| dt 0.05 | B0 | 0.0 / 1,970 / 303 / 47.94 | 13.4 / 527 / 682 / 94.05 | 9.6 / 270 / 288 / 50.57 |
| | B2 | 0.0 / 565 / 24 / 56.93 | 13.4 / 504 / 192 / 94.27 | 10.7 / 247 / 59 / 50.57 |
| | B3 | 0.0 / 830 / 25 / 57.40 | 14.0 / 506 / 419 / 94.12 | 8.8 / 255 / 151 / 50.66 |
| | B4 | 0.0 / 1,414 / 25 / 59.42 | 13.3 / 511 / 193 / 94.25 | 9.4 / 251 / 61 / 50.44 |
| dt 0.2 | B0 | 0.0 / 562 / 252 / 47.97 | 10.9 / 100 / 596 / 95.88 | 10.0 / 84 / 290 / 51.23 |
| | B2 | 0.0 / 179 / 24 / 56.49 | 10.4 / 93 / 165 / 95.85 | 9.3 / 79 / 47 / 51.19 |
| | B3 | 0.0 / 241 / 25 / 56.51 | 10.2 / 96 / 379 / 95.72 | 9.0 / 78 / 156 / 50.86 |
| | B4 | 0.0 / 378 / 26 / 58.13 | 10.3 / 93 / 168 / 95.85 | 9.9 / 85 / 52 / 51.17 |
| `reactionTime` 0.5 s | B0 | 0.5 / 2,266 / 405 / 49.78 | 8.8 / 419 / 1,130 / 111.09 | 8.3 / 1,527 / 447 / 52.02 |
| | B2 | 0.8 / 2,375 / 542 / 57.09 | 7.5 / 320 / 337 / 111.38 | 10.5 / 691 / 179 / 52.14 |
| | B3 | 0.9 / 2,368 / 532 / 56.80 | 7.5 / 372 / 757 / 111.25 | 8.4 / 1,390 / 276 / 52.14 |
| | B4 | 1.3 / 2,503 / 444 / 58.36 | 8.1 / 343 / 343 / 111.12 | 9.5 / 905 / 196 / 52.03 |

B2 is T 1.03 s here. The reductions hold across dt. Under a reaction delay the prototype's step jerk rises under every
option, because R9's start rule and reapplied law still switch in one tick.

## Readings

1. **The onset step is most of the step jerk.** B2–B4 remove most of it:
   - prototype −92 %;
   - W74 −38 to −72 %;
   - W99 −42 to −79 %.
2. **The prototype's hard braking falls** (four-leg 1,027 → 220–677; M2.6 3,814 → 977–2,552). PROTOTYPE_QUEUE §3 traced its extra
   hard braking to followers behind a leader braking late at a constant 2 m/s². That link is not re-traced here.
3. **The prototype's delay rises by 9–12 s** on the four-leg (47.7 → 56.6–59.5) and M2.6 (45.4 → 53.8–56.4), about
   20 %, while its trips stay the same. IDM brakes 80–110 m earlier toward a red or a standing queue. It was not traced
   whether the delay comes from approaches to reds that turn green before arrival, or from approaches to queues.
   W74's and W99's delay on the four-leg hardly moves: their own models already brake before the line law binds.
4. **B1 is not usable:** IDM alone creeps to the line and then clamps or caps (W99 on M2.6 345 clamps per run).
5. **jerk > 10 m/s³ rises under B2 and B3 for W74 and W99.** Their D258 floor `−v²/(2·gap)` now meets a gentler law
   and switches between the two more often. B4 is the one option that lowers it for every model on the four-leg.
6. **No option removes the last step to rest.** An exact stop at the line in finite time ends at a non-zero
   deceleration. A deceleration that falls to 0 at rest needs a profile shape: Akçelik & Biggs's (unread) or a new
   one, which would need a citation (D178).
