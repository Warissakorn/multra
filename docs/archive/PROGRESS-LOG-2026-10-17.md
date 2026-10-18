# PROGRESS log archive — 2026-10-17

Old PROGRESS entries, moved whole, newest first. Nothing here is current: NEXT and ROADMAP
hold status, the decision record holds the reasoning.

## 2026-10-17 — B-8 Stop dwell time implemented (D231)

D230's contract ([evidence](../evidence/stop-dwell.md)).
- **Engine:** the Stop service entry records whole ticks of rest from the vehicle's behaviour when it
  comes to the line; one without the key. Rows BA118–BA123 failed first against a one-tick stub.
- **File and editor:** schema 41, refused below; validation on both models; a behaviour-dialog row
  (failed first). Codec hooks proved by mutation.
- **Measured:** shipped runs byte-identical. On a T-junction copy with Stop controls, a 2 s dwell
  raised minor delay by 13–18 s. Every clamp there sits at a Stop line, with or without the key
  (traced next). +0.25 % Ir on four-leg, +0.41 % on the Stop copy.

`check` 174/174, Linux only.

## 2026-10-17 — B-8 Stop dwell time contract (D230, docs only)

NEXT's item ([STOP_DWELL](../reference/STOP_DWELL.md)). Today a Stop rests a vehicle for 1–2 ticks,
so its effect depends on the time step.
- **The owner's choices:** a driver-behaviour key, absent = today (byte-identical), fixed seconds.
- **The contract:** `stopDwellTime` (s) at schema 41; the rest becomes whole ticks of at least
  that time, recorded when the vehicle comes to the line; rows BA117–BA123.

## 2026-10-17 — B-5 desired-speed distribution curve implemented (D229)

D228's contract ([evidence](../evidence/speed-distribution.md)).
- **Engine:** `speedAt`/`shareOf` in `speed_distribution.hpp`, the old uniform formulas without a
  curve; used by the draw, `quantileOf` and D191's rescale. Rows BA112–BA115 failed first against a
  stub; BA110/BA111 pin today's draw.
- **File and editor:** schema 40, refused below; `INVALID_SPEED_CURVE`; the type dialog's curve box
  and point table (BA116 failed first); the Lab moves a curve with min/max. Codec hooks proved by
  mutation.
- **Measured:** shipped runs byte-identical; a 50 km/h-centred car curve on four-leg moved mean delay
  46.00 → 45.40 s (inside the noise); +0.06 % Ir.

`check` 173/173, Linux only.

## 2026-10-17 — B-5 desired-speed distribution contract (D228, docs only)

NEXT's item after D227, read against ROADMAP M2.1 ("distributions") and tier B
([SPEED_DISTRIBUTION](../reference/SPEED_DISTRIBUTION.md)).
- **The owner's choices:** a piecewise-linear cumulative curve (Vissim's form), decisions and areas
  still linear on the driver's share, set on the vehicle type.
- **The contract:** optional `desiredSpeed.curve` at schema 40, ends equal to `min`/`max`; the same
  single draw inverted through it, so files without a curve stay byte-identical; rows BA110–BA116.

## 2026-10-17 — D223's red-onset clamp traced (D227, no code change)

NEXT's item ([evidence](../evidence/speed-decisions.md#d227-the-seed-47-red-onset-clamp)).
- **Cause:** AMBER §1's red rule meeting a slow arrival. Vehicle 58 went on amber, was 6.7 m short
  at red, could stop at its maximum and was held. The prototype's braking softens near the line,
  so the last tick overshot by 1.6 cm at 0.33 m/s.
- **Not the decision code:** it is idle on those ticks, and slow car types with no decision clamp
  the same way (seed 68). Shipped speeds give no clamp in seeds 42–81.
- **Candidate:** a `v²/(2·gap)` braking floor removed both red-onset clamps but added a creep clamp
  (seed 56). Not adopted; noted in ENGINEERING_NOTES.
- AMBER §1 gains the note. The CLI pin is byte-identical; no code changed.

`check` 172/172, Linux only.

## 2026-10-17 — Acceleration capped under the curve bound (D226)

NEXT's item ([evidence](../evidence/curve-speed.md#d226-acceleration-capped-while-the-bound-holds)).
D225 found that a speed target below the desired speed clamped only the speed, so the distance ran
`½·a·dt²` a tick ahead. D212's curve bound had the same gap.
- **Row BA109** (the BA57 fixture: each tick adds at most the trapezoid of its speeds) failed first.
- **One line in `simulation.cpp`** now caps acceleration for any bound below the desired speed, curve
  or area. It replaces D225's area-only line.
- **No key, no area:** byte-identical, and the Ir is identical too.
- **Key on:** delay moves by ≤ 0.04 s; t-junction clamps 3 → 1.

