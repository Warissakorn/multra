# Swap lock: the rear partner too close to accept — 2026-11-04

Branch `claude/relaxed-einstein-nfmzno` on `946232e` (D445) plus this change (D446). Local host:
Linux, GCC 13.3.0, Ninja, `--preset headless` (Debug). **Linux only; no desktop build.** Contracts:
[M3_8_CONTRACT](../reference/M3_8_CONTRACT.md) §2 "Cooperation" (D207) and
[LANE_CHANGE_DURATION](../reference/LANE_CHANGE_DURATION.md) §4 (D208). Follows Finding 3 of the
[lateral-run audit](lateral-run-audit.md). Nothing here is validated (rule 4).

## Diagnosis (lane-change-lab seed 49, vehicles 83 and 88)

Scratch probes (not committed) traced both vehicles switched on (keys 0.5/1.0/1, as in the audit).

- 83 is on `route-30/lane-1` (left lane, a stub) and must reach the middle lane. 88 is on
  `route-29/lane-2` (middle lane, a stub) and must reach the left lane. So this is a swapping pair.
- At t = 61.1 s both starts are accepted. The two are at 6.2 and 5.2 m/s, and 88's front is about
  2.7 m behind 83's target place.
- Each body moves to `lateral ±0.8`, still inside its own lane, and stops there. D208's covering
  test refuses every move over the line: 83 would cover the middle lane, where 88 is too close behind
  to accept 83 as its leader, and 88 would cover the left lane with 83 too close ahead.
- Both reach about 11 m/s and drive in parallel, the gap unchanged, until 83 slows near its dead end
  (t ≈ 110 s).
- D207 exists for this pair ("the rear one yields"), but its partner test ends at the rear one's
  `standstillDistance` (2 m). The rear one was beyond that and still inside the distance D208 needs,
  so nobody yielded and nobody could cover.
- Switched off, the change is instantaneous at the accepted start, so there is no lock.
- Refusing the second start would not have helped: 83 alone still could not cover with 88 too close
  behind.

## Change

- **D207's partner, extended:** while X's change is under way (`lateralTarget` set, so switched-on
  runs only), a rear stub Y beyond its standstill distance is still X's partner as long as it would
  not accept X, at X's target place, as its leader. Y yields as D207 already defines: a moving
  obstacle at X's speed, braking no harder than comfortable.
- **One test:** `acceptsAsLeader` (the D208 follower test) is now one function. `coversSafely` and
  the D207 partner test both call it.
- **File split:** `yieldMutual` moved from `lanes.cpp` (496 lines, at the limit) to the new
  `lane_swap.cpp`. Its code is unchanged except for the partner condition.

## Row (`laneswap`)

| Row | Case | Before | After |
|---|---|---|---|
| BA161 | The `swap()` pair, switched on (keys 0.5/1.0). The rear one's front is `standstillDistance + 1` m behind the front one's target place; front 5 m/s, rear 4 m/s | Fails: neither crosses within 8 s (they cross at 14.7 s) | Both cross within 8 s, the front one first; no clamp, no 2D overlap, no braking harder than comfortable |

**Forcing, asserted first:** the gap is beyond the standstill distance, and both changes start on
the first tick. A sweep of gap 2.2–3.5 m and speeds 3–10 m/s found the locking placements; most
placements swap in 1.8 s.

`ctest --preset headless`: **174/174** pass, and `check` passes.

## Product runs, seeds 42–51, `--threads 4`

- **Switched off:** every batch is identical to D445's. No vehicle has a target there, so the
  extended test never runs.
- **Switched on:** completed trips and clamps are unchanged for every project. The m2.6 clamps stay
  at 1, the switched-off count. Only the lab's on output changed: its movement times, not its counts.

**Change durations** (scratch probe: from the tick a target is set to the crossing), switched on:

| Project | Changes over 8 s, seeds 42–51 (before → after) | Longest (before → after) |
|---|---|---|
| lane-change-lab | 36 → **27** | 56.2 s → 48.2 s |
| m2.6-study-template | 20 → 18 | 91.4 s → 91.4 s |

- In lab seed 49, 83 and 88 now both cross at t = 62.8 s, 1.7 s after they start. Before, they
  waited until about 110 s.
- **Residue, outside this change:** seed 49's longest wait is now vehicle 580, 36 s. It needs the
  left lane, where car 585, which is not changing lanes, rides alongside at almost the same speed
  (13.4 vs 12.6 m/s) and only slowly pulls ahead. Nobody seeks a gap. That is the generic
  cooperation and gap-seeking case (M3.2.8c covers it only near the dead end), not a swap pair.
- The long m2.6 waits were not examined here.
