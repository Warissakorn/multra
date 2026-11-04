# Gap seeking (D455, D458)

A mandatory changer whose change is refused for lack of a gap seeks one. This contract extends
cooperative braking ([M3_8_CONTRACT](M3_8_CONTRACT.md) §2, M3.2.8c), and it has two slices:
- the follower opens a gap (slice 1, D455);
- the changer falls back behind its blocker (slice 2, D458).

Rule 4 applies: nothing here is validated.

## 1. Slice 1: the follower opens a gap anywhere in the span (D455)

(2026-11-04) and chose the follower's slice. M3_8_CONTRACT's "Who needs help" drops the look-ahead.

- **Who needs help now:** any stub vehicle that is not waiting, is wholly inside a span with a
  target, and is still a stub after this tick's changes, so its change was refused.
  - D452 and D453 still exclude a change whose landing D450 (b) refuses.
- **Who is not asked:** a stub that itself needs the changer's lane. That is the changer's swap
  partner, and D207's rule holds it.
  - Asked as an ordinary follower, it stood short of its own dead end, where (b) does not count
    it, and the pair locked. Two weaves run alone locked this way in scratch.
- **Unchanged otherwise:** who helps, how, the cap and the closing-speed test. The waiting rule is unchanged.
  A behaviour without the key does not help, and the help is stateless.
- **Why:**
  - A changer refused for lack of a gap drove alongside same-speed traffic for as long as 170 m.
    Help arrived only inside its look-ahead (about 50 m at 10 m/s), by when its partners waited at
    the dead end (weaving lab, D451–D454).
  - SUMO's LC2013 has followers adjust their speed for a blocked strategic change
    (`lcCooperativeSpeed`).
  - Vissim's "advanced merging" gives the merging vehicle and its cooperating trailing vehicles an
    earlier look-ahead.
  - Both are known here from secondary sources only (the [evidence](../evidence/gap-seeking-follower.md)).
- **Rows:** acceptance row A38 is revised. A refused stub far from its dead end now gets a helper.
  Rows BA194–BA195 are added.
- **Still deferred:**
  - the changer's own speed adjustment, to line up with a gap (slice 2);
  - the lane-change state machine, a no-progress timer, and abandoning a change.

## 2. Slice 2: the changer falls back behind its blocker (D458)

The owner said "go on" (2026-11-04) after the lateral lab. In a dense stream, slice 1 is not enough:
the changer rides alongside, or just behind, a target-lane vehicle at its own speed. Its forward test
(rule 3) then fails as well, and nobody acts. Examples:
- the 150 m weave switched on at ×0.8 averaged 43 s of delay;
- in the lateral lab's short weave, 47–62 s.

- **Only with the lateral switch on** (the owner's choice after measuring both ways). Switched off,
  a change is instantaneous the moment a gap opens, so falling back only loses time. Measured that
  way, delay in the weaving lab switched off rose at ×1.0 (25/19/21 → 29/25/30 s) and in the
  lane-change lab by 1.7 %. Switched on, the crossing takes time (D204), so the changer must line up
  with a gap first. Every switched-off run is unchanged.
- **Who seeks:** a stub vehicle that meets all of these:
  - moving, not waiting, and still a stub after this tick's changes;
  - no change under way (no `lateralTarget`);
  - wholly inside a span with a target;
  - its behaviour carries `maxDecelerationCooperativeBraking` (the key switches gap seeking on);
  - its landing is not refused by D450 (b), as for D452.
- **Its blocker:** the target-route vehicle with the nearest rear whose front is ahead of the
  changer's mapped front.
  - It counts only while rule 3 fails against it: alongside, or a leader too close for the
    changer's mandatory braking or this tick's move.
  - A blocker that is the changer's swap partner is left to D207, as in slice 1.
- **What it does:** the changer takes the blocker as a moving virtual leader at gap
  `max(0, rear − front)`. This is the same hold a slice-1 follower gets, applied as a second
  obstacle.
  - Its braking is capped at its type's `comfortableDeceleration`, not at the cooperative key. With
    the key, its own followers in a dense mixed queue were clamped (BA184).
  - Slice 1 then lets the next stream vehicle open the gap behind it.
- **Unchanged:**
  - it never speeds up to slip in ahead;
  - it never waives a safety test;
  - it is stateless, and replay is exact.
- **Why:** SUMO LC2013 slows a changer blocked by its target-lane leader, so that the leader passes.
  This is known from secondary sources only.
- **Rows:** BA204–BA205; [evidence](../evidence/gap-seeking-changer.md).
- **Still deferred:**
  - speeding up to slip in ahead;
  - the lane-change state machine;
  - a no-progress timer;
  - abandoning a change.
