# Lateral crossing at the window's end — 2026-11-03

Branch `claude/relaxed-einstein-nfmzno` on `59df944` plus this change (D444). Local host: Linux,
GCC 13.3.0, Ninja, `--preset headless` (Debug). **Linux only; no desktop build** (Qt was not
installed). Contracts: [LATERAL](../reference/LATERAL.md) §7, [LANE_CHANGE_DURATION](../reference/LANE_CHANGE_DURATION.md)
§3, §9. Follows Finding 1 of the [lateral-run audit](lateral-run-audit.md). Nothing here is validated
(rule 4).

## Diagnosis (m2.6 seed 45, vehicle 613)

A scratch probe (not committed) printed vehicle 613's state and every span of its route each tick,
switched on as in the audit (keys 0.5/1.0/1 on the catalog behaviour of a copied `data/`).

- The route `link:link-13/path-4` (lane-18) has a dead end at 277 m. Its span onto `path-3` runs
  over [240, 277] on both sides. On `path-3`, lane-17 is the part [240, 277], and `connector-53`
  starts at 277.
- The change starts at 255.4 m with a target of 3.5 m. The changer steps back once (D208), slows,
  and stands at its dead end, 277 m, at t = 1119.4 s with `lateral` 0.8.
- At 277 m the span still holds the body (`front ≤ fromEnd`). But the mapped front, 277, is exactly
  the join on `path-3`, and `frontPart` puts a front at a join on the downstream part:
  `connector-53`, another carriageway. `crossingOn` therefore found no crossing, and the clamp let the
  centre move out to the line (1.75 m) and stay there. Its body covered lane-17 until the run ended.
- The same lookup gives the start's target (`lateralStart`). A changer that starts at the window's
  end gets the Connector's centre, 0 m, as its target, so it never moves.

So the start and the crossing disagreed about the lane at the window's end. The 2D stopping point
proposed in review was not needed: at the dead end, the span still holds the body.

## Change

- `targetSectionAt` (`lateral.cpp`) reads the target lane's section inside the span's to-range. At a
  join at the window's end, that is the part the span leads onto. `lateralStart` and `crossingOn`
  both call it.
- `spanHolds` (`lanes.hpp`) is the one "the span holds the body" test. It replaces four copies in
  `lanes.cpp` and `lateral_motion.cpp`; the expression is unchanged.
- On a side with no crossing, `moveLaterally` clamps the target so the whole **body** stays in its
  lane: `±max(0, half − width/2)`. Before, it clamped the centre to the line. A body already over
  such a line moves back.

## Rows (`lanechangeduration`, `lateralmotion`)

| Row | Case | Before | After |
|---|---|---|---|
| BA158 | A stub standing at its dead end, where the window ends at a join on the target route (a 50 m part on another carriageway) | Fails: target 0, no crossing | Target −3.5; crosses; target cleared |
| BA159 | A target across a line no span covers, from the lane centre and from a centre on the line, standing past the window | Fails: the body covers the other lane | It never covers the other lane; `lateral ≥ −(1.75 − width/2)` |
| BA44 | Without a span, the target across the line | Pinned the centre on the line (D189) | Now pins the body's edge on the line (D444) |

Each forcing is asserted first: BA158 checks the join at the window's end, and BA159 starts with the
body past the window.

`ctest --preset headless`: **174/174** pass, and `cmake --build build/headless --target check` passes.

## Product runs, seeds 42–51, `--threads 4`

`multra-cli --data-dir <dir> --project <p> --seeds 42-51`, with `data/` (off) and the copy (on).

| Project | Completed, off | Completed, on (audit → now) | Clamps on (audit → now) |
|---|---|---|---|
| m2.6-study-template | 1967, 2054, 1994, **1908**, 1977, 1959, 1932, 1909, 1930, 1965 | seed 45: **1531 → 1903**; all seeds: 1967, 2052, 1994, 1903, 1974, 1957, 1931, 1909, 1930, 1961 | 33 → 28 (seed 42: 13, 43: 6) |
| lane-change-lab | 885, 893, 863, 899, 887, 865, 865, 852, 860, 864 | identical to off | 0 |
| four-leg-signalised | 494, 504, 514, 525, 511, 517, 503, 452, 514, 513 | identical to off | 0 |
| t-junction-priority | 266, 283, 285, 296, 281, 275, 251, 237, 278, 271 | identical to off | 0 |

- The m2.6 off column for seed 45 equals the audit's 1908. The switched-off path changed only
  through `spanHolds`, the same expression as before.
- The four-leg counts differ from the audit's because that project gained its cool-down since
  (schema 27).

**Straddles** (scratch probe: the longest run of ticks in which a vehicle casts an `own = false`
span), m2.6, switched on:

- No vehicle straddles at the end of any seed.
- Seed 45 ends with 57 vehicles active; the audit had 119 on and 52 off.
- The longest straddle is 2.4–2.6 s in every seed except seed 42, where it is 5.8 s (vehicle 1133,
  Finding 2).
- The longest standing straddle is 0.6–1.8 s.

## Still open

- **Finding 2** (waiting-changer clamps, m2.6 seeds 42/43): 13 and 6 clamps; not addressed here.
- **Finding 3** (two changers swapping lanes lock side by side, lab seeds 44/49): not addressed.
  The pair waits at `lateral ±0.8`, where the body is still inside its lane. This probe does not
  count it as a straddle, so it shows no change.
- **Another frontPart lookup at the window's end:** the safety filter `followerFilter`, called on the
  target route at the mapped front from `decideLaneChanges`, still uses `frontPart`. It is not
  changed here; a start exactly at the window's end can read the downstream part's band.
