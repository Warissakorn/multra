# The approach to a vehicle at rest, measured on NGSIM Lankershim — 2026-11-26

Evidence for STOP_PROFILE §11 (D666). The owner chose to measure ("ตามแนะนำ", option 1 of
[queue-approach-sources](queue-approach-sources.md#what-could-settle-it)). The question is when drivers begin to slow
for a vehicle at rest ahead. That decides whether B3's early onset (and its +9.5 s delay, D664) or today's law is
nearer to field data. This is a measurement, not a calibration: no MULTRA value is set from it (D544, D178).

## Data

- **NGSIM, Lankershim Boulevard, Los Angeles, 2005-06-16, 08:28–09:00.** It is U.S. DOT FHWA's *Next Generation
  Simulation Vehicle Trajectories*, dataset `8ect-6jqj` on data.transportation.gov. It was downloaded on 2026-11-26
  with `location=lankershim`: 1,607,319 rows at 0.1 s, in feet.
- **Columns used:** `vehicle_id`, `global_time`, `v_length`, `v_class`, `v_vel`, `lane_id`, `int_id`, `preceding`
  and `space_headway` (front to front).
- **Scope:**
  - cars only (`v_class` 2);
  - mid-block, not inside an intersection (`int_id` 0, `lane_id` > 0);
  - speeds at onset of at least 5 m/s.
- **Known limits:**
  - NGSIM's speeds come from video and are noisy, so every speed is smoothed. The onset therefore depends on the
    smoothing, which is varied below.
  - Lankershim is an urban arterial with closely spaced signals. Onset speeds are mostly 6–12 m/s, slower than the
    four-leg's 11–17 m/s.
  - A vehicle id is reused across the two periods, so tracks are split at time gaps.

## Method

Scripts: [onset](queue-approach-ngsim-onset.py) and [first episode](queue-approach-ngsim-first.py). Each is run with
`python3 -I` on the downloaded CSV.

- **Speed:** a centred moving average of `v_vel` over 1 s (varied 0.5–2 s). Acceleration is its difference.
- **Required deceleration at onset:** `req = v_on² / (2·d)`. This is the constant deceleration that would stop the car
  where it actually stopped. MULTRA's law starts braking when this reaches the type's b (2 m/s² for a car).
- **Two onset definitions:**
  1. **The last braking phase** ([onset](queue-approach-ngsim-onset.py)). It goes back from the stop (below 0.5
     m/s) over the continuous phase in which the smoothed acceleration stays below −0.3 m/s² (varied −0.1 to −0.5).
     `d` is the distance actually covered to the stop. Events are classed by the leader:
     - **queue:** the leader is at rest at the onset;
     - **joining:** it is moving at the onset and at rest at the stop;
     - **line:** no `preceding` vehicle.
  2. **The first deceleration episode** ([first episode](queue-approach-ngsim-first.py)). This is the first episode
     with acceleration below −0.3 m/s² that starts behind a leader at rest. It is counted only when the car stops behind
     that same leader within 30 s, and the leader stays below 1 m/s throughout. `d` is the gap to the leader's rear,
     less 2.8 m (the median rest gap measured in definition 1). Drivers often brake in stages, so this onset is
     earlier.

## Results

### The last braking phase (1 s smoothing, −0.3 m/s²)

Percentiles 10 / 25 / 50 / 75 / 90.

| Class | n | Speed at onset (m/s) | req at onset (m/s²) | Peak deceleration (m/s²) | Rest gap (m) |
|---|---|---|---|---|---|
| queue | 195 | 5.4 / 6.1 / 7.6 / 9.4 / 12.1 | 1.24 / 1.52 / **1.88** / 2.53 / 3.02 | 2.1 / 2.6 / 3.5 / 4.7 / 6.5 | 1.2 / 2.1 / 2.8 / 4.2 / 5.6 |
| joining | 354 | 5.9 / 6.8 / 8.6 / 10.4 / 12.5 | 1.13 / 1.39 / **1.74** / 2.21 / 2.66 | 1.9 / 2.4 / 3.2 / 4.3 / 5.6 | 1.2 / 1.9 / 2.7 / 3.7 / 5.0 |
| line | 100 | 6.2 / 7.0 / 9.0 / 11.6 / 14.3 | 1.47 / 1.73 / **2.07** / 2.68 / 4.72 | 2.4 / 2.9 / 3.7 / 4.4 / 9.3 | — |

**Sensitivity:** the median req for `queue` under each measurement choice.

| Smoothing | Onset −0.1 | −0.2 | −0.3 | −0.5 |
|---|---|---|---|---|
| 0.5 s | | | 2.56 | |
| 1 s | 1.79 | 1.82 | 1.88 | 2.05 |
| 2 s | | | 1.45 | |

### The first deceleration episode toward a vehicle at rest

- 354 approaches.
- req at the first onset: 0.66 / 1.00 / **1.42** / 1.93 / 2.55 m/s² (10th to 90th percentile).
- The median speed at onset is 8.8 m/s.
- 16 % start below 0.8 m/s², 25 % below 1.0, and 22 % at 2.0 or above.

### Against MULTRA's laws (by construction, for a car: a = 2.5, b = 2)

- **Today's law** (B0, and B3P toward vehicles) starts braking at req = b = 2.0 m/s².
  - **First episode:** 22 % of the measured drivers start that late.
  - **Last braking phase:** 43 % do.
- **B3** at the desired speed (v = v0) starts braking where IDM first asks −0.3 m/s², at req = √(a·b)/√(a/0.3) ≈
  0.77 m/s².
  - 16 % of the measured first onsets are that early.
  - Below the desired speed, IDM's free term delays the onset. At v = 8.6 m/s with v0 = 14 m/s it starts at about
    req 2.1.

## Readings

1. **Drivers approaching a vehicle at rest start later than B3 and earlier than today's law.**
   - The first onset has a median of 1.4 m/s² of required deceleration.
   - The last continuous braking phase begins at a median of 1.8–1.9 m/s² (1.5–2.6 across the measurement choices).
2. **Approaches are often staged.** A gentle first episode is followed by a pause, then the final braking. Neither
   today's law nor B3 has a pause.
3. **The final braking peaks above b.** The median peak is 3.2–3.7 m/s², where MULTRA's law brakes at b = 2. NGSIM's
   noise inflates peaks, so this is an upper reading.
4. **Approaches to a line and to a queue look alike.** The last-phase medians are 2.07 for a line and 1.88 for a
   queue. That supports one law for both (LINE_STOP §2's premise).
5. **The "queue moves off before arrival" case cannot be separated cleanly.** Of the episodes that start behind a
   leader at rest, 44 end at rest only after the leader had moved. The rest are confounded by leader-id changes and
   lane changes.
6. **Neither B3 nor B0 matches the median, and a law fitted to it would be calibration** on one arterial (D544).
