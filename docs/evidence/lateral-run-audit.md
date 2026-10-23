# Lateral behaviour in product runs: an audit — 2026-10-23

`main` at `048cc89`, no engine change. Local host: Linux, GCC 13.3.0, Ninja, Qt 6.4.2 offscreen,
`--preset desktop` (Debug). **Linux only.** Contracts: [LATERAL](../reference/LATERAL.md) §5–§8,
[LANE_CHANGE_DURATION](../reference/LANE_CHANGE_DURATION.md) §3–§5, §9. Nothing here is validated
(rule 4).

## Setup

- **Shipped data never runs this code.** No behaviour in `data/` carries the LATERAL §9 keys, so
  every shipped run keeps `lateral` at 0 and changes lanes instantly (LATERAL §8).
- **Switched on** as in [lane-change-duration](lane-change-duration.md): a copy of `data/` with
  `lateralMinDistanceStanding` 0.5, `lateralMinDistanceAt50` 1.0 and `maxLateralSpeed` 1 on both
  catalog behaviours.
- **Batch numbers:** `multra-cli --data-dir <dir> --project <p> --seeds 42-51 --threads 4`, run
  once with `data/` (off) and once with the copy (on).
- **Per-tick checks:** a scratch program, not committed, linked against `multra_core`/`multra_project`.
  After every `stepSimulation` it checks:
  - 2D overlap: every pair of `appendVehicleSpans` bodies on one segment, positive area;
  - `|lateralSpeed|` against the cap;
  - continuity of the road position `centre + lateral` within one carriageway, across crossings;
  - a vehicle moving backwards or with negative speed;
  - how long each change takes, from target set to target cleared;
  - targets still set at the end of the run.

## What holds

The invariants hold in all four projects, seeds 42–51, switched on:

- no 2D overlap;
- `|lateralSpeed|` never above 1 m/s;
- no jump in the road position;
- no vehicle moving backwards or with negative speed.

Every lane change is a lateral one (none instantaneous). The median change takes 3.4 s at
1 m/s, one tick short of 3.5 m because the probe dates a change from the tick it first sees the
target set. `t-junction-priority` has no lane change; it is unchanged.

| Project, seeds 42–51 | Completed, off → on | Clamps, off → on |
|---|---|---|
| lane-change-lab | identical in every seed | 0 → 0 |
| four-leg-signalised | identical, except seed 49: 397 → 396 | 0 → 0 |
| m2.6-study-template | seed 45: **1908 → 1531**; others within 4 | **1 → 33** |

The lab's seed-49 clamps and its 626 completed trips in
[lane-change-duration](lane-change-duration.md) are gone since D208/D210.

## Finding 1 — a stuck changer straddles the line and jams both lanes (m2.6 seed 45)

Vehicle 613 on `link:link-13/path-4` (lane-18):

1. It starts its change at 273.7 m.
2. It steps back once (D208).
3. It stops at its dead end at 277 m, about t = 1120 s.
4. It then moves outward to `lateral` = 1.75, with its centre exactly on the lane line, and stays
   there with its target at 3.5 until the run ends.

Its body covers lane-17, so the lane-17 queue (617, 623, 631, …) stops behind it as well as its
own lane. 119 vehicles are left active, against 52 switched off.

Mechanism: at the dead end no span covers the body any more, so the crossing cannot happen.
LATERAL §7 still lets `yT` be clamped to "on the boundary at most". The D208 step back runs only
when `coversSafely` refuses an outward move, so nothing brings the body back. This contradicts
LANE_CHANGE_DURATION §4 ("it never waits straddling the line"). It is §9's reopen condition: "a
changer waits so long it misses its window: then abandoning".

## Finding 2 — repeated clamps on a waiting changer (m2.6 seeds 42, 43)

All 13 clamps of seed 42 come from one episode at the same place. Vehicle 1133 on
`link:link-13/path-4`, lane-18, is clamped every tick from t = 2028.8 s to 2032.8 s. Throughout,
it is held at `lateral` 1.1 with target 3.5, and its body is over the line toward lane-17.
Its car-following accepts a move that the hard cap then refuses. Under the existing clamp rule,
its distance creeps forward (11.85 → 13.19 m) while it reports speed 0. Switched off, the seed
has no clamp. The straddler's leader search (D210) and the hard cap disagree on what is ahead.
That is not diagnosed further here.

## Finding 3 — two changers swapping lanes lock side by side (lane-change-lab)

Vehicles 83 (`route-30/lane-1`, target −3.5) and 88 (`route-29/lane-2`, target +3.5) in seed 49
both start their change at t = 61.1 s, just after insertion and about 1 m apart along the road.
D206 counts each changer at its target place, so a swap passes both start tests.

Each then waits at the line (D208) with `lateral` ±0.8, alongside the other at the same desired
speed, for about 50 s and 500 m. Neither adapts its speed to make the gap. The lock breaks only
when 83 slows near its dead end (t ≈ 110 s). Seed 44 has the same pair pattern (233 and 236,
45 s). Across the ten lab seeds, 0–8 changes per seed take longer than 8 s. Throughput is
unaffected here, because the lanes are long. On a short approach the pair would miss their
window, as in Finding 1. This is §9's "two changers wait on each other" condition, in a soft
form.

## Display (read, not re-measured)

The canvas draws lane centre + `lateral` along the left normal of the heading
(`canvas_run.cpp`). A lateral crossing is never slid. BA46 pins both. Two limits stay as
[lateral-display](lateral-display.md) states them: the body does not yaw while it moves sideways,
and the offset is exact only on straight lanes.
