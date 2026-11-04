# The start's target-lane filter at the window's end — 2026-11-04

Branch `claude/relaxed-einstein-nfmzno` on `21090d8` (D447) plus this change (D448). Local host:
Linux, GCC 13.3.0, Ninja, `--preset headless` (Debug). **Linux only; no desktop build.** Contracts:
[LANE_CHANGE_DURATION](../reference/LANE_CHANGE_DURATION.md) §3 and [LATERAL](../reference/LATERAL.md) §6.
Nothing here is validated (rule 4).

## The gap

D444 made the start's target and the crossing read the target lane through one lookup,
`targetSectionAt`. The start's safety filter did not. `decideLaneChanges` builds `sideways` with
`followerFilter` on the target route at the mapped front, and `followerFilter` takes `frontPart`.

When the mapped front is exactly a join at the window's end (a stub standing at its dead end),
`frontPart` picks the downstream part, a Connector on another carriageway. `lateralCounts` then counts
every span on the target lane, because they are on a different carriageway. The error is
conservative: a span that a correct filter ignores counts as alongside, and the start is refused.

## Change

- `targetPartAt` (`lateral.cpp`) returns the target route's part, using D444's to-range rule.
  `targetSectionAt` is now its cross-section.
- `followerFilterOn` takes the front's part. `followerFilter` calls it with `frontPart`, so every
  other caller is unchanged.
- The two start sites in `decideLaneChanges` (mandatory and discretionary) call
  `followerFilterOn(…, targetPartAt(…), …)`.

## Row (`lanechangeduration`)

| Row | Case | Before | After |
|---|---|---|---|
| BA162 | Three lanes. The left lane is a stub onto the middle one, whose route joins a part on another carriageway at the window's end (300 m). A car on the right lane, alongside, leans 0.3 m into the middle lane's far edge. The changer stands at 299 m (control), then at 300 m | Control accepted; at 300 m refused (the copy counted as alongside) | Both accepted, target −3.5 |

**Forcing, asserted first:** the leaning car casts an `own = false` span on the middle lane, and the
target route has its join at 300 m.

`ctest --preset headless`: **174/174** pass, and `check` passes.

## Product runs, seeds 42–51

Every batch of all four projects, off and on, is byte-identical to D447's, and so is the
change-duration probe. No product run reaches this case; the evidence is the row.