`check` 172/172, Linux only. CI on PR #176: the sanitizer failure (BA85's dangling reference) is
fixed. The Windows `properties-layout-ui` failure is also red on `main`.

## 2026-10-17 — B-6 slice 2: reduced-speed areas (D224, D225)

NEXT's item, contract first ([SPEED_DECISIONS §7](../reference/SPEED_DECISIONS.md),
[evidence](../evidence/reduced-speed-areas.md)). PR #176 opened for D206–D223 first.
- **The owner's choices:**
  - the anticipation deceleration is a per-area value, else comfortable;
  - delay inside an area is not counted;
  - an area can be limited to chosen lanes, named by lane id.
- **What changed:**
  - engine: `min(desired, area)` inside an area, anticipated before it, with an acceleration cap
    while the area binds;
  - area pieces in the free flow;
  - codec at schema 39, validation (including overlap), compile per lane section;
  - commands, including a lane-drop guard and the lane-id map on split;
  - an editor tab: select a Link, then *Add reduced-speed area*, with lane boxes.
- **Tests:** every engine row but BA97 and BA102 failed first. The rows also caught three faults in
  the first implementation; one of them, the missing acceleration cap, also affects D212's curve
  bound (listed in NEXT). Every codec and command mutation is caught.
- **Measured** with a 30 km/h area on the four-leg west approach: on one lane it slows only that
  lane's movement, by +8.6 s travel time and +1.9 s delay. No clamps.
- No area: byte-identical, +0.77 % Ir (hooks +0.5 %), recorded.

`check` 172/172, Linux only.

## 2026-10-17 — B-6 slice 1: desired-speed decisions (D222, D223)

NEXT's item, contract first ([SPEED_DECISIONS](../reference/SPEED_DECISIONS.md),
[evidence](../evidence/speed-decisions.md)).
- **The owner's choices:**
  - Vissim's model: per-type ranges at a sign, with each driver keeping their place in the range;
  - delay measured against the speed in force;
  - decisions before reduced-speed areas (areas written as a contract only).
- **What changed:**
  - engine, codec at schema 38, validation, compile per lane, and commands;
  - an editor tab: select a Link, then *Add speed decision*.
- **Tests:** the engine and UI rows failed first; the later rows were proved by mutation.
- **Measured** with a 30 km/h decision on the four-leg west approach: travel time +26 s but delay
  only +4–6 s. One red-onset clamp in ten seeds is recorded, not yet traced.
- No decision: byte-identical, hooks +0.10 % Ir.

`check` 170/170, Linux only.

## 2026-10-17 — B-3: accepted deceleration for mandatory lane changes (D220, D221)

NEXT's item, contract first ([MERGE_DECELERATION](../reference/MERGE_DECELERATION.md),
[evidence](../evidence/merge-deceleration.md)).
- **The owner's choices:** two keys, as Vissim has (`mandatoryDecelerationOwn`,
  `mandatoryDecelerationTrailingVehicle`), and the trailing one is read from the changer's behaviour.
- **What changed:**
  - mandatory changes only; the room condition still prevents clamps;
  - schema 37 and two editor rows.
- **Tests:** four rows failed first, and two mutations are caught. The third mutation showed the
  contract's `min(key, maxDeceleration)` could never matter, because following already clamps there,
  so it was removed.
- **Measured** in the shipped projects: the keys barely move anything (delays by ≤ 0.16 s, no
  clamps, the lab's one 2.5 s dead-end wait gone), because their demand rarely meets a refused
  change. Keys absent: byte-identical, +0.002 % Ir.

`check` 168/168, Linux only.

## 2026-10-17 — B-2: commitment deceleration at give-way lines (D218, D219)

NEXT's item, contract first ([COMMITMENT](../reference/COMMITMENT.md),
[evidence](../evidence/commitment-deceleration.md)).
- **The owner's choices:**
  - the key applies at give-way lines only, and the red head keeps the type's maximum (red-light
    running is B-9);
  - the lower of the key and the type's `maxDeceleration` wins.
- **What changed:**
  - `commitmentDeceleration` (schema 36, either model) in the give-way commitment test;
  - an editor row; no Lab row, because the circuits have no give-way line.
- **Tests:** five rows failed first, and three mutations are caught. The first scene clamped even
  without the key, so a sweep chose an 8 m scene.
- **Measured** with the key on every behaviour:
  - T-junction delay falls from 6.45 to 5.65 s at 2 m/s², while clamps rise from 0.2 to 0.8 per seed;
  - M2.6 clamps rise from 1.1 to 8.3, as D69 warned.

  No value ships. Key absent: byte-identical, +0.04 % Ir.

`check` 167/167, Linux only.

